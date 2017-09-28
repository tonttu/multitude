// For PRIx64
#define __STDC_FORMAT_MACROS

#include "FfmpegDecoder.hpp"

#include "Utils.hpp"
#include "AudioTransfer.hpp"

#include <Nimble/Vector2.hpp>

#include <Radiant/Allocators.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/Sleep.hpp>

#include <Resonant/DSPNetwork.hpp>

#include <Valuable/AttributeBool.hpp>
#include <Valuable/State.hpp>

#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QThread>
#include <QFileInfo>

#include <array>
#include <cassert>

extern "C" {
  typedef uint64_t UINT64_C;
  typedef int64_t INT64_C;

# include <libavdevice/avdevice.h>

# include <libavutil/frame.h>
# include <libavutil/imgutils.h>
# include <libavutil/opt.h>
# include <libavutil/pixdesc.h>

# include <libavformat/avformat.h>

# include <libavcodec/avcodec.h>

# include <libavfilter/avfiltergraph.h>
# include <libavfilter/buffersink.h>
# include <libavfilter/buffersrc.h>

# include <libswscale/swscale.h>
}

#ifdef RADIANT_LINUX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/videodev2.h>
#endif

namespace
{

  int libavLock(void ** mutexPtr, enum AVLockOp op)
  {
    Radiant::Mutex *& mutex = reinterpret_cast<Radiant::Mutex *&>(*mutexPtr);

    switch(op) {
    case AV_LOCK_CREATE:
      mutex = new Radiant::Mutex();
      return !mutex;

    case AV_LOCK_OBTAIN:
      mutex->lock();
      return 0;

    case AV_LOCK_RELEASE:
      mutex->unlock();
      return 0;

    case AV_LOCK_DESTROY:
      delete mutex;
      mutex = 0;
      return 0;
    }
    return 1;
  }

  RADIANT_TLS(const char *) s_src = nullptr;

  void libavLog(void *, int level, const char * fmt, va_list vl)
  {
    if(level > AV_LOG_INFO) return;
    //AVClass ** avclass = reinterpret_cast<AVClass**>(ptr);

    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), fmt, vl);
    for(int i = static_cast<int>(strlen(buffer)) - 1; i >= 0; --i) {
      if(buffer[i] == '\r' || buffer[i] == '\n')
        buffer[i] = '\0';
      else
        break;
    }

    QString msg = QString("%1: %2").arg((const char*)s_src).arg(buffer);

    if(level > AV_LOG_WARNING) {
      Radiant::info("%s", msg.toUtf8().data());
    } else if(level > AV_LOG_ERROR) {
      if (!msg.contains("max_analyze_duration reached") && !msg.contains("First timestamp is missing,")) {
        Radiant::warning("%s", msg.toUtf8().data());
      }
    } else {
      if (!msg.contains("too full or near too full")) {
        Radiant::error("%s", msg.toUtf8().data());
      }
    }
  }

  void avError(const QString & prefix, int err)
  {
    char buffer[128];
    av_strerror(err, buffer, sizeof(buffer));
    Radiant::error("%s - %s", prefix.toUtf8().data(), buffer);
  }

  // Supported audio formats
  const AVSampleFormat s_sampleFmts[] = {
    // We have to make the conversion to planar float for Resonant anyway,
    // why not let avformat to do it for us
    AV_SAMPLE_FMT_FLTP,
    (AVSampleFormat)-1
  };
}


namespace VideoDisplay
{

  class VideoFrameFfmpeg : public VideoFrame
  {
  public:
    ~VideoFrameFfmpeg()
    {
      if (frame) {
        if (referenced)
          av_frame_unref(frame);
        av_frame_free(&frame);
        frame = nullptr;
      }
      referenced = false;
    }

    bool referenced = false;
    AVFrame* frame = nullptr;
  };

  struct MyAV
  {
  public:
    AVPacket packet;
    AVFrame * frame;

    AVFormatContext * formatContext;

    AVCodecContext * videoCodecContext;
    AVCodec * videoCodec;

    AVCodecContext * audioCodecContext;
    AVCodec * audioCodec;

    int videoStreamIndex;
    int audioStreamIndex;

    double videoTsToSecs;
    double audioTsToSecs;
    int decodedAudioBufferSamples;
    bool needFlushAtEof;
    bool seekByBytes;
    bool seekingSupported;

    double duration;
    double start;
    Nimble::Size videoSize;
  };

  struct PtsCorrectionContext
  {
  public:
    int64_t num_faulty_pts; /// Number of incorrect PTS values so far
    int64_t num_faulty_dts; /// Number of incorrect DTS values so far
    int64_t last_pts;       /// PTS of the last frame
    int64_t last_dts;       /// DTS of the last frame
  };

  struct FilterGraph
  {
  public:
    AVFilterContext * bufferSourceFilter;
    AVFilterContext * bufferSinkFilter;
    AVFilterContext * formatFilter;
    AVFilterGraph * graph;
  };

  // -------------------------------------------------------------------------

  class FfmpegDecoder::D
  {
  public:
    D(FfmpegDecoder* decoder);
    ~D();

    bool initFilters(FilterGraph & FilterGraph, const QString& description,
                     bool video);
    bool open();
    void close();

    void increaseSeekGeneration();
    bool seekToBeginning();
    bool seek();

    QByteArray supportedPixFormatsStr();
    void updateSupportedPixFormats();

    // Partially borrowed from libav / ffplay
    int64_t guessCorrectPts(AVFrame * frame);

    //int64_t videoDpts(AVFrame * frame);
    //int64_t audioDpts(AVFrame * frame);

    bool decodeVideoPacket(double & dpts, double & nextDpts);
    bool decodeAudioPacket(double & dpts, double & nextDpts);
    VideoFrameFfmpeg * getFreeFrame(bool & setTimestampToPts, double & dpts);
    void checkSeek(double & nextVideoDpts, double & videoDpts, double & nextAudioDpts);

    void setFormat(VideoFrameFfmpeg & frame, const AVPixFmtDescriptor & fmtDescriptor,
                   Nimble::Vector2i size);

    // Sets the audio location from m_host->audioLocationAttribute to Resonant audio panner
    void syncAudioLocation();


    FfmpegDecoder * m_host;
    int m_activeSeekGeneration;

    bool m_running;

    MyAV m_av;
    PtsCorrectionContext m_ptsCorrection;

    bool m_realTimeSeeking;
    SeekRequest m_seekRequest;
    double m_exactVideoSeekRequestPts = std::numeric_limits<double>::quiet_NaN();
    double m_exactAudioSeekRequestPts = std::numeric_limits<double>::quiet_NaN();
    // m_activeSeekGeneration will be set to this once the seeking is finished
    int m_seekRequestGeneration = 0;

    AVDecoder::Options m_options;
    Radiant::TimeStamp m_pauseTimestamp;

    QList<AVPixelFormat> m_pixelFormats;

    FilterGraph m_videoFilter;
    FilterGraph m_audioFilter;

    // only used when there is no audio or the audio track has ended
    double m_radiantTimestampToPts;

    double m_loopOffset;

    float m_audioGain;
    bool m_minimiseAudioLatency = false;
    AudioTransferPtr m_audioTransfer;

    /// Change listener ID for m_host->audioLocationAttribute
    long m_audioLocationListener = -1;

    /// In some videos, the audio track might be shorter than the video track
    /// We have some heuristic to determine when the audio track has actually ended,
    /// we really can't rely on some header-information, we just detect when
    /// there are not audio frames coming out from the av packets.
    bool m_audioTrackHasEnded;
    double m_maxAudioDelay;
    double m_lastDecodedAudioPts;
    double m_lastDecodedVideoPts;

    typedef Utils::LockFreeQueue<VideoFrameFfmpeg, 40> DecodedVideoFrames;
    std::unique_ptr<DecodedVideoFrames> m_decodedVideoFrames;

    // Typically we release video frames in releaseOldVideoFrames call, but
    // with certain hardware (Magewell Pro Capture Quad HDMI on Linux)
    // calling unref blocks until a next frame is available. In this case we
    // unreference old used frame at the same time we are referencing a new
    // one. This work-around fixes playback but consumes more memory
    // (one 4k video could consume up to 475MB), so it is not enabled by default.
    bool m_frameUnrefMightBlock = false;

    int m_index;
  };


  // -------------------------------------------------------------------------

  FfmpegDecoder::D::D(FfmpegDecoder *decoder)
    : m_host(decoder),
      m_activeSeekGeneration(0),
      m_running(true),
      m_av(),
      m_ptsCorrection(),
      m_realTimeSeeking(false),
      m_pauseTimestamp(Radiant::TimeStamp::currentTime()),
      m_videoFilter(),
      m_audioFilter(),
      m_radiantTimestampToPts(std::numeric_limits<double>::quiet_NaN()),
      m_loopOffset(0),
      m_audioGain(1),
      m_audioTrackHasEnded(false),
      m_maxAudioDelay(0.3),
      m_lastDecodedAudioPts(std::numeric_limits<double>::quiet_NaN()),
      m_lastDecodedVideoPts(std::numeric_limits<double>::quiet_NaN()),
      m_index(0)
  {
    memset(&m_av, 0, sizeof(m_av));
    m_av.videoStreamIndex = -1;
    m_av.audioStreamIndex = -1;
    m_av.videoSize = Nimble::Size();
  }

  FfmpegDecoder::D::~D()
  {
    AudioTransferPtr tmp(m_audioTransfer);
    if(tmp) {
      if(!tmp->isShutdown())
        Radiant::error("LibavDecover::D::~D # Audio transfer is still active!");
    }
  }

  void FfmpegDecoder::D::updateSupportedPixFormats()
  {
    // Todo see what pixel formats are actually supported
    m_pixelFormats.clear();

    if (m_options.pixelFormat() == VideoFrame::UNKNOWN || m_options.pixelFormat() == VideoFrame::GRAY) {
      m_pixelFormats << AV_PIX_FMT_GRAY8;     ///<        Y        ,  8bpp
    }

    if (m_options.pixelFormat() == VideoFrame::UNKNOWN || m_options.pixelFormat() == VideoFrame::GRAY_ALPHA) {
      m_pixelFormats << AV_PIX_FMT_Y400A;     ///< 8bit gray, 8bit alpha
    }

    if (m_options.pixelFormat() == VideoFrame::UNKNOWN || m_options.pixelFormat() == VideoFrame::RGB) {
      m_pixelFormats << AV_PIX_FMT_BGR24;     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
    }

    if (m_options.pixelFormat() == VideoFrame::UNKNOWN || m_options.pixelFormat() == VideoFrame::RGBA) {
      m_pixelFormats << AV_PIX_FMT_BGRA;      ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
    }

    if (m_options.pixelFormat() == VideoFrame::UNKNOWN || m_options.pixelFormat() == VideoFrame::YUV) {
      m_pixelFormats << AV_PIX_FMT_YUV420P;   ///< planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
      m_pixelFormats << AV_PIX_FMT_YUV422P;   ///< planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
      m_pixelFormats << AV_PIX_FMT_YUV444P;   ///< planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
      m_pixelFormats << AV_PIX_FMT_YUV410P;   ///< planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
      m_pixelFormats << AV_PIX_FMT_YUV411P;   ///< planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
      m_pixelFormats << AV_PIX_FMT_YUVJ420P;  ///< planar YUV 4:2:0, 12bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV420P and setting color_range
      m_pixelFormats << AV_PIX_FMT_YUVJ422P;  ///< planar YUV 4:2:2, 16bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV422P and setting color_range
      m_pixelFormats << AV_PIX_FMT_YUVJ444P;  ///< planar YUV 4:4:4, 24bpp, full scale (JPEG), deprecated in favor of PIX_FMT_YUV444P and setting color_range
      m_pixelFormats << AV_PIX_FMT_YUV440P;   ///< planar YUV 4:4:0 (1 Cr & Cb sample per 1x2 Y samples)
      m_pixelFormats << AV_PIX_FMT_YUVJ440P;  ///< planar YUV 4:4:0 full scale (JPEG), deprecated in favor of PIX_FMT_YUV440P and setting color_range
    }

    if (m_options.pixelFormat() == VideoFrame::UNKNOWN || m_options.pixelFormat() == VideoFrame::YUVA) {
      m_pixelFormats << AV_PIX_FMT_YUVA420P;  ///< planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
      m_pixelFormats << AV_PIX_FMT_YUVA444P;  ///< planar YUV 4:4:4 32bpp, (1 Cr & Cb sample per 1x1 Y & A samples)
      m_pixelFormats << AV_PIX_FMT_YUVA422P;  ///< planar YUV 4:2:2 24bpp, (1 Cr & Cb sample per 2x1 Y & A samples)
    }
  }

  QByteArray FfmpegDecoder::D::supportedPixFormatsStr()
  {
    QByteArray lst;
    for (auto format: m_pixelFormats) {
      const char * str = av_get_pix_fmt_name(format);
      if (!str) {
        Radiant::error("supportedPixFormatsStr # Failed to convert pixel format %d to string", format);
      } else {
        if (!lst.isEmpty())
          lst += "|";
        lst += str;
      }
    }
    return lst;
  }

  bool FfmpegDecoder::D::initFilters(FilterGraph &filterGraph,
                                     const QString &description, bool video)
  {
    QByteArray errorMsg("FfmpegDecoder::D::initFilters # " + m_options.source().toUtf8() +
                        " " + (video ? "video" : "audio") +":");

    AVFilter * buffersrc = nullptr;
    AVFilter * buffersink = nullptr;
    AVFilter * format = nullptr;
    AVFilterInOut * outputs = nullptr;
    AVFilterInOut * inputs  = nullptr;
    int err = 0;

    try {
      buffersrc = avfilter_get_by_name(video ? "buffer" : "abuffer");
      if(!buffersrc) throw "Failed to find filter \"(a)buffer\"";

      buffersink = avfilter_get_by_name(video ? "buffersink" : "abuffersink");
      if(!buffersink) throw "Failed to find filter \"(a)buffersink\"";

      format = avfilter_get_by_name(video ? "format" : "aformat");
      if (!format) throw "Failed to find filter \"(a)format\"";

      filterGraph.graph = avfilter_graph_alloc();
      if(!filterGraph.graph) throw "Failed to allocate filter graph";
      /// Ensure that filters do not spawn threads
      filterGraph.graph->thread_type = 0;

      QString args;
      if(video) {
        AVRational timeBase = av_codec_get_pkt_timebase(m_av.videoCodecContext);
        args.sprintf("video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                     m_av.videoCodecContext->width, m_av.videoCodecContext->height,
                     m_av.videoCodecContext->pix_fmt,
                     timeBase.num, timeBase.den,
                     m_av.videoCodecContext->sample_aspect_ratio.num,
                     m_av.videoCodecContext->sample_aspect_ratio.den);
        int err = avfilter_graph_create_filter(&filterGraph.bufferSourceFilter, buffersrc,
                                               "in", args.toUtf8().data(), nullptr, filterGraph.graph);
        if (err < 0) throw "Failed to create video buffer source";

        err = avfilter_graph_create_filter(&filterGraph.bufferSinkFilter, buffersink,
                                           "out", nullptr, nullptr, filterGraph.graph);
        if (err < 0) throw "Failed to create video buffer sink";

        err = avfilter_graph_create_filter(&filterGraph.formatFilter, format,
                                           "format", supportedPixFormatsStr().data(),
                                           nullptr, filterGraph.graph);
        if (err < 0) throw "Failed to create video format filter";
      } else {
        if(!m_av.audioCodecContext->channel_layout)
          m_av.audioCodecContext->channel_layout = av_get_default_channel_layout(
                m_av.audioCodecContext->channels);

        QByteArray channelLayoutName(255, '\0');
        av_get_channel_layout_string(channelLayoutName.data(), channelLayoutName.size(),
                                     m_av.audioCodecContext->channels, m_av.audioCodecContext->channel_layout);

        AVRational timeBase = av_codec_get_pkt_timebase(m_av.audioCodecContext);
        args.sprintf("time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s",
                     timeBase.num, timeBase.den,
                     m_av.audioCodecContext->sample_rate,
                     av_get_sample_fmt_name(m_av.audioCodecContext->sample_fmt),
                     channelLayoutName.data());
        err = avfilter_graph_create_filter(&filterGraph.bufferSourceFilter, buffersrc,
                                           "in", args.toUtf8().data(), nullptr, filterGraph.graph);
        if(err < 0) throw "Failed to create audio buffer source";

        err = avfilter_graph_create_filter(&filterGraph.bufferSinkFilter, buffersink, "out",
                                           nullptr, nullptr, filterGraph.graph);
        if(err < 0) throw "Failed to create audio buffer sink";

        args.sprintf("sample_fmts=fltp:sample_rates=44100:channel_layouts=%s",
                     m_options.channelLayout().data());
        err = avfilter_graph_create_filter(&filterGraph.formatFilter, format, "format",
                                           args.toUtf8().data(), nullptr, filterGraph.graph);
        if(err < 0) throw "Failed to create audio format filter";
      }

      err = avfilter_link(filterGraph.formatFilter, 0, filterGraph.bufferSinkFilter, 0);
      if (err < 0) throw "Failed to link format filter to buffer sink";

      if(!description.isEmpty()) {
        outputs = avfilter_inout_alloc();
        if(!outputs) throw "Failed to allocate AVFilterInOut";

        inputs  = avfilter_inout_alloc();
        if(!inputs) throw "Failed to allocate AVFilterInOut";

        outputs->name = av_strdup("in");
        outputs->filter_ctx = filterGraph.bufferSourceFilter;
        outputs->pad_idx = 0;
        outputs->next = nullptr;

        inputs->name = av_strdup("out");
        inputs->filter_ctx = filterGraph.formatFilter;
        inputs->pad_idx = 0;
        inputs->next = nullptr;

        err = avfilter_graph_parse2(filterGraph.graph, description.toUtf8().data(),
                                    &inputs, &outputs);
        if(err < 0) throw "Failed to parse filter description";
      } else {
        err = avfilter_link(filterGraph.bufferSourceFilter, 0,
                            filterGraph.formatFilter, 0);
        if(err < 0) throw "Failed to link buffer source and buffer sink";
      }

      err = avfilter_graph_config(filterGraph.graph, nullptr);
      if(err < 0) throw "Graph failed validity test";

      return true;
    } catch (const char * error) {
      if(err < 0) {
        avError(QString("%1 %2").arg(errorMsg.data(), error), err);
      } else {
        Radiant::error("%s %s", errorMsg.data(), error);
      }
      avfilter_graph_free(&filterGraph.graph);
    }
    return false;
  }

  bool FfmpegDecoder::D::open()
  {
    AVInputFormat * inputFormat = nullptr;
    AVDictionary * avoptions = nullptr;

    const QString src(m_options.source());
    const QFileInfo sourceFileInfo(src);

    QByteArray errorMsg("FfmpegDecoder::D::open # " + src.toUtf8() + ":");

    m_exactVideoSeekRequestPts = std::numeric_limits<double>::quiet_NaN();
    m_exactAudioSeekRequestPts = std::numeric_limits<double>::quiet_NaN();

#ifdef RADIANT_LINUX
    /// Detect video4linux2 devices automatically
    if (m_options.format().isEmpty() && AVDecoder::looksLikeV4L2Device(src)) {
      m_options.setFormat("video4linux2");
    }

    m_frameUnrefMightBlock = false;
    if (m_options.format() == "v4l2" || m_options.format() == "video4linux2") {
      // We are just detecting parameters for this device, we don't care about
      // reporting errors, proper error reporting is handled by ffmpeg
      int fd = ::open(src.toUtf8().data(), O_RDWR);
      if (fd >= 0) {
        v4l2_capability cap;
        memset(&cap, 0, sizeof(cap));
        if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
          QByteArray card = (const char*)cap.card;

          // With Magewell Pro Capture Quad (HDMI) cards we ask higher framerate
          // than the normal 25 and also use work-around for issue in av_frame_unref.
          // See also comments for m_frameUnrefMightBlock
          if (card.contains("Pro Capture Quad")) {
            if (!m_options.demuxerOptions().contains("framerate")) {
              m_options.setDemuxerOption("framerate", "60");
            }
            m_frameUnrefMightBlock = true;
          }
        }
        ::close(fd);
      }
    }
#endif

#ifdef RADIANT_WINDOWS
    /// Detect DirectShow devices automatically
    if (m_options.format().isEmpty()) {
      if (src.startsWith("audio=") || src.startsWith("video=")) {
        m_options.setFormat("dshow");
      }
    }

    /// Set audio buffer to 50 ms in DirectShow instead of the default 500 ms.
    /// This will also reduce audio latency by 450 ms, which is important when
    /// using low-latency streaming-mode.
    if (m_options.format() == "dshow" &&
        !m_options.demuxerOptions().contains("audio_buffer_size"))
      m_options.setDemuxerOption("audio_buffer_size", "50");
#endif

    if(!m_options.demuxerOptions().isEmpty()) {
      for(auto it = m_options.demuxerOptions().begin(); it != m_options.demuxerOptions().end(); ++it) {
        int err = av_dict_set(&avoptions, it.key().toUtf8().data(), it.value().toUtf8().data(), 0);
        if(err < 0) {
          Radiant::warning("%s av_dict_set(%s, %s): %d", errorMsg.data(),
                           it.key().toUtf8().data(), it.value().toUtf8().data(), err);
        }
      }
    }

    // If user specified any specific format, try to use that.
    // Otherwise avformat_open_input will just auto-detect the format.
    if(!m_options.format().isEmpty()) {
      inputFormat = av_find_input_format(m_options.format().toUtf8().data());
      if(!inputFormat)
        Radiant::warning("%s Failed to find input format '%s'", errorMsg.data(), m_options.format().toUtf8().data());
    }

    // If source exists, we want to pass it through QFileInfo so Qt resource
    // system paths get dereferenced. Otherwise pass use it directly in case it
    // is a video stream, webcam, or something similar.
    QString openTarget = src;
    if(sourceFileInfo.exists())
      openTarget = sourceFileInfo.absoluteFilePath();

    m_av.formatContext = avformat_alloc_context();

    // Interrupt blocking IO
    AVIOInterruptCB interruptCallback;
    interruptCallback.callback = [] (void * opaque) -> int {
      bool running = *static_cast<bool*>(opaque);
      return running ? 0 : 1;
    };
    interruptCallback.opaque = &m_running;
    m_av.formatContext->interrupt_callback = interruptCallback;

    int err = avformat_open_input(&m_av.formatContext, openTarget.toUtf8().data(),
                                  inputFormat, &avoptions);

    {
      AVDictionaryEntry * it = nullptr;
      while((it = av_dict_get(avoptions, "", it, AV_DICT_IGNORE_SUFFIX))) {
        Radiant::warning("%s Unrecognized demuxer option %s = %s",
                         errorMsg.data(), it->key, it->value);
      }
      av_dict_free(&avoptions);
      avoptions = nullptr;
    }

    if(err != 0) {
      if (m_running)
        avError(QString("%1 Failed to open the source file").arg(errorMsg.data()), err);
      avformat_free_context(m_av.formatContext);
      m_av.formatContext = nullptr;
      return false;
    }

    // Retrieve stream information, avformat processes some stream data, so
    // this might take a while, and it might fail with some files (at least
    // with some mkv files), so we don't abort on error
    err = avformat_find_stream_info(m_av.formatContext, nullptr);
    if(err < 0)
      avError(QString("%1 Failed to find stream info").arg(errorMsg.data()), err);

    if(m_options.isVideoEnabled()) {
      m_av.videoStreamIndex = av_find_best_stream(m_av.formatContext, AVMEDIA_TYPE_VIDEO,
                                                  m_options.videoStreamIndex(), -1,
                                                  &m_av.videoCodec, 0);
      if(m_av.videoStreamIndex < 0) {
        if(m_av.videoStreamIndex == AVERROR_STREAM_NOT_FOUND) {
          Radiant::warning("%s Video stream not found", errorMsg.data());
        } else if(m_av.videoStreamIndex == AVERROR_DECODER_NOT_FOUND) {
          Radiant::error("%s No decoder found for any video stream", errorMsg.data());
        } else {
          Radiant::error("%s Error #%d when trying to find video stream",
                         errorMsg.data(), m_av.videoStreamIndex);
        }
      } else {
        m_av.videoCodecContext = avcodec_alloc_context3(nullptr);
        assert(m_av.videoCodecContext);
        avcodec_parameters_to_context(m_av.videoCodecContext, m_av.formatContext->streams[m_av.videoStreamIndex]->codecpar);
        av_codec_set_pkt_timebase(m_av.videoCodecContext, m_av.formatContext->streams[m_av.videoStreamIndex]->time_base);
        m_av.videoCodecContext->codec_id = m_av.videoCodec->id;
        m_av.videoCodecContext->opaque = this;
        m_av.videoCodecContext->refcounted_frames = 1;
        if (m_options.videoDecodingThreads() <= 0) {
          // Select the thread count automatically.
          // One thread is not enough for 4k videos if you have slow CPU, 4 seems
          // to be too much if you have lots of small videos playing at the same time.
          m_av.videoCodecContext->thread_count = (m_av.videoCodec->capabilities & CODEC_CAP_AUTO_THREADS) ? 0 : 2;
        } else {
          m_av.videoCodecContext->thread_count = m_options.videoDecodingThreads();
        }
      }
    }

    if(m_options.isAudioEnabled()) {
      m_av.audioStreamIndex = av_find_best_stream(m_av.formatContext, AVMEDIA_TYPE_AUDIO,
                                                  m_options.audioStreamIndex(), m_av.videoStreamIndex,
                                                  &m_av.audioCodec, 0);
      if(m_av.audioStreamIndex < 0) {
        if(m_av.audioStreamIndex == AVERROR_STREAM_NOT_FOUND) {
          Radiant::debug("%s Audio stream not found", errorMsg.data());
        } else if(m_av.audioStreamIndex == AVERROR_DECODER_NOT_FOUND) {
          Radiant::error("%s No decoder found for any audio stream", errorMsg.data());
        } else {
          Radiant::error("%s Error #%d when trying to find audio stream",
                         errorMsg.data(), m_av.audioStreamIndex);
        }
      } else {
        m_av.audioCodecContext = avcodec_alloc_context3(nullptr);
        assert(m_av.audioCodecContext);
        avcodec_parameters_to_context(m_av.audioCodecContext, m_av.formatContext->streams[m_av.audioStreamIndex]->codecpar);
        av_codec_set_pkt_timebase(m_av.audioCodecContext, m_av.formatContext->streams[m_av.audioStreamIndex]->time_base);
        m_av.audioCodecContext->codec_id = m_av.audioCodec->id;
        m_av.audioCodecContext->opaque = this;
        m_av.audioCodecContext->thread_count = 1;
        m_av.audioCodecContext->refcounted_frames = 1;
      }
    }

    if(!m_av.videoCodec && !m_av.audioCodec) {
      Radiant::error("%s Didn't open any media streams", errorMsg.data());
      avformat_close_input(&m_av.formatContext);
      return false;
    }

    // Open codecs
    if(m_av.videoCodec) {
      if(!m_options.videoOptions().isEmpty()) {
        for(auto it = m_options.videoOptions().begin(); it != m_options.videoOptions().end(); ++it) {
          int err = av_dict_set(&avoptions, it.key().toUtf8().data(), it.value().toUtf8().data(), 0);
          if(err < 0) {
            Radiant::warning("%s av_dict_set(%s, %s): %d", errorMsg.data(),
                             it.key().toUtf8().data(), it.value().toUtf8().data(), err);
          }
        }
      }

      err = avcodec_open2(m_av.videoCodecContext, m_av.videoCodec, &avoptions);

      {
        AVDictionaryEntry * it = nullptr;
        while((it = av_dict_get(avoptions, "", it, AV_DICT_IGNORE_SUFFIX))) {
          Radiant::warning("%s Unrecognized video codec option %s = %s",
                           errorMsg.data(), it->key, it->value);
        }
        av_dict_free(&avoptions);
        avoptions = nullptr;
      }

      if(err < 0) {
        m_av.videoCodecContext = nullptr;
        m_av.videoCodec = nullptr;
        avError(QString("%1 Failed to open video codec").arg(errorMsg.data()), err);
      }
    }

    if(m_av.audioCodec) {
      if(!m_options.audioOptions().isEmpty()) {
        for(auto it = m_options.audioOptions().begin(); it != m_options.audioOptions().end(); ++it) {
          int err = av_dict_set(&avoptions, it.key().toUtf8().data(), it.value().toUtf8().data(), 0);
          if(err < 0) {
            Radiant::warning("%s av_dict_set(%s, %s): %d", errorMsg.data(),
                             it.key().toUtf8().data(), it.value().toUtf8().data(), err);
          }
        }
      }

      err = avcodec_open2(m_av.audioCodecContext, m_av.audioCodec, &avoptions);

      {
        AVDictionaryEntry * it = nullptr;
        while((it = av_dict_get(avoptions, "", it, AV_DICT_IGNORE_SUFFIX))) {
          Radiant::warning("%s Unrecognized audio codec option %s = %s",
                           errorMsg.data(), it->key, it->value);
        }
        av_dict_free(&avoptions);
        avoptions = nullptr;
      }

      if(err < 0) {
        m_av.audioCodecContext = nullptr;
        m_av.audioCodec = nullptr;
        avError(QString("%1 Failed to open audio codec").arg(errorMsg.data()), err);
      }
    }

    if(!m_av.videoCodec && !m_av.audioCodec) {
      Radiant::error("%s Failed to open any media stream codecs", errorMsg.data());
      avformat_close_input(&m_av.formatContext);
      return false;
    }

    if(m_av.videoCodecContext) {

      bool pixelFormatSupported = false;
      for (auto fmt: m_pixelFormats) {
        if(m_av.videoCodecContext->pix_fmt == fmt) {
          pixelFormatSupported = true;
          break;
        }
      }
      const bool useVideoFilters = !pixelFormatSupported || !m_options.videoFilters().isEmpty();

      if(useVideoFilters)
        initFilters(m_videoFilter, m_options.videoFilters(), true);
    }

    if(m_av.audioCodecContext) {
      if (m_options.channelLayout().isEmpty()) {
        QByteArray channelLayout(255, '\0');
        av_get_channel_layout_string(channelLayout.data(), channelLayout.size(),
                                     m_av.audioCodecContext->channels,
                                     m_av.audioCodecContext->channel_layout);
        m_options.setChannelLayout(channelLayout);
      }

      bool audioFormatSupported = false;
      for(auto it = s_sampleFmts; *it != (AVSampleFormat)-1; ++it) {
        if(m_av.audioCodecContext->sample_fmt == *it) {
          audioFormatSupported = true;
          break;
        }
      }
      /// @todo shouldn't be hard-coded
      const int targetSampleRate = 44100;
      const bool useAudioFilters = !audioFormatSupported ||
          !m_options.audioFilters().isEmpty() ||
          m_av.audioCodecContext->sample_rate != targetSampleRate ||
          m_av.audioCodecContext->channel_layout != av_get_channel_layout(m_options.channelLayout().data());

      if(useAudioFilters)
        initFilters(m_audioFilter, m_options.audioFilters(), false);
    }

    // pts/dts x video/audioTsToSecs == timestamp in seconds
    if(m_av.videoCodecContext) {
      const auto & time_base = m_av.formatContext->streams[m_av.videoStreamIndex]->time_base;
      m_av.videoTsToSecs = time_base.den != 0 ? av_q2d(time_base) :
                                              av_q2d(m_av.videoCodecContext->framerate);
    }

    if(m_av.audioCodecContext) {
      const auto & time_base = m_av.formatContext->streams[m_av.audioStreamIndex]->time_base;
      m_av.audioTsToSecs = time_base.den != 0 ? av_q2d(time_base) :
                                              av_q2d(m_av.audioCodecContext->framerate);
    }

    // Size of the decoded audio buffer, in samples (~44100 samples means one second buffer)
    m_av.decodedAudioBufferSamples = m_av.audioCodecContext ?
          m_options.audioBufferSeconds() * m_av.audioCodecContext->sample_rate : 0;

    m_av.needFlushAtEof = (m_av.audioCodec && (m_av.audioCodec->capabilities & AV_CODEC_CAP_DELAY)) ||
        (m_av.videoCodec && (m_av.videoCodec->capabilities & AV_CODEC_CAP_DELAY));

    // We seek by bytes only if the input file has timestamp discontinuities
    // (seeking by timestamp doesn't really make sense in that case). If the
    // format doesn't support byte seek, we still use timestamp seeking as a
    // fallback, and then just hope for the best.
    m_av.seekByBytes = (m_av.formatContext->iformat->flags & AVFMT_TS_DISCONT) &&
        !(m_av.formatContext->iformat->flags & AVFMT_NO_BYTE_SEEK);

    /// @todo can seeking be supported even if format context doesn't have an IO Context?
    m_av.seekingSupported = m_av.formatContext->pb && m_av.formatContext->pb->seekable;

    av_init_packet(&m_av.packet);

    m_av.frame = av_frame_alloc();
    if(!m_av.frame) {
      Radiant::error("%s Failed to allocate new AVFrame", errorMsg.data());
      close();
      return false;
    }

    if(m_av.audioCodec) {
      int channelLayout = av_get_channel_layout(m_options.channelLayout());
      AudioTransferPtr audioTransfer = std::make_shared<AudioTransfer>(m_host, av_get_channel_layout_nb_channels(channelLayout));

      m_audioTransfer = audioTransfer;
      audioTransfer->setGain(m_audioGain);
      audioTransfer->setMinimizeLatency(m_minimiseAudioLatency);
      audioTransfer->setSeekGeneration(m_activeSeekGeneration);
      audioTransfer->setPlayMode(m_options.playMode());

      static QAtomicInt counter;
      int value = counter.fetchAndAddRelease(1);
      QString id;
      if (src.size() > 50) {
        id = "..." + src.right(47);
      } else {
        id = src;
      }
      audioTransfer->setId(QString("%2 %1").arg(value).arg(id).toUtf8());

      auto item = std::make_shared<Resonant::DSPNetwork::Item>();
      item->setModule(audioTransfer);
      item->setTargetChannel(0);

      Resonant::DSPNetwork::instance()->addModule(item);
    }

    if(m_av.videoCodecContext) {
      /// @todo sometimes it might be possible to get invalid size here if the
      /// first video packet is too far in the stream or it takes very long
      /// time to decode it (there is opening timeout in libav). If this happens,
      /// we should skip the whole header-ready event and continue decoding the
      /// stream normally. After the first video frame is decoded, size should
      /// be updated and then the events should be triggered
      m_av.videoSize = Nimble::Size(m_av.videoCodecContext->width, m_av.videoCodecContext->height);
    } else {
      m_av.videoSize = Nimble::Size();
    }
    m_av.duration = m_av.formatContext->duration / double(AV_TIME_BASE);
    m_av.start = std::numeric_limits<double>::quiet_NaN();

    m_decodedVideoFrames.reset(new DecodedVideoFrames());

    return true;
  }



  void FfmpegDecoder::D::close()
  {
    m_av.duration = 0;
    m_av.videoSize = Nimble::Size();

    if (m_videoFilter.graph)
      avfilter_graph_free(&m_videoFilter.graph);
    if (m_audioFilter.graph)
      avfilter_graph_free(&m_audioFilter.graph);

    // Close the codecs
    if (m_av.audioCodecContext)
      avcodec_free_context(&m_av.audioCodecContext);
    if (m_av.videoCodecContext)
      avcodec_free_context(&m_av.videoCodecContext);

    m_decodedVideoFrames.reset();
    av_frame_free(&m_av.frame);

    // Close the video file
    if(m_av.formatContext)
      avformat_close_input(&m_av.formatContext);

    m_av.videoCodec = nullptr;
    m_av.audioCodec = nullptr;

    AudioTransferPtr audioTransfer(m_audioTransfer);
    m_audioTransfer.reset();
    if(audioTransfer) {
      audioTransfer->shutdown();
      Resonant::DSPNetwork::instance()->markDone(audioTransfer);
    }
  }

  bool FfmpegDecoder::D::seekToBeginning()
  {
    int err = 0;
    if(m_av.seekingSupported) {
      if(m_av.seekByBytes) {
        err = avformat_seek_file(m_av.formatContext, -1,
                                 std::numeric_limits<int64_t>::min(), 0,
                                 std::numeric_limits<int64_t>::max(),
                                 AVSEEK_FLAG_BYTE);
      } else {
        int64_t pos = m_av.formatContext->start_time == (int64_t) AV_NOPTS_VALUE
            ? 0 : m_av.formatContext->start_time;
        err = avformat_seek_file(m_av.formatContext, -1,
                                 std::numeric_limits<int64_t>::min(), pos,
                                 std::numeric_limits<int64_t>::max(), 0);
      }
      if(err < 0) {
        avError(QString("LibavDecoder::D::seekToBeginning # %1: Seek error, re-opening the stream").arg(m_options.source()), err);
        close();
        return open();
      } else {
        if(m_av.audioCodecContext)
          avcodec_flush_buffers(m_av.audioCodecContext);
        if(m_av.videoCodecContext)
          avcodec_flush_buffers(m_av.videoCodecContext);
        m_audioTrackHasEnded = false;
        m_lastDecodedAudioPts = std::numeric_limits<double>::quiet_NaN();
        m_lastDecodedVideoPts = std::numeric_limits<double>::quiet_NaN();
      }
    } else {
      // If we want to loop, but there is no way to seek, we just close
      // and re-open the stream
      close();
      return open();
    }
    return true;
  }

  void FfmpegDecoder::D::increaseSeekGeneration()
  {
    if (m_seekRequestGeneration > m_activeSeekGeneration) {
      m_activeSeekGeneration = m_seekRequestGeneration;
    } else {
      ++m_activeSeekGeneration;
      m_seekRequestGeneration = m_activeSeekGeneration;
    }
    AudioTransferPtr audioTransfer(m_audioTransfer);
    if(audioTransfer)
      audioTransfer->setSeekGeneration(m_activeSeekGeneration);
    m_radiantTimestampToPts = std::numeric_limits<double>::quiet_NaN();
    if(m_options.playMode() == PAUSE)
      m_pauseTimestamp = Radiant::TimeStamp::currentTime();
  }

  bool FfmpegDecoder::D::seek()
  {
    QByteArray errorMsg("FfmpegDecoder::D::seek # " + m_options.source().toUtf8() + ":");

    m_exactVideoSeekRequestPts = std::numeric_limits<double>::quiet_NaN();
    m_exactAudioSeekRequestPts = std::numeric_limits<double>::quiet_NaN();

    if(m_seekRequest.value() <= std::numeric_limits<double>::epsilon()) {
      bool ok = seekToBeginning();
      if(ok)
        increaseSeekGeneration();
      return ok;
    }

    if(!m_av.seekingSupported)
      return false;

    bool seekByBytes = m_av.seekByBytes || m_seekRequest.type() == SEEK_BY_BYTES;

    if(m_seekRequest.type() == SEEK_BY_BYTES &&
       (m_av.formatContext->iformat->flags & AVFMT_NO_BYTE_SEEK)) {
      Radiant::error("%s Seek failed, media doesn't support byte seeking",
                     errorMsg.data());
      return false;
    }

    int64_t pos = 0;
    if(!seekByBytes) {
      if(m_seekRequest.type() == SEEK_BY_SECONDS) {
        pos = m_seekRequest.value() * AV_TIME_BASE;
        if (m_seekRequest.flags() & SEEK_FLAG_ACCURATE) {
          m_exactVideoSeekRequestPts = m_exactAudioSeekRequestPts = m_seekRequest.value();
        }
      } else {
        assert(m_seekRequest.type() == SEEK_RELATIVE);
        if(m_av.formatContext->duration > 0) {
          pos = m_seekRequest.value() * m_av.formatContext->duration;
        } else {
          if(m_av.formatContext->iformat->flags & AVFMT_NO_BYTE_SEEK) {
            Radiant::error("%s Seek failed, couldn't get the content duration"
                           " and the media doesn't support byte seeking",
                           errorMsg.data());
            return false;
          }
          seekByBytes = true;
        }
      }
      if(m_av.formatContext->start_time != (int64_t) AV_NOPTS_VALUE)
          pos += m_av.formatContext->start_time;
    }

    if(seekByBytes) {
      if(m_seekRequest.type() == SEEK_BY_BYTES) {
        pos = m_seekRequest.value();
      } else if(m_seekRequest.type() == SEEK_BY_SECONDS) {
        int64_t size = avio_size(m_av.formatContext->pb);
        if(m_av.formatContext->duration <= 0 || size <= 0) {
          Radiant::error("%s Seek failed, couldn't get the media duration/size",
                         errorMsg.data());
          return false;
        }
        // This is just a guess, since there is no byte size and time 1:1 mapping
        pos = size * m_seekRequest.value() / m_av.duration;

      } else {
        assert(m_seekRequest.type() == SEEK_RELATIVE);
        int64_t size = avio_size(m_av.formatContext->pb);
        if(size <= 0) {
          Radiant::error("%s Seek failed, couldn't get the media size",
                         errorMsg.data());
          return false;
        }
        pos = m_seekRequest.value() * size;
      }
    }

    int64_t minTs = 0, maxTs = std::numeric_limits<int64_t>::max();
    if (m_seekRequest.flags() & SEEK_FLAG_FORWARD) {
      minTs = pos;
    } else {
      maxTs = pos;
    }
    int err = avformat_seek_file(m_av.formatContext, -1, minTs, pos, maxTs,
                                 seekByBytes ? AVSEEK_FLAG_BYTE : AVSEEK_FLAG_BACKWARD);
    if(err < 0) {
      Radiant::error("%s Seek failed", errorMsg.data());
      return false;
    }

    if(m_av.audioCodecContext)
      avcodec_flush_buffers(m_av.audioCodecContext);
    if(m_av.videoCodecContext)
      avcodec_flush_buffers(m_av.videoCodecContext);
    increaseSeekGeneration();
    m_audioTrackHasEnded = false;
    m_lastDecodedAudioPts = std::numeric_limits<double>::quiet_NaN();
    m_lastDecodedVideoPts = std::numeric_limits<double>::quiet_NaN();

    return true;
  }

  VideoFrameFfmpeg * FfmpegDecoder::D::getFreeFrame(bool &setTimestampToPts, double &dpts)
  {
    AudioTransferPtr audioTransfer(m_audioTransfer);

    while(m_running && m_decodedVideoFrames) {
      VideoFrameFfmpeg * frame = m_decodedVideoFrames->takeFree();
      if(frame) return frame;
      // Set this here, because another frame might be waiting for us
      // However, if we have a filter that changes pts, this might not be right.
      if(Nimble::Math::isNAN(m_radiantTimestampToPts)) {
        const Radiant::TimeStamp now = Radiant::TimeStamp::currentTime();
        m_radiantTimestampToPts = dpts + m_loopOffset - now.secondsD() + 4.0/60.0;
        setTimestampToPts = true;
      }
      if(!m_running) break;
      // if the video buffer is full, and audio buffer is almost empty,
      // we need to resize the video buffer, otherwise we could starve.
      // Growing the video buffer is safe, as long as the buffer size
      // doesn't grow over the hard-limit (setSize checks that)
      if(audioTransfer && audioTransfer->bufferStateSeconds() < m_options.audioBufferSeconds() * 0.15f) {
        if(m_decodedVideoFrames->setSize(m_decodedVideoFrames->size() + 1)) {
          m_options.setVideoBufferFrames(m_decodedVideoFrames->size());
          continue;
        }
      }

      if (m_seekRequest.type() != SEEK_NONE) break;

      // If we are not ready, then nobody is also releasing old decoded video frames
      if (m_host->state() != STATE_READY) {
        if (m_host->releaseOldVideoFrames(Timestamp(0, m_activeSeekGeneration)) > 0)
          continue;
      }

      Radiant::Sleep::sleepSome(0.01);
    }
    return nullptr;
  }

  void FfmpegDecoder::D::setFormat(VideoFrameFfmpeg & frame, const AVPixFmtDescriptor & fmtDescriptor,
                                     Nimble::Vector2i size)
  {
    // not exactly true for all formats, but it is true for all formats that we support
    frame.setPlanes((fmtDescriptor.flags & AV_PIX_FMT_FLAG_PLANAR) ? fmtDescriptor.nb_components : 1);

    if(fmtDescriptor.nb_components == 1)
      frame.setFormat(VideoFrame::GRAY);
    else if(fmtDescriptor.nb_components == 2)
      frame.setFormat(VideoFrame::GRAY_ALPHA);
    else if(fmtDescriptor.nb_components == 3 && (fmtDescriptor.flags & AV_PIX_FMT_FLAG_RGB))
      frame.setFormat(VideoFrame::RGB);
    else if(fmtDescriptor.nb_components == 3)
      frame.setFormat(VideoFrame::YUV);
    else if(fmtDescriptor.nb_components == 4 && (fmtDescriptor.flags & AV_PIX_FMT_FLAG_RGB))
      frame.setFormat(VideoFrame::RGBA);
    else if(fmtDescriptor.nb_components == 4)
      frame.setFormat(VideoFrame::YUVA);
    else {
      frame.setFormat(VideoFrame::UNKNOWN);
      frame.setPlanes(0);
    }

    for(int i = 0; i < frame.planes(); ++i) {
      frame.setPlaneSize(i, size);
      if((frame.format() == VideoFrame::YUV || frame.format() == VideoFrame::YUVA) && (i == 1 || i == 2)) {
        frame.setPlaneSize(i, Nimble::Vector2i(
              -((-size.x) >> fmtDescriptor.log2_chroma_w),
              -((-size.y) >> fmtDescriptor.log2_chroma_h)));
      }
      frame.setLineSize(i, 0);
      frame.setData(i, nullptr);
    }
    for(int i = frame.planes(); i < 4; ++i)
      frame.clear(i);
  }

  void FfmpegDecoder::D::syncAudioLocation()
  {
    AudioTransferPtr audioTransfer(m_audioTransfer);
    if (audioTransfer) {
      char buf[128];

      Radiant::BinaryData control;

      control.writeString("panner/setsourcelocation");

      snprintf(buf, sizeof(buf), "%s-%d", audioTransfer->id().data(), (int) 0);

      control.writeString(buf);
      control.writeVector2Float32(m_host->audioLocationAttribute()); // sound source location

      Resonant::DSPNetwork::instance()->send(control);
    }
  }

  int64_t FfmpegDecoder::D::guessCorrectPts(AVFrame* frame)
  {
    int64_t reordered_pts = frame->pkt_pts;
    int64_t dts = frame->pkt_dts;
    int64_t pts = AV_NOPTS_VALUE;


    if (dts != (int64_t) AV_NOPTS_VALUE) {
      m_ptsCorrection.num_faulty_dts += dts <= m_ptsCorrection.last_dts;
      m_ptsCorrection.last_dts = dts;
    }
    if (reordered_pts != (int64_t) AV_NOPTS_VALUE) {
      m_ptsCorrection.num_faulty_pts += reordered_pts <= m_ptsCorrection.last_pts;
      m_ptsCorrection.last_pts = reordered_pts;
    }
    if ((m_ptsCorrection.num_faulty_pts<=m_ptsCorrection.num_faulty_dts || dts == (int64_t) AV_NOPTS_VALUE)
        && reordered_pts != (int64_t) AV_NOPTS_VALUE)
      pts = reordered_pts;
    else
      pts = dts;

    if (pts == (int64_t) AV_NOPTS_VALUE)
      pts = frame->pts;

    if (pts == (int64_t) AV_NOPTS_VALUE)
      pts = frame->pkt_pts;

    return pts;
  }

  bool FfmpegDecoder::D::decodeVideoPacket(double &dpts, double &nextDpts)
  {
    const double maxPtsReorderDiff = 0.1;
    const double prevDpts = dpts;
    dpts = std::numeric_limits<double>::quiet_NaN();

    int gotPicture = 0;
    av_frame_unref(m_av.frame);
    int err = avcodec_decode_video2(m_av.videoCodecContext, m_av.frame, &gotPicture, &m_av.packet);
    if(err < 0) {
      avError(QString("FfmpegDecoder::D::decodeVideoPacket # %1: Failed to decode a video frame").
              arg(m_options.source()), err);
      return false;
    }

    if(!gotPicture)
      return false;

    int64_t pts = guessCorrectPts(m_av.frame);

    /// Some mpeg2 streams don't give valid pts value for the last frame,
    /// guess the value from the previous frame.
    dpts = pts == AV_NOPTS_VALUE ? nextDpts : m_av.videoTsToSecs * pts;

    bool setTimestampToPts = false;

    VideoFrameFfmpeg * frame = nullptr;

    if(m_videoFilter.graph) {
      int err = av_buffersrc_add_frame(m_videoFilter.bufferSourceFilter, m_av.frame);

      if(err < 0) {
        avError(QString("FfmpegDecoder::D::decodeVideoPacket # %1: av_buffersrc_add_ref/av_buffersrc_write_frame failed").
                arg(m_options.source()), err);
      } else {
        bool skip = false;
        while (true) {
          err = av_buffersink_get_frame(m_videoFilter.bufferSinkFilter, m_av.frame);
          if (err == AVERROR(EAGAIN) || err == AVERROR_EOF) {
            if (skip)
              return false;
            break;
          }
          if (err < 0) {
            avError(QString("FfmpegDecoder::D::decodeVideoPacket # %1: av_buffersink_read failed").
                    arg(m_options.source()), err);
            break;
          }

          /// AVFrame->pts should be AV_NOPTS_VALUE if not defined,
          /// but some filters just set it always to zero
          if (m_av.frame->pts != (int64_t) AV_NOPTS_VALUE && m_av.frame->pts != 0) {
            pts = m_av.frame->pts;
            dpts = m_av.videoTsToSecs * m_av.frame->pts;
          }

          if (std::isfinite(m_exactVideoSeekRequestPts)) {
            if (dpts < m_exactVideoSeekRequestPts) {
              skip = true;
              continue;
            }
            m_exactVideoSeekRequestPts = std::numeric_limits<double>::quiet_NaN();
          }

          frame = getFreeFrame(setTimestampToPts, dpts);

          if(!frame)
            return false;

          if(!frame->frame) {
            frame->frame = av_frame_alloc();
          } else if (frame->referenced) {
            av_frame_unref(frame->frame);
          }

          av_frame_ref(frame->frame, m_av.frame);
          frame->referenced = true;

          frame->setIndex(m_index++);

          auto fmtDescriptor = av_pix_fmt_desc_get(AVPixelFormat(frame->frame->format));
          setFormat(*frame, *fmtDescriptor, Nimble::Vector2i(frame->frame->width, frame->frame->height));
          for (int i = 0; i < frame->planes(); ++i) {
            frame->setLineSize(i, frame->frame->linesize[i]);
            frame->setData(i, frame->frame->data[i]);
          }

          frame->setImageSize(Nimble::Vector2i(frame->frame->width, frame->frame->height));
          frame->setTimestamp(Timestamp(dpts + m_loopOffset, m_activeSeekGeneration));

          VideoFrameFfmpeg * lastReadyFrame = m_decodedVideoFrames->lastReadyItem();
          if (lastReadyFrame && lastReadyFrame->timestamp().seekGeneration() == frame->timestamp().seekGeneration() &&
              lastReadyFrame->timestamp().pts()-maxPtsReorderDiff > frame->timestamp().pts()) {
            // There was a problem with the stream, previous frame had larger timestamp than this
            // frame, that should be newer. This must be broken stream or concatenated MPEG file
            // or something similar. We treat this like it was a seek request
            // On some files there are some individual frames out-of-order, we try to minimize
            // this by allowing maximum difference of maxPtsReorderDiff
            /// @todo we probably also want to check if frame.pts is much larger than previous_frame.pts
            increaseSeekGeneration();
            frame->setTimestamp(Timestamp(dpts + m_loopOffset, m_activeSeekGeneration));
            setTimestampToPts = false;
          }
          m_decodedVideoFrames->put();
        }
      }
    } else {
      if (std::isfinite(m_exactVideoSeekRequestPts)) {
        if (dpts < m_exactVideoSeekRequestPts) {
          return false;
        }
        m_exactVideoSeekRequestPts = std::numeric_limits<double>::quiet_NaN();
      }

      frame = getFreeFrame(setTimestampToPts, dpts);
      if(!frame)
        return false;

      if(!frame->frame) {
        frame->frame = av_frame_alloc();
      } else if (frame->referenced) {
        av_frame_unref(frame->frame);
      }

      frame->setIndex(m_index++);

      av_frame_ref(frame->frame, m_av.frame);
      frame->referenced = true;

      /// Copy properties to VideoFrame, that acts as a proxy to AVFrame

      auto fmtDescriptor = av_pix_fmt_desc_get(AVPixelFormat(frame->frame->format));

      int planes = (fmtDescriptor->flags & AV_PIX_FMT_FLAG_PLANAR) ? fmtDescriptor->nb_components : 1;

      assert(frame->frame->format == m_av.frame->format);
      assert(frame->frame->width == m_av.frame->width);
      assert(frame->frame->height == m_av.frame->height);
      for(int i = 0; i < planes; ++i) {
        assert(frame->frame->linesize[i] == m_av.frame->linesize[i]);
        assert(frame->frame->data[i] == m_av.frame->data[i]);
      }

      setFormat(*frame, *fmtDescriptor, Nimble::Vector2i(m_av.frame->width, m_av.frame->height));
      for(int i = 0; i < frame->planes(); ++i) {
        frame->setLineSize(i, frame->frame->linesize[i]);
        frame->setData(i, frame->frame->data[i]);
      }

      frame->setImageSize(Nimble::Vector2i(m_av.frame->width, m_av.frame->height));
      frame->setTimestamp(Timestamp(dpts + m_loopOffset, m_activeSeekGeneration));

      VideoFrameFfmpeg * lastReadyFrame = m_decodedVideoFrames->lastReadyItem();
      if (lastReadyFrame && lastReadyFrame->timestamp().seekGeneration() == frame->timestamp().seekGeneration() &&
          lastReadyFrame->timestamp().pts()-maxPtsReorderDiff > frame->timestamp().pts()) {
        increaseSeekGeneration();
        frame->setTimestamp(Timestamp(dpts + m_loopOffset, m_activeSeekGeneration));
        setTimestampToPts = false;
      }
      m_decodedVideoFrames->put();
    }

    // Normally av.packet.duration can't be trusted
    if(Nimble::Math::isNAN(prevDpts)) {
      nextDpts = m_av.videoTsToSecs * (m_av.packet.duration + pts);
    } else {
      nextDpts = dpts + (dpts - prevDpts);
    }

    if(Nimble::Math::isNAN(m_radiantTimestampToPts) || setTimestampToPts) {
      const Radiant::TimeStamp now = Radiant::TimeStamp::currentTime();
      m_radiantTimestampToPts = dpts + m_loopOffset - now.secondsD() + 4.0/60.0;
    }

    return true;
  }

  bool FfmpegDecoder::D::decodeAudioPacket(double &dpts, double &nextDpts)
  {
    AVPacket packet = m_av.packet;
    bool gotFrames = false;
    bool flush = packet.size == 0;
    AudioTransferPtr audioTransfer(m_audioTransfer);

    while(m_running && (packet.size > 0 || flush)) {
      int gotFrame = 0;
      av_frame_unref(m_av.frame);
      const int consumedBytes = avcodec_decode_audio4(m_av.audioCodecContext, m_av.frame,
                                                &gotFrame, &packet);
      if(consumedBytes < 0) {
        avError(QString("LibavDecoder::D::decodeAudioPacket # %1: Audio decoding error").
                arg(m_options.source()), consumedBytes);
        break;
      }

      if(gotFrame) {
        int64_t pts = guessCorrectPts(m_av.frame);

        dpts = m_av.audioTsToSecs * pts;
        nextDpts = dpts + double(m_av.frame->nb_samples) / m_av.frame->sample_rate;

        DecodedAudioBuffer * decodedAudioBuffer = nullptr;

        if (std::isfinite(m_exactAudioSeekRequestPts)) {
          if (dpts < m_exactAudioSeekRequestPts) {
            packet.data += consumedBytes;
            packet.size -= consumedBytes;
            continue;
          }
          m_exactAudioSeekRequestPts = std::numeric_limits<double>::quiet_NaN();
        }

        gotFrames = true;

        if(m_audioFilter.graph) {

          av_buffersrc_add_frame(m_audioFilter.bufferSourceFilter, m_av.frame);
          while (true) {

            int err = av_buffersink_get_frame_flags(m_audioFilter.bufferSinkFilter, m_av.frame, 0);
            if (err == AVERROR(EAGAIN) || err == AVERROR_EOF)
              break;

            if (err < 0) {
              avError(QString("LibavDecoder::D::decodeAudioPacket # %1: av_buffersink_read failed").
                      arg(m_options.source()), err);
              break;
            }


            while(true) {
              decodedAudioBuffer = audioTransfer->takeFreeBuffer(
                    m_av.decodedAudioBufferSamples - m_av.frame->nb_samples);
              if(decodedAudioBuffer) break;
              if(!m_running) return gotFrames;

              if (m_seekRequest.type() != SEEK_NONE) return false;

              Radiant::Sleep::sleepSome(0.01);
              // Make sure that we don't get stuck with a file that doesn't
              // have video frames in the beginning
              audioTransfer->setEnabled(true);
            }

            /// This used to work in ffmpeg, in libav this pts has some weird values after seeking
            /// @todo should we care?
            /*if(output->pts != (int64_t) AV_NOPTS_VALUE) {
              pts = output->pts;
              dpts = av.audioTsToSecs * output->pts;
              nextDpts = dpts + double(output->audio->nb_samples) / output->audio->sample_rate;
            }*/

            int64_t channel_layout = av_frame_get_channel_layout(m_av.frame);
            decodedAudioBuffer->fillPlanar(Timestamp(dpts + m_loopOffset, m_activeSeekGeneration),
                                         av_get_channel_layout_nb_channels(channel_layout),
                                         m_av.frame->nb_samples, (const float **)(m_av.frame->data));
            audioTransfer->putReadyBuffer(m_av.frame->nb_samples);
          }

        } else {
          /// @todo verify that this branch will actually work with AV_SAMPLE_FMT_FLTP
          /// as the only case we end up here is when AudioCodecContext has
          /// AV_SAMPLE_FMT_FLTP as sample_fmt

          while(true) {
            decodedAudioBuffer = audioTransfer->takeFreeBuffer(
                  m_av.decodedAudioBufferSamples - m_av.frame->nb_samples);
            if(decodedAudioBuffer) break;
            if(!m_running) return gotFrames;
            Radiant::Sleep::sleepSome(0.01);
          }

          int samples = m_av.frame->nb_samples;

          decodedAudioBuffer->fill(Timestamp(dpts + m_loopOffset, m_activeSeekGeneration),
                                   m_av.audioCodecContext->channels, samples,
                                   reinterpret_cast<const int16_t *>(m_av.frame->data[0]));
          audioTransfer->putReadyBuffer(samples);
        }
      } else {
        flush = false;
      }
      packet.data += consumedBytes;
      packet.size -= consumedBytes;
    }
    return gotFrames;
  }

  void FfmpegDecoder::D::checkSeek(double &nextVideoDpts, double &videoDpts, double &nextAudioDpts)
  {
    if((m_seekRequest.type() != SEEK_NONE)) {
      if(seek()) {
        m_loopOffset = 0;
        nextVideoDpts = std::numeric_limits<double>::quiet_NaN();
        nextAudioDpts = std::numeric_limits<double>::quiet_NaN();
        videoDpts = std::numeric_limits<double>::quiet_NaN();
      }
      m_seekRequest.setType(SEEK_NONE);
    }
  }


  // -------------------------------------------------------------------------


  FfmpegDecoder::FfmpegDecoder()
    : m_d(new D(this))
  {
    Thread::setName("FfmpegDecoder");

    auto sync = [this] { m_d->syncAudioLocation(); };
    m_d->m_audioLocationListener = audioLocationAttribute().addListener(sync);
 }

  FfmpegDecoder::~FfmpegDecoder()
  {
    close();
    audioLocationAttribute().removeListener(m_d->m_audioLocationListener);
    if(isRunning())
      waitEnd();
    m_d->close();
  }

  AVDecoder::PlayMode FfmpegDecoder::playMode() const
  {
    return m_d->m_options.playMode();
  }

  void FfmpegDecoder::setPlayMode(AVDecoder::PlayMode mode)
  {
    if(m_d->m_options.playMode() == mode)
      return;

    m_d->m_options.setPlayMode(mode);
    AudioTransferPtr audioTransfer(m_d->m_audioTransfer);

    if(audioTransfer)
      audioTransfer->setPlayMode(mode);
    if(mode == PAUSE)
      m_d->m_pauseTimestamp = Radiant::TimeStamp::currentTime();
    if(mode == PLAY)
      m_d->m_radiantTimestampToPts -= m_d->m_pauseTimestamp.sinceSecondsD();
  }

  Timestamp FfmpegDecoder::getTimestampAt(const Radiant::TimeStamp &ts) const
  {
    AudioTransferPtr audioTransfer(m_d->m_audioTransfer);

    /// If we are doing real-time seeking, we don't have a video frame buffer
    /// and we don't care about av-sync, just show the latest frame we have decoded
    if(m_d->m_realTimeSeeking && m_d->m_av.videoCodec && m_d->m_decodedVideoFrames) {
      VideoFrameFfmpeg* frame = m_d->m_decodedVideoFrames->lastReadyItem();
      if(frame)
        return Timestamp(frame->timestamp().pts() + 0.0001, frame->timestamp().seekGeneration());
    }

    /// Normally synchronize video to audio
    if(audioTransfer && !m_d->m_audioTrackHasEnded && audioTransfer->isEnabled()) {
      Timestamp t = audioTransfer->toPts(ts);
      if (t.seekGeneration() == m_d->m_activeSeekGeneration)
        return t;
    }

    if (m_d->m_options.playMode() == PAUSE && m_d->m_decodedVideoFrames) {
      if (!std::isfinite(m_d->m_radiantTimestampToPts)) {
        for (int i = 0; ; ++i) {
          VideoFrameFfmpeg* frame = m_d->m_decodedVideoFrames->readyItem(i);
          if (!frame) break;

          if (frame->timestamp().seekGeneration() < m_d->m_activeSeekGeneration)
            continue;

          return Timestamp(frame->timestamp().pts() + 0.0001, frame->timestamp().seekGeneration());
        }
      } else {
        return Timestamp(m_d->m_pauseTimestamp.secondsD() + m_d->m_radiantTimestampToPts, m_d->m_activeSeekGeneration);
      }
    }

    if(Nimble::Math::isNAN(m_d->m_radiantTimestampToPts))
      return Timestamp();

    return Timestamp(ts.secondsD() + m_d->m_radiantTimestampToPts, m_d->m_activeSeekGeneration);
  }

  Timestamp FfmpegDecoder::latestDecodedVideoTimestamp() const
  {
    if (!m_d->m_decodedVideoFrames)
      return Timestamp();

    VideoFrameFfmpeg* frame = m_d->m_decodedVideoFrames->lastReadyItem();
    if(frame) {
      return frame->timestamp();
    } else {
      return Timestamp();
    }
  }

  VideoFrame* FfmpegDecoder::getFrame(const Timestamp &ts, ErrorFlags &errors) const
  {
    VideoFrameFfmpeg * ret = nullptr;
    for(int i = 0; m_d->m_decodedVideoFrames; ++i) {
      VideoFrameFfmpeg* frame = m_d->m_decodedVideoFrames->readyItem(i);
      if(!frame) break;

      if(frame->timestamp().seekGeneration() < ts.seekGeneration())
        continue;

      if(frame->timestamp().pts() > ts.pts()) {
        if(ret) return ret;
        return frame;
      }
      if(frame->timestamp().pts() == ts.pts()) {
        return frame;
      }
      ret = frame;
    }
    errors |= ERROR_VIDEO_FRAME_BUFFER_UNDERRUN;
    return ret;
  }

  int FfmpegDecoder::releaseOldVideoFrames(const Timestamp &ts, bool *eof)
  {
    if (!m_d->m_decodedVideoFrames)
      return 0;

    int frameIndex = 0;
    for(;; ++frameIndex) {
      VideoFrameFfmpeg * frame = m_d->m_decodedVideoFrames->readyItem(frameIndex);
      if(!frame)
        break;

      if(frame->timestamp().seekGeneration() > ts.seekGeneration() ||
         (frame->timestamp().seekGeneration() == ts.seekGeneration() &&
          frame->timestamp().pts() > ts.pts()))
        break;
    }

    // always keep one frame alive
    --frameIndex;

    for(int i = 0; i < frameIndex; ++i) {
      VideoFrameFfmpeg * frame = m_d->m_decodedVideoFrames->readyItem();
      assert(frame);

      if (frame && frame->referenced && !m_d->m_frameUnrefMightBlock) {
        av_frame_unref(frame->frame);
        frame->referenced = false;
      }

      m_d->m_decodedVideoFrames->next();
    }

    AudioTransferPtr audioTransfer(m_d->m_audioTransfer);

    if(eof) {
      *eof = finished() && (!audioTransfer|| audioTransfer->bufferStateSeconds() <= 0.0f)
          && m_d->m_decodedVideoFrames->itemCount() <= 1;
    }

    return frameIndex;
  }

  Nimble::Matrix4f FfmpegDecoder::yuvMatrix() const
  {
    if(!m_d->m_av.videoCodecContext)
      return Nimble::Matrix4f::IDENTITY;
    /// @todo why does everything look so wrong when using the correct colorspace?
    ///       for now we just force ITU-R BT601-6 (same as SMPTE170M)
    // this should be m_d->av.videoCodecContext->colorspace
    const int colorspace = SWS_CS_SMPTE170M;
    const int * coeffs = sws_getCoefficients(colorspace);
    int l = 16, h = 235;
    if(m_d->m_av.videoCodecContext->color_range == AVCOL_RANGE_JPEG) {
      l = 0;
      h = 255;
    }
    // a and b scale the y value from [l, h] -> [0, 1]
    const float a = 255.0f/(h-l);
    const float b = l/255.0;

    const float c[4] = {  coeffs[0]/65536.0f, -coeffs[2]/65536.0f,
                         -coeffs[3]/65536.0f,  coeffs[1]/65536.0f };

    // Last column transform uv from 0..1 to -0.5..0.5
    return Nimble::Matrix4f(
        a, 0.0f, c[0], -b*a - 0.5f * c[0],
        a, c[1], c[2], -b*a - 0.5f * (c[2]+c[1]),
        a, c[3],    0, -b*a - 0.5f * c[3],
          0,    0,    0, 1);
  }

  void FfmpegDecoder::setAudioGain(float gain)
  {
    m_d->m_audioGain = gain;
    AudioTransferPtr audioTransfer(m_d->m_audioTransfer);

    if (audioTransfer)
      audioTransfer->setGain(gain);
  }

  void FfmpegDecoder::setMinimizeAudioLatency(bool minimize)
  {
    m_d->m_minimiseAudioLatency = minimize;
    AudioTransferPtr audioTransfer(m_d->m_audioTransfer);

    if (audioTransfer)
      audioTransfer->setMinimizeLatency(minimize);
  }

  void FfmpegDecoder::audioTransferDeleted()
  {
    close();
    if(isRunning())
      waitEnd();

    m_d->m_audioTransfer.reset();
  }

  void FfmpegDecoder::load(const Options &options)
  {
    assert(!isRunning());
    m_d->m_options = options;
    m_d->updateSupportedPixFormats();
    seek(m_d->m_options.seekRequest());
  }

  void FfmpegDecoder::close()
  {
    m_d->m_running = false;
    AudioTransferPtr audioTransfer(m_d->m_audioTransfer);
    // Kill audio so that it stops at the same time as the video
    if (audioTransfer)
      audioTransfer->setGain(0.0f);
  }

  Nimble::Size FfmpegDecoder::videoSize() const
  {
    return m_d->m_av.videoSize;
  }

  bool FfmpegDecoder::isLooping() const
  {
    return m_d->m_options.isLooping();
  }

  void FfmpegDecoder::setLooping(bool doLoop)
  {
    m_d->m_options.setLooping(doLoop);
  }

  double FfmpegDecoder::duration() const
  {
    return m_d->m_av.duration;
  }

  int FfmpegDecoder::seek(const SeekRequest & req)
  {
    int gen = ++m_d->m_seekRequestGeneration;
    m_d->m_seekRequest = req;
    return gen;
  }

  bool FfmpegDecoder::realTimeSeeking() const
  {
    return m_d->m_realTimeSeeking;
  }

  void FfmpegDecoder::setRealTimeSeeking(bool value)
  {
    AudioTransferPtr audioTransfer(m_d->m_audioTransfer);

    m_d->m_realTimeSeeking = value;
    if(audioTransfer)
      audioTransfer->setSeeking(value);
  }

  void FfmpegDecoder::runDecoder()
  {
    QByteArray errorMsg("LibavDecoder::D::runDecoder # " + m_d->m_options.source().toUtf8() + ":");
    QThread::currentThread()->setPriority(QThread::LowPriority);

    QByteArray src = m_d->m_options.source().toUtf8();
    s_src = src.data();

    ffmpegInit();

    if (state() != STATE_FINISHED || !m_d->m_av.videoSize.isValid()) {
      if (!m_d->open()) {
        state() = STATE_ERROR;
        return;
      }
    }
    state() = STATE_HEADER_READY;

    enum EofState {
      Normal,
      Flush,
      Eof
    } eof = EofState::Normal;

    double nextVideoDpts = std::numeric_limits<double>::quiet_NaN();
    double nextAudioDpts = std::numeric_limits<double>::quiet_NaN();
    double videoDpts = std::numeric_limits<double>::quiet_NaN();

    auto & av = m_d->m_av;

    AudioTransferPtr audioTransfer(m_d->m_audioTransfer);

    if (av.videoCodec && audioTransfer)
      audioTransfer->setEnabled(false);

    m_d->m_pauseTimestamp = Radiant::TimeStamp::currentTime();
    bool waitingFrame = false;

    int lastError = 0;
    int consecutiveErrorCount = 0;
    /// With v4l2 streams on some devices (like Inogeni DVI capture cards) lots
    /// of errors in the beginning is normal
    int maxConsecutiveErrors = 50;

    while(m_d->m_running) {
      m_d->m_decodedVideoFrames->setSize(m_d->m_options.videoBufferFrames());

      int err = 0;

      if(!waitingFrame || !m_d->m_realTimeSeeking)
        m_d->checkSeek(nextVideoDpts, videoDpts, nextAudioDpts);

      if(m_d->m_running && m_d->m_realTimeSeeking && av.videoCodec) {
        VideoFrameFfmpeg* frame = m_d->m_decodedVideoFrames->lastReadyItem();
        if(frame && frame->timestamp().seekGeneration() == m_d->m_activeSeekGeneration) {
          /// frame done, give some break for this thread
          Radiant::Sleep::sleepSome(0.001);
          continue;
        }
      }

      if(eof == EofState::Normal) {
        err = av_read_frame(av.formatContext, &av.packet);
      }

      if(err < 0) {
        /// @todo refactor following error handling + heuristics
        ///
        // With streams we might randomly get EAGAIN, at least on linux
        if(err == AVERROR(EAGAIN)) {
          Radiant::Sleep::sleepSome(0.001);
          continue;
        } else
        if(err != AVERROR_EOF) {
          if (err == lastError) {
            if (++consecutiveErrorCount > maxConsecutiveErrors) {
              state() = STATE_ERROR;
              s_src = nullptr;
              return;
            }
          } else {
            avError(QString("%1 Read error").arg(errorMsg.data()), err);
            lastError = err;
          }
          ++consecutiveErrorCount;
          Radiant::Sleep::sleepSome(0.001);
          continue;
        }

        lastError = 0;
        consecutiveErrorCount = 0;

        if(av.needFlushAtEof) {
          eof = EofState::Flush;
        } else {
          eof = EofState::Eof;
        }

      } else {
        lastError = 0;
        consecutiveErrorCount = 0;
      }

      // We really are at the end of the stream and we have flushed all the packages
      if(eof == EofState::Eof) {
        /// @todo refactor eof handling away

        if(m_d->m_realTimeSeeking) {
          Radiant::Sleep::sleepSome(0.001);
          continue;
        }
        if(m_d->m_options.isLooping()) {
          m_d->seekToBeginning();
          eof = EofState::Normal;

          if(!Nimble::Math::isNAN(av.start)) {
            // might be NaN
            // no need to check because the comparision will just be false
            double newDuration = nextVideoDpts - av.start;
            if(newDuration > m_d->m_av.duration) {
              m_d->m_av.duration = newDuration;
            }
            newDuration = nextAudioDpts - av.start;
            if(newDuration > m_d->m_av.duration)
              m_d->m_av.duration = newDuration;
          }

          m_d->m_loopOffset += m_d->m_av.duration;
          continue;
        } else {
          // all done
          break;
        }
      }

      av.frame->opaque = nullptr;
      bool gotVideoFrame = false;
      bool gotAudioFrame = false;
      double audioDpts = std::numeric_limits<double>::quiet_NaN();

      /// todo come up with descriptive name
      bool videoCodec = av.videoCodec;
      bool v1 = videoCodec && eof == EofState::Normal && av.packet.stream_index == av.videoStreamIndex;
      bool v2 = videoCodec && eof == EofState::Flush && (av.videoCodec->capabilities & AV_CODEC_CAP_DELAY);

      if(v1 || v2) {
        if(v2) {
          av_init_packet(&av.packet);
          av.packet.data = nullptr;
          av.packet.size = 0;
          av.packet.stream_index = av.videoStreamIndex;
        }
        gotVideoFrame = m_d->decodeVideoPacket(videoDpts, nextVideoDpts);
        if(gotVideoFrame && audioTransfer)
          audioTransfer->setEnabled(true);
      }

      av.frame->opaque = nullptr;

      /// todo come up with descriptive name
      bool acodec = av.audioCodec;
      bool a1 = acodec && eof == EofState::Normal && av.packet.stream_index == av.audioStreamIndex;
      bool a2 = acodec && eof == EofState::Flush && (av.audioCodec->capabilities & AV_CODEC_CAP_DELAY);
      if(a1 || a2) {
        if(a2) {
          av_init_packet(&av.packet);
          av.packet.data = nullptr;
          av.packet.data = 0;
          av.packet.stream_index = av.audioStreamIndex;
        }
        gotAudioFrame = m_d->decodeAudioPacket(audioDpts, nextAudioDpts);
      }

      const bool gotFrames = gotAudioFrame || gotVideoFrame;

      // Flush is done if there are no more frames
      if(eof == EofState::Flush && !gotFrames)
        eof = EofState::Eof;

      if(Nimble::Math::isNAN(av.start) && gotFrames) {
        if(Nimble::Math::isNAN(videoDpts))
          av.start = audioDpts;
        else if(Nimble::Math::isNAN(audioDpts))
          av.start = videoDpts;
        else
          av.start = std::min(videoDpts, audioDpts);
      }

      waitingFrame = m_d->m_realTimeSeeking && av.videoCodec && !gotFrames;

      av_packet_unref(&av.packet);

      if(gotFrames)
        state() = STATE_READY;

      if (audioTransfer) {
        if (!Nimble::Math::isNAN(audioDpts))
          m_d->m_lastDecodedAudioPts = audioDpts;
        if (!Nimble::Math::isNAN(videoDpts))
          m_d->m_lastDecodedVideoPts = videoDpts;
        double delay = m_d->m_lastDecodedAudioPts - m_d->m_lastDecodedVideoPts;

        // In case of NaN, this will become false
        bool ended = m_d->m_audioTrackHasEnded;
        if (delay < -m_d->m_maxAudioDelay)
          ended = true;
        else if (!Nimble::Math::isNAN(audioDpts))
          ended = false;
        if (m_d->m_audioTrackHasEnded != ended) {
          m_d->m_audioTrackHasEnded = ended;
          if (ended) {
            // Radiant::info("%s Audio/Video decoding delay: %lf, assuming that the audio track has ended", errorMsg.data(), delay);
            m_d->m_radiantTimestampToPts = audioTransfer->toPts(Radiant::TimeStamp(0)).pts();
          } else {
            // Radiant::info("%s Got audio packet, restoring audio sync. Delay: %lf", errorMsg.data(), delay);
            // This is a file specific feature, there seems to be no other way
            // than just guess a better estimate when ever we see that we made a mistake
            m_d->m_maxAudioDelay = std::min(1.4, m_d->m_maxAudioDelay + 0.1);
          }
        }
      }
    }

    state() = STATE_FINISHED;
    s_src = nullptr;

    if (audioTransfer) {
      // Tell audio transfer that there are no more samples coming, so that it
      // knows that it can disable itself when it runs out of the decoded
      // buffer. We can also then know that we shouldn't synchronize remaining
      // video frames to audio anymore after that.
      audioTransfer->setDecodingFinished(true);
    }

    // If m_running is false, someone called AVDecoder::close(), so we can
    // close the decoder here in the decoder thread. Otherwise ~FfmpegDecoder()
    // would need to do it, which might block longer than the user expects.
    // Also this way we can close all decoders in parallel on application
    // shutdown, saving a lot of time, especially with Datapath video sources
    // (those can take 1-2 seconds to close).
    if (!m_d->m_running)
      m_d->close();
  }

  void ffmpegInit()
  {
    MULTI_ONCE {
      av_log_set_callback(libavLog);
      avcodec_register_all();
      avdevice_register_all();
      av_register_all();
      avformat_network_init();
      avfilter_register_all();

      int err = av_lockmgr_register(libavLock);
      if(err != 0)
        Radiant::error("libavInit # Failed to register new Libav lock manager");
    }
  }
}
