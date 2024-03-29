/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

// For PRIx64
#define __STDC_FORMAT_MACROS

#include "FfmpegDecoder.hpp"
#include "FfmpegVideoFormatSelector.hpp"

#include "Utils.hpp"
#include "AudioTransfer.hpp"
#include "VideoCaptureMonitor.hpp"

#include <Nimble/Vector2.hpp>

#include <Radiant/Allocators.hpp>
#include <Radiant/Condition.hpp>
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

# include <libavfilter/avfilter.h>
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
  Radiant::Mutex s_exclusiveAccessMutex;
  std::set<QString> s_exclusiveAccess;

  RADIANT_TLS(const char *) s_src = nullptr;

  thread_local bool s_forceNewestFrame = false;

  RADIANT_TLS(const VideoDisplay::FfmpegDecoder::LogHandler *) s_logHandler = nullptr;

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

    if (s_logHandler && (*s_logHandler)(level, buffer))
      return;

    QString msg = QString("%1: %2").arg((const char*)s_src).arg(buffer);

    if(level >= AV_LOG_DEBUG) {
      Radiant::debug("Video decoder: %s", msg.toUtf8().data());
    } else if(level >= AV_LOG_INFO) {
      Radiant::info("Video decoder: %s", msg.toUtf8().data());
    } else if(level >= AV_LOG_WARNING) {
      Radiant::warning("Video decoder: %s", msg.toUtf8().data());

      /// When decoding RTSP streams, it's possible that we have too many frames
      /// in buffer and that we are not consuming packets fast enough from RTP.
      /// Try to recover from this by clearing the buffer.
      if (msg.contains("max delay reached. need to consume packet"))
        s_forceNewestFrame = true;

    } else if(level >= AV_LOG_ERROR) {
      /// max_analyze_duration and first timestamps "errors" happen with some
      /// files and those situations are handled in our decoder once the first
      /// frame has been decoded and the decoder goes to READY state.
      ///
      /// We don't care about "real-time buffer <device> too full or near too
      /// full (151% of size: 3041280 [rtbufsize parameter])! frame dropped!"
      /// errors. Those mean that we are not reading all frames fast enough
      /// from the dshow graph. This means that we are just probably seeking,
      /// stopping, starting or just rendering at lower framerate than the
      /// video input running. We typically use streaming-mode anyway in
      /// this case, so we are only interested in the latest frame. We don't
      /// care about dropped frames.
      if (!msg.contains("max_analyze_duration reached") &&
          !msg.contains("First timestamp is missing,") &&
          !msg.contains("rtbufsize parameter")) {
        Radiant::error("Video decoder: %s", msg.toUtf8().data());
      }
    } else {
      if (!msg.contains("too full or near too full")) {
        Radiant::error("Video decoder: %s", msg.toUtf8().data());
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
  AVFrameWrapper::~AVFrameWrapper()
  {
    if (avframe) {
      if (referenced)
        av_frame_unref(avframe);
      av_frame_free(&avframe);
      avframe = nullptr;
    }
    referenced = false;
  }

  VideoFrameFfmpeg::~VideoFrameFfmpeg()
  {
    if (frame.avframe) {
      if (!frameUnrefMightBlock && frame.referenced) {
        av_frame_unref(frame.avframe);
        frame.referenced = false;
      }
      if (auto pool = deallocatedFrames.lock()) {
        Radiant::Guard g(pool->mutex);
        pool->frames.push_back(std::move(frame));
      }
    }
  }

  struct MyAV
  {
  public:
    AVPacket packet;
    AVFrame * frame;

    AVFormatContextPtr formatContext;

    AVCodecContext * videoCodecContext;
    AVCodec * videoCodec;

    AVCodecContext * audioCodecContext;
    AVCodec * audioCodec;

    int videoStreamIndex;
    int audioStreamIndex;

    int decodedAudioBufferSamples;
    bool needFlushAtEof;
    bool seekByBytes;
    bool seekingSupported;

    bool hasReliableDuration;
    double duration;
    double start;
    Nimble::Size videoSize;

    int64_t startPts;
    AVRational startPtsTb;

    int64_t nextPts;
    AVRational nextPtsTb;
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

    /// m_decodedVideoFramesMutex needs to be locked before calling either of these functions
    void setSeekGeneration(int generation);
    bool increaseSeekGeneration();

    bool seekToBeginning();
    bool seek(SeekRequest req, int seekRequestGeneration);

    QByteArray supportedPixFormatsStr();
    void updateSupportedPixFormats();

    bool decodeVideoPacket(double & dpts);
    bool decodeAudioPacket(double & dpts);
    std::shared_ptr<VideoFrameFfmpeg> getFreeFrame();
    bool checkSeek();

    void setFormat(VideoFrameFfmpeg & frame, const AVPixFmtDescriptor & fmtDescriptor,
                   Nimble::Vector2i size);

    /// If the source is a video capture device, then only one decoder can be
    /// open at a time. Try to get exclusive access to this source. Often when
    /// you are quickly reloading a video, the old decoder might still be open
    /// and reserving the device when the new decoder is trying to open it.
    bool claimExclusiveAccess(const QString & src, double maxWaitTimeSecs);
    void releaseExclusiveAccess();

    std::shared_ptr<VideoFrameFfmpeg> firstReadyDecodedFrame();
    std::shared_ptr<VideoFrameFfmpeg> lastReadyDecodedFrame();
    std::shared_ptr<VideoFrameFfmpeg> playFrame(Radiant::TimeStamp presentTimestamp,
                                                FfmpegDecoder::ErrorFlags & errors,
                                                FfmpegDecoder::PlayFlags flags);

    FfmpegDecoder * m_host;
    std::shared_ptr<AVSync> m_sync = std::make_shared<AVSync>();
    bool m_hasExternalSync = false;

    bool m_running;

    MyAV m_av;
    PtsCorrectionContext m_ptsCorrection;

    bool m_forceNewestFrame = false;
    bool m_realTimeSeeking;
    SeekRequest m_seekRequest;
    double m_exactVideoSeekRequestPts = std::numeric_limits<double>::quiet_NaN();
    double m_exactAudioSeekRequestPts = std::numeric_limits<double>::quiet_NaN();
    // m_sync activeSeekGeneration will be set to this once the seeking is finished
    int m_seekRequestGeneration = 0;
    /// If this mutex is locked at the same time with m_decodedVideoFramesMutex,
    /// the latter needs to be locked first
    Radiant::Mutex m_seekRequestMutex;

    AVDecoder::Options m_options;

    QList<AVPixelFormat> m_pixelFormats;

    FilterGraph m_videoFilter;
    FilterGraph m_audioFilter;

    // only used when there is no audio or the audio track has ended

    double m_loopOffset;

    float m_audioGain;
    AudioTransferPtr m_audioTransfer;

    Radiant::Mutex m_decodedVideoFramesMutex;
    Radiant::Condition m_decodedVideoFramesCond;
    std::vector<std::shared_ptr<VideoFrameFfmpeg>> m_decodedVideoFrames;

    /// Recycled AVFrames frames from ~VideoFrameFfmpeg. This are needed for
    /// implementing m_frameUnrefMightBlock and also reduces the number of
    /// memory allocations in normal video playback.
    std::shared_ptr<DeallocatedFrames> m_deallocatedFrames{std::make_shared<DeallocatedFrames>()};

    // Typically we release video frames in releaseOldVideoFrames call, but
    // with certain hardware (Magewell Pro Capture Quad HDMI on Linux)
    // calling unref blocks until a next frame is available. In this case we
    // unreference old used frame at the same time we are referencing a new
    // one. This work-around fixes playback but consumes more memory
    // (one 4k video could consume up to 475MB), so it is not enabled by default.
    bool m_frameUnrefMightBlock = false;

    // Some video files report invalid color range, so even if they say they
    // use AVCOL_RANGE_JPEG, we normally still render them with
    // AVCOL_RANGE_MPEG, see https://redmine.multitouch.fi/issues/14426 and
    // https://redmine.multitaction.com/issues/16638.
    // Video capture devices and other local video streams are an exception
    // implemented using this flag.
    bool m_allowJpegRange = false;

    int m_index;

    bool m_hasDecodedAudioFrames = false;

    Radiant::Timer m_decodingStartTime;

    bool m_hasExclusiveAccess = false;
    std::set<QString>::iterator m_exclusiveAccess;
  };


  // -------------------------------------------------------------------------

  static void setMapOptions(const QMap<QString, QString> & input, AVDictionary ** output,
                            const QByteArray & errorMsg)
  {
    for (auto it = input.begin(); it != input.end(); ++it) {
      int err = av_dict_set(output, it.key().toUtf8().data(), it.value().toUtf8().data(), 0);
      if (err < 0 && !errorMsg.isNull()) {
        Radiant::warning("%s av_dict_set(%s, %s): %d", errorMsg.data(),
                         it.key().toUtf8().data(), it.value().toUtf8().data(), err);
      }
    }
  }

  FfmpegDecoder::D::D(FfmpegDecoder *decoder)
    : m_host(decoder),
      m_running(true),
      m_av(),
      m_ptsCorrection(),
      m_realTimeSeeking(false),
      m_videoFilter(),
      m_audioFilter(),
      m_loopOffset(0),
      m_audioGain(1),
      m_index(0)
  {
    memset(static_cast<void*>(&m_av), 0, sizeof(m_av));
    m_av.videoStreamIndex = -1;
    m_av.audioStreamIndex = -1;
    m_av.videoSize = Nimble::Size();
    m_av.startPts = AV_NOPTS_VALUE;
    m_av.nextPts = AV_NOPTS_VALUE;
  }

  FfmpegDecoder::D::~D()
  {
    AudioTransferPtr tmp(m_audioTransfer);
    if(tmp) {
      if(!tmp->isShutdown())
        Radiant::error("FfmpegDecoder::D::~D # Audio transfer is still active!");
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

    AVFilterInOut * outputs = nullptr;
    AVFilterInOut * inputs  = nullptr;
    int err = 0;

    try {
      const AVFilter* buffersrc = avfilter_get_by_name(video ? "buffer" : "abuffer");
      if(!buffersrc) throw "Failed to find filter \"(a)buffer\"";

      const AVFilter* buffersink = avfilter_get_by_name(video ? "buffersink" : "abuffersink");
      if(!buffersink) throw "Failed to find filter \"(a)buffersink\"";

      const AVFilter* format = avfilter_get_by_name(video ? "format" : "aformat");
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

        args.sprintf("time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s",
                     1, m_av.audioCodecContext->sample_rate,
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

    QString src(m_options.source());
    const QFileInfo sourceFileInfo(src);

    QByteArray errorMsg("FfmpegDecoder::D::open # " + src.toUtf8() + ":");

    m_exactVideoSeekRequestPts = std::numeric_limits<double>::quiet_NaN();
    m_exactAudioSeekRequestPts = std::numeric_limits<double>::quiet_NaN();
    m_hasDecodedAudioFrames = false;
    m_allowJpegRange = false;

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

          // With Magewell Pro Capture Quad (HDMI) cards we use work-around for
          // issue in av_frame_unref.
          // See also comments for m_frameUnrefMightBlock
          if (card.contains("Pro Capture Quad"))
            m_frameUnrefMightBlock = true;

          // Datapath capture cards use JPEG color range by default
          if (cap.driver && QByteArray(reinterpret_cast<const char*>(cap.driver)).startsWith("Vision"))
            m_allowJpegRange = true;
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

#ifdef RADIANT_OSX
    /// Detect some AVFoundation devices automatically
    if (m_options.format().isEmpty()) {
      if (src.startsWith("AVFoundation:")) {
        src = src.mid(13);
        m_options.setSource(src);
        m_options.setFormat("avfoundation");
      } else if (QRegExp("\\d+:\\d+").exactMatch(src)) {
        m_options.setFormat("avfoundation");
      }
    }
#endif

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

    const bool isStream = m_options.format() == "dshow" ||
        m_options.format() == "v4l2" ||
        m_options.format() == "video4linux2" ||
        m_options.format() == "avfoundation";

    if (isStream) {
      claimExclusiveAccess(src, 10.0);
    }

    if (isStream) {
      bool skipScanInputFormat = false;
#ifdef RADIANT_WINDOWS
      if (auto monitor = VideoCaptureMonitor::weakInstance().lock()) {
        for (VideoCaptureMonitor::VideoSource vsrc: monitor->sources()) {
          if (vsrc.device == src.toUtf8()) {
            /// Datapath VisionSC-HD4+ cards have issues in Windows when you
            /// scan the input formats. Sometimes there is a side effect that
            /// the same source that is scanned, can't be opened for a while.
            /// We don't need any of these options with this card anyway,
            /// since the card automatically selects all this.
            ///
            /// Datapath capture cards use JPEG color range by default
            if (vsrc.friendlyName.toLower().contains("datapath vision")) {
              skipScanInputFormat = true;
              m_allowJpegRange = true;
            }
            break;
          }
        }
      }
#endif

      if (!skipScanInputFormat && !m_options.demuxerOptions().contains("list_options")) {
        std::vector<VideoInputFormat> formats = scanInputFormats(
              src, inputFormat, m_options.demuxerOptions());

        if (const VideoInputFormat * format = chooseFormat(formats, m_options))
          applyFormatOptions(*format, m_options);
      }
    }

    setMapOptions(m_options.demuxerOptions(), &avoptions, errorMsg);

    AVFormatContext * formatContext = avformat_alloc_context();

    // Interrupt blocking IO
    AVIOInterruptCB interruptCallback;
    interruptCallback.callback = [] (void * opaque) -> int {
      bool running = *static_cast<bool*>(opaque);
      return running ? 0 : 1;
    };
    interruptCallback.opaque = &m_running;
    formatContext->interrupt_callback = interruptCallback;

    // avformat_open_input will delete formatContext on error
    int err = avformat_open_input(&formatContext, openTarget.toUtf8().data(),
                                  inputFormat, &avoptions);
    if(err != 0) {
      if (m_running)
        avError(QString("%1 Failed to open the source file").arg(errorMsg.data()), err);
      av_dict_free(&avoptions);
      releaseExclusiveAccess();
      return false;
    }

    m_av.formatContext = AVFormatContextPtr(formatContext, [hasExclusiveAccess = m_hasExclusiveAccess, ex = m_exclusiveAccess]
                                            (AVFormatContext * self) {
      avformat_close_input(&self);
      if (hasExclusiveAccess) {
        Radiant::Guard g(s_exclusiveAccessMutex);
        s_exclusiveAccess.erase(ex);
      }
    });
    // Exclusive access management was moved to the shared_ptr deleter
    m_hasExclusiveAccess = false;

    {
      AVDictionaryEntry * it = nullptr;
      while((it = av_dict_get(avoptions, "", it, AV_DICT_IGNORE_SUFFIX))) {
        Radiant::warning("%s Unrecognized demuxer option %s = %s",
                         errorMsg.data(), it->key, it->value);
      }
      av_dict_free(&avoptions);
      avoptions = nullptr;
    }

    // Retrieve stream information, avformat processes some stream data, so
    // this might take a while, and it might fail with some files (at least
    // with some mkv files), so we don't abort on error
    err = avformat_find_stream_info(m_av.formatContext.get(), nullptr);
    if(err < 0)
      avError(QString("%1 Failed to find stream info").arg(errorMsg.data()), err);

    if(m_options.isVideoEnabled()) {
      m_av.videoStreamIndex = av_find_best_stream(m_av.formatContext.get(), AVMEDIA_TYPE_VIDEO,
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
          m_av.videoCodecContext->thread_count = (m_av.videoCodec->capabilities & AV_CODEC_CAP_AUTO_THREADS) ? 0 : 2;
        } else {
          m_av.videoCodecContext->thread_count = m_options.videoDecodingThreads();
        }
      }
    }

    if(m_options.isAudioEnabled()) {
      m_av.audioStreamIndex = av_find_best_stream(m_av.formatContext.get(), AVMEDIA_TYPE_AUDIO,
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
      m_av.formatContext.reset();
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

      if(err < 0) {
        m_av.videoCodecContext = nullptr;
        m_av.videoCodec = nullptr;
        avError(QString("%1 Failed to open video codec").arg(errorMsg.data()), err);
      } else {
        AVDictionaryEntry * it = nullptr;
        while((it = av_dict_get(avoptions, "", it, AV_DICT_IGNORE_SUFFIX))) {
          Radiant::warning("%s Unrecognized video codec option %s = %s",
                           errorMsg.data(), it->key, it->value);
        }
      }

      av_dict_free(&avoptions);
      avoptions = nullptr;
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

      if(err < 0) {
        m_av.audioCodecContext = nullptr;
        m_av.audioCodec = nullptr;
        avError(QString("%1 Failed to open audio codec").arg(errorMsg.data()), err);
      } else {
        AVDictionaryEntry * it = nullptr;
        while((it = av_dict_get(avoptions, "", it, AV_DICT_IGNORE_SUFFIX))) {
          Radiant::warning("%s Unrecognized audio codec option %s = %s",
                           errorMsg.data(), it->key, it->value);
        }
      }

      av_dict_free(&avoptions);
      avoptions = nullptr;
    }

    if(!m_av.videoCodec && !m_av.audioCodec) {
      Radiant::error("%s Failed to open any media stream codecs", errorMsg.data());
      m_av.formatContext.reset();
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

    if(m_av.audioCodecContext) {
      auto & as = m_av.formatContext->streams[m_av.audioStreamIndex];
      if ((m_av.formatContext->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) &&
          !m_av.formatContext->iformat->read_seek) {
        m_av.startPts = as->start_time;
        m_av.startPtsTb = as->time_base;
      }
    }

    // Size of the decoded audio buffer, in samples (~44100 samples means one second buffer)
    m_av.decodedAudioBufferSamples = m_av.audioCodecContext ?
          static_cast<int>(m_options.audioBufferSeconds() * m_av.audioCodecContext->sample_rate) : 0;

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
      uint64_t channelLayout = av_get_channel_layout(m_options.channelLayout());
      AudioTransferPtr audioTransfer = std::make_shared<AudioTransfer>(
            m_host, av_get_channel_layout_nb_channels(channelLayout), m_sync);

      m_audioTransfer = audioTransfer;
      audioTransfer->setGain(m_audioGain);

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
    if (m_av.formatContext->duration != AV_NOPTS_VALUE) {
      m_av.duration = m_av.formatContext->duration / double(AV_TIME_BASE);
      m_av.hasReliableDuration = true;
    } else {
      /// m_av.duration will be updated every time we decode a frame, since it
      /// might be needed for looping. However, since we set hasReliableDuration
      /// to false, it makes sure we don't return possible incorrect number
      /// from FfmpegDecoder::duration().
      m_av.duration = 0;
      m_av.hasReliableDuration = false;
    }
    m_av.start = std::numeric_limits<double>::quiet_NaN();

    {
      Radiant::Guard g(m_decodedVideoFramesMutex);
      m_decodedVideoFrames.clear();
    }
    m_decodedVideoFramesCond.wakeAll();

    return true;
  }



  void FfmpegDecoder::D::close()
  {
    m_av.duration = 0;
    m_av.hasReliableDuration = false;
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

    {
      Radiant::Guard g(m_deallocatedFrames->mutex);
      m_deallocatedFrames->frames.clear();
    }

    {
      Radiant::Guard g(m_decodedVideoFramesMutex);
      m_decodedVideoFrames.clear();
    }
    av_frame_free(&m_av.frame);

    // Close the video file
    m_av.formatContext.reset();

    m_av.videoCodec = nullptr;
    m_av.audioCodec = nullptr;

    AudioTransferPtr audioTransfer(m_audioTransfer);
    m_audioTransfer.reset();
    if(audioTransfer) {
      audioTransfer->shutdown();
      Resonant::DSPNetwork::markDone(audioTransfer);
    }
  }

  bool FfmpegDecoder::D::seekToBeginning()
  {
    int err = 0;
    if(m_av.seekingSupported) {
      if(m_av.seekByBytes) {
        err = avformat_seek_file(m_av.formatContext.get(), -1,
                                 std::numeric_limits<int64_t>::min(), 0,
                                 std::numeric_limits<int64_t>::max(),
                                 AVSEEK_FLAG_BYTE);
      } else {
        int64_t pos = m_av.formatContext->start_time == (int64_t) AV_NOPTS_VALUE
            ? 0 : m_av.formatContext->start_time;
        err = avformat_seek_file(m_av.formatContext.get(), -1,
                                 std::numeric_limits<int64_t>::min(), pos,
                                 std::numeric_limits<int64_t>::max(), 0);
      }
      if(err < 0) {
        avError(QString("FfmpegDecoder::D::seekToBeginning # %1: Seek error, re-opening the stream").arg(m_options.source()), err);
        close();
        return open();
      } else {
        if(m_av.audioCodecContext)
          avcodec_flush_buffers(m_av.audioCodecContext);
        if(m_av.videoCodecContext)
          avcodec_flush_buffers(m_av.videoCodecContext);
        m_av.nextPts = m_av.startPts;
        m_av.nextPtsTb = m_av.startPtsTb;
      }
    } else {
      // If we want to loop, but there is no way to seek, we just close
      // and re-open the stream
      close();
      return open();
    }
    return true;
  }

  void FfmpegDecoder::D::setSeekGeneration(int generation)
  {
    m_sync->setSeekGeneration(generation);
    m_decodedVideoFrames.clear();
    m_decodedVideoFramesCond.wakeAll();
  }

  bool FfmpegDecoder::D::increaseSeekGeneration()
  {
    Radiant::Guard g(m_seekRequestMutex);
    if (m_seekRequest.type() != AVDecoder::SEEK_NONE)
      return false;
    setSeekGeneration(m_sync->seekGeneration() + 1);
    return true;
  }

  bool FfmpegDecoder::D::seek(SeekRequest req, int seekRequestGeneration)
  {
    QByteArray errorMsg("FfmpegDecoder::D::seek # " + m_options.source().toUtf8() + ":");

    m_exactVideoSeekRequestPts = std::numeric_limits<double>::quiet_NaN();
    m_exactAudioSeekRequestPts = std::numeric_limits<double>::quiet_NaN();

    if(req.value() <= std::numeric_limits<double>::epsilon()) {
      bool ok = seekToBeginning();
      if(ok) {
        Radiant::Guard g(m_decodedVideoFramesMutex);
        setSeekGeneration(seekRequestGeneration);
      }
      return ok;
    }

    if(!m_av.seekingSupported)
      return false;

    bool seekByBytes = m_av.seekByBytes || req.type() == SEEK_BY_BYTES;

    if(req.type() == SEEK_BY_BYTES &&
       (m_av.formatContext->iformat->flags & AVFMT_NO_BYTE_SEEK)) {
      Radiant::error("%s Seek failed, media doesn't support byte seeking",
                     errorMsg.data());
      return false;
    }

    int64_t pos = 0;
    if(!seekByBytes) {
      if(req.type() == SEEK_BY_SECONDS) {
        pos = static_cast<int64_t>(req.value() * AV_TIME_BASE);
        if (req.flags() & SEEK_FLAG_ACCURATE) {
          m_exactVideoSeekRequestPts = m_exactAudioSeekRequestPts = req.value();
        }
      } else {
        assert(req.type() == SEEK_RELATIVE);
        if(m_av.formatContext->duration > 0) {
          pos = static_cast<int64_t>(req.value() * m_av.formatContext->duration);
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
      if(req.type() == SEEK_BY_BYTES) {
        pos = static_cast<int64_t>(req.value());
      } else if(req.type() == SEEK_BY_SECONDS) {
        int64_t size = avio_size(m_av.formatContext->pb);
        if(m_av.formatContext->duration <= 0 || size <= 0) {
          Radiant::error("%s Seek failed, couldn't get the media duration/size",
                         errorMsg.data());
          return false;
        }
        // This is just a guess, since there is no byte size and time 1:1 mapping
        pos = static_cast<int64_t>(size * req.value() / m_av.duration);

      } else {
        assert(req.type() == SEEK_RELATIVE);
        int64_t size = avio_size(m_av.formatContext->pb);
        if(size <= 0) {
          Radiant::error("%s Seek failed, couldn't get the media size",
                         errorMsg.data());
          return false;
        }
        pos = static_cast<int64_t>(req.value() * size);
      }
    }

    int64_t minTs = 0, maxTs = std::numeric_limits<int64_t>::max();
    if (req.flags() & SEEK_FLAG_FORWARD) {
      minTs = pos;
    } else {
      maxTs = pos;
    }
    int err = avformat_seek_file(m_av.formatContext.get(), -1, minTs, pos, maxTs,
                                 seekByBytes ? AVSEEK_FLAG_BYTE : AVSEEK_FLAG_BACKWARD);
    if(err < 0) {
      Radiant::error("%s Seek failed", errorMsg.data());
      return false;
    }

    if(m_av.audioCodecContext)
      avcodec_flush_buffers(m_av.audioCodecContext);
    if(m_av.videoCodecContext)
      avcodec_flush_buffers(m_av.videoCodecContext);
    {
      Radiant::Guard g(m_decodedVideoFramesMutex);
      setSeekGeneration(seekRequestGeneration);
    }
    m_av.nextPts = m_av.startPts;
    m_av.nextPtsTb = m_av.startPtsTb;

    return true;
  }

  std::shared_ptr<VideoFrameFfmpeg> FfmpegDecoder::D::getFreeFrame()
  {
    AudioTransferPtr audioTransfer(m_audioTransfer);

    {
      Radiant::Guard g(m_decodedVideoFramesMutex);
      while (m_running && (int)m_decodedVideoFrames.size() >= m_options.videoBufferFrames()) {
        {
          Radiant::Guard g(m_seekRequestMutex);
          if (m_seekRequest.type() != SEEK_NONE)
            return nullptr;
        }

        // if the video buffer is full, and audio buffer is almost empty,
        // we need to resize the video buffer, otherwise we could starve.
        if (audioTransfer && audioTransfer->bufferStateSeconds() < m_options.audioBufferSeconds() * 0.15f &&
            m_options.videoBufferFrames() < 40) {
          m_options.setVideoBufferFrames(m_options.videoBufferFrames() + 1);
          continue;
        }

        m_decodedVideoFramesCond.wait(m_decodedVideoFramesMutex, 100);
      }
    }

    if (!m_running)
      return nullptr;

    std::shared_ptr<VideoFrameFfmpeg> frame = std::make_shared<VideoFrameFfmpeg>();
    frame->frameUnrefMightBlock = m_frameUnrefMightBlock;
    frame->deallocatedFrames = m_deallocatedFrames;
    auto & de = *m_deallocatedFrames;
    Radiant::Guard g(de.mutex);
    if (!de.frames.empty()) {
      frame->frame = std::move(de.frames.front());
      de.frames.erase(de.frames.begin());
    }
    return frame;
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

  bool FfmpegDecoder::D::claimExclusiveAccess(const QString & src, double maxWaitTimeSecs)
  {
    Radiant::Timer timer;
    while (m_running) {
      {
        Radiant::Guard g(s_exclusiveAccessMutex);
        auto it = s_exclusiveAccess.find(src);
        if (it == s_exclusiveAccess.end()) {
          it = s_exclusiveAccess.insert(src).first;
          m_hasExclusiveAccess = true;
          m_exclusiveAccess = it;
          return true;
        }
      }
      if (timer.time() > maxWaitTimeSecs)
        break;
      Radiant::Sleep::sleepMs(10);
    }
    return false;
  }

  void FfmpegDecoder::D::releaseExclusiveAccess()
  {
    if (m_hasExclusiveAccess) {
      m_hasExclusiveAccess = false;
      Radiant::Guard g(s_exclusiveAccessMutex);
      s_exclusiveAccess.erase(m_exclusiveAccess);
    }
  }

  std::shared_ptr<VideoFrameFfmpeg> FfmpegDecoder::D::firstReadyDecodedFrame()
  {
    Radiant::Guard g(m_decodedVideoFramesMutex);
    for (auto & frame: m_decodedVideoFrames)
      if (frame->timestamp().seekGeneration() == m_sync->seekGeneration())
        return frame;
    return {};
  }

  std::shared_ptr<VideoFrameFfmpeg> FfmpegDecoder::D::lastReadyDecodedFrame()
  {
    Radiant::Guard g(m_decodedVideoFramesMutex);
    if (m_decodedVideoFrames.empty())
      return {};
    if (m_decodedVideoFrames.back()->timestamp().seekGeneration() == m_sync->seekGeneration())
      return m_decodedVideoFrames.back();
    return {};
  }

  std::shared_ptr<VideoFrameFfmpeg> FfmpegDecoder::D::playFrame(Radiant::TimeStamp presentTimestamp,
                                                                AVDecoder::ErrorFlags & errors,
                                                                AVDecoder::PlayFlags flags)
  {
    if (!m_av.videoCodec)
      return nullptr;

    /// If we are doing real-time seeking, we don't have a video frame buffer
    /// and we don't care about av-sync, just show the latest frame we have decoded
    const bool useNewestFrame = m_realTimeSeeking || (flags & PLAY_FLAG_NO_BUFFERING)
        || m_forceNewestFrame;

    if (useNewestFrame) {
      std::shared_ptr<VideoFrameFfmpeg> frame = lastReadyDecodedFrame();
      if (frame && !(flags & PLAY_FLAG_NO_SYNC)) {
        m_forceNewestFrame = false;
        m_sync->sync(presentTimestamp, frame->timestamp());
      }
      return frame;
    }

    if (m_av.audioCodec && !m_hasDecodedAudioFrames && m_decodingStartTime.time() < 2.0) {
      // Audio track is not ready, keep playing the first frame
      return firstReadyDecodedFrame();
    }

    if (!m_sync->isValid()) {
      auto frame = firstReadyDecodedFrame();
      if (frame && !(flags & PLAY_FLAG_NO_SYNC))
        m_sync->sync(presentTimestamp, frame->timestamp());
      return frame;
    }

    const Timestamp ts = m_sync->map(presentTimestamp);
    std::shared_ptr<VideoFrameFfmpeg> ret;

    {
      Radiant::Guard g(m_decodedVideoFramesMutex);
      for (std::shared_ptr<VideoFrameFfmpeg> & frame: m_decodedVideoFrames) {
        if (frame->timestamp().pts() > ts.pts()) {
          if (ret)
            return ret;
          return frame;
        }
        if (frame->timestamp().pts() == ts.pts())
          return frame;
        ret = frame;
      }
    }

    if (ret && !(flags & PLAY_FLAG_NO_SYNC)) {
      const double maxDiff = 1.0;
      if (m_hasExternalSync) {
        /// If we are off by more than one second, it's time to seek
        if (std::abs(ts.pts() - ret->timestamp().pts()) > maxDiff)
          m_host->seek(SeekRequest(ts.pts() + 0.5f, SEEK_BY_SECONDS));
      } else {
        /// If we are behind more than one second, it's time to resynchronize
        if ((ts.pts() - ret->timestamp().pts()) > maxDiff) {
          Radiant::Guard g(m_decodedVideoFramesMutex);
          if (increaseSeekGeneration())
            m_sync->sync(presentTimestamp, ret->timestamp());
        }
      }
    }
    errors |= ERROR_VIDEO_FRAME_BUFFER_UNDERRUN;
    return ret;
  }

  bool FfmpegDecoder::D::decodeVideoPacket(double &dpts)
  {
    const double maxPtsReorderDiff = 0.1;
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

    m_av.frame->pts = av_frame_get_best_effort_timestamp(m_av.frame);

    AVRational tb = m_av.formatContext->streams[m_av.videoStreamIndex]->time_base;

    if (m_av.frame->pts != AV_NOPTS_VALUE)
      dpts = av_q2d(tb) * m_av.frame->pts;

    std::shared_ptr<VideoFrameFfmpeg> videoFrame;

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

          tb = m_videoFilter.bufferSinkFilter->inputs[0]->time_base;
          dpts = av_q2d(tb) * m_av.frame->pts;

          if (std::isfinite(m_exactVideoSeekRequestPts)) {
            if (dpts < m_exactVideoSeekRequestPts) {
              skip = true;
              continue;
            }
            m_exactVideoSeekRequestPts = std::numeric_limits<double>::quiet_NaN();
          }

          videoFrame = getFreeFrame();
          if (!videoFrame)
            return false;

          if (!videoFrame->frame.avframe) {
            videoFrame->frame.avframe = av_frame_alloc();
          } else if (videoFrame->frame.referenced) {
            av_frame_unref(videoFrame->frame.avframe);
          }

          AVFrame & avframe = *videoFrame->frame.avframe;
          av_frame_ref(&avframe, m_av.frame);
          videoFrame->frame.referenced = true;
          videoFrame->frame.context = m_av.formatContext;

          videoFrame->setIndex(m_index++);

          auto fmtDescriptor = av_pix_fmt_desc_get(AVPixelFormat(avframe.format));
          setFormat(*videoFrame, *fmtDescriptor, Nimble::Vector2i(avframe.width, avframe.height));
          for (int i = 0; i < videoFrame->planes(); ++i) {
            videoFrame->setLineSize(i, avframe.linesize[i]);
            videoFrame->setData(i, avframe.data[i]);
          }

          videoFrame->setImageSize(Nimble::Vector2i(avframe.width, avframe.height));
          videoFrame->setTimestamp(Timestamp(dpts + m_loopOffset, m_sync->seekGeneration()));

          {
            Radiant::Guard g(m_decodedVideoFramesMutex);
            if (!m_decodedVideoFrames.empty()) {
              VideoFrameFfmpeg & lastReadyFrame = *m_decodedVideoFrames.back();
              if (lastReadyFrame.timestamp().seekGeneration() == videoFrame->timestamp().seekGeneration() &&
                  lastReadyFrame.timestamp().pts()-maxPtsReorderDiff > videoFrame->timestamp().pts()) {
                // There was a problem with the stream, previous frame had larger timestamp than this
                // frame, that should be newer. This must be broken stream or concatenated MPEG file
                // or something similar. We treat this like it was a seek request
                // On some files there are some individual frames out-of-order, we try to minimize
                // this by allowing maximum difference of maxPtsReorderDiff
                /// @todo we probably also want to check if frame.pts is much larger than previous_frame.pts
                if (increaseSeekGeneration())
                  videoFrame->setTimestamp(Timestamp(dpts + m_loopOffset, m_sync->seekGeneration()));
              }
            }
            m_decodedVideoFrames.push_back(std::move(videoFrame));
          }
          m_decodedVideoFramesCond.wakeAll();
        }
      }
    } else {
      if (std::isfinite(m_exactVideoSeekRequestPts)) {
        if (dpts < m_exactVideoSeekRequestPts) {
          return false;
        }
        m_exactVideoSeekRequestPts = std::numeric_limits<double>::quiet_NaN();
      }

      videoFrame = getFreeFrame();
      if (!videoFrame)
        return false;

      if(!videoFrame->frame.avframe) {
        videoFrame->frame.avframe = av_frame_alloc();
      } else if (videoFrame->frame.referenced) {
        av_frame_unref(videoFrame->frame.avframe);
      }

      AVFrame & avframe = *videoFrame->frame.avframe;
      av_frame_ref(&avframe, m_av.frame);
      videoFrame->frame.referenced = true;
      videoFrame->frame.context = m_av.formatContext;

      videoFrame->setIndex(m_index++);

      /// Copy properties to VideoFrame, that acts as a proxy to AVFrame

      auto fmtDescriptor = av_pix_fmt_desc_get(AVPixelFormat(avframe.format));

      int planes = (fmtDescriptor->flags & AV_PIX_FMT_FLAG_PLANAR) ? fmtDescriptor->nb_components : 1;

      assert(avframe.format == m_av.frame->format);
      assert(avframe.width == m_av.frame->width);
      assert(avframe.height == m_av.frame->height);
      for(int i = 0; i < planes; ++i) {
        assert(avframe.linesize[i] == m_av.frame->linesize[i]);
        assert(avframe.data[i] == m_av.frame->data[i]);
      }

      setFormat(*videoFrame, *fmtDescriptor, Nimble::Vector2i(m_av.frame->width, m_av.frame->height));
      for(int i = 0; i < videoFrame->planes(); ++i) {
        videoFrame->setLineSize(i, avframe.linesize[i]);
        videoFrame->setData(i, avframe.data[i]);
      }

      videoFrame->setImageSize(Nimble::Vector2i(m_av.frame->width, m_av.frame->height));
      videoFrame->setTimestamp(Timestamp(dpts + m_loopOffset, m_sync->seekGeneration()));

      {
        Radiant::Guard g(m_decodedVideoFramesMutex);
        if (!m_decodedVideoFrames.empty()) {
          VideoFrameFfmpeg & lastReadyFrame = *m_decodedVideoFrames.back();
          if (lastReadyFrame.timestamp().seekGeneration() == videoFrame->timestamp().seekGeneration() &&
              lastReadyFrame.timestamp().pts()-maxPtsReorderDiff > videoFrame->timestamp().pts()) {
            if (increaseSeekGeneration())
              videoFrame->setTimestamp(Timestamp(dpts + m_loopOffset, m_sync->seekGeneration()));
          }
        }
        m_decodedVideoFrames.push_back(std::move(videoFrame));
      }
      m_decodedVideoFramesCond.wakeAll();
    }

    return true;
  }

  bool FfmpegDecoder::D::decodeAudioPacket(double &dpts)
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
        avError(QString("FfmpegDecoder::D::decodeAudioPacket # %1: Audio decoding error").
                arg(m_options.source()), consumedBytes);
        break;
      }

      if(gotFrame) {
        AVRational tb {1, m_av.frame->sample_rate};
        if (m_av.frame->pts != AV_NOPTS_VALUE) {
          m_av.frame->pts = av_rescale_q(m_av.frame->pts, m_av.audioCodecContext->pkt_timebase, tb);
        } else if (m_av.nextPts != AV_NOPTS_VALUE) {
          m_av.frame->pts = av_rescale_q(m_av.nextPts, m_av.nextPtsTb, tb);
        }

        if (m_av.frame->pts != AV_NOPTS_VALUE) {
          m_av.nextPts = m_av.frame->pts + m_av.frame->nb_samples;
          m_av.nextPtsTb = tb;
          dpts = av_q2d(tb) * m_av.frame->pts;
        }

        DecodedAudioBuffer * decodedAudioBuffer = nullptr;

        if (std::isfinite(m_exactAudioSeekRequestPts) && std::isfinite(dpts)) {
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

            tb = m_audioFilter.bufferSinkFilter->inputs[0]->time_base;
            if (m_av.frame->pts != AV_NOPTS_VALUE)
              dpts = av_q2d(tb) * m_av.frame->pts;

            if (err < 0) {
              avError(QString("FfmpegDecoder::D::decodeAudioPacket # %1: av_buffersink_read failed").
                      arg(m_options.source()), err);
              break;
            }


            while(true) {
              decodedAudioBuffer = audioTransfer->takeFreeBuffer(
                    m_av.decodedAudioBufferSamples - m_av.frame->nb_samples);
              if(decodedAudioBuffer) break;
              if(!m_running) return gotFrames;

              if (m_seekRequest.type() != SEEK_NONE) return false;

              if (m_av.videoCodec && m_av.decodedAudioBufferSamples < 44100*6) {
                Radiant::Guard g(m_decodedVideoFramesMutex);
                if (m_decodedVideoFrames.size() <= 1) {
                  // If the audio sample rate is low, or the stream has huge
                  // audio packets, we might get stuck here while we are
                  // having video buffer underrun. Increase the audio buffer
                  // size and try again.
                  m_av.decodedAudioBufferSamples += 22050;
                  continue;
                }
              }

              Radiant::Sleep::sleepSome(0.01);
              // Make sure that we don't get stuck with a file that doesn't
              // have video frames in the beginning
              audioTransfer->setEnabled(true);
            }

            int64_t channel_layout = av_frame_get_channel_layout(m_av.frame);
            decodedAudioBuffer->fillPlanar(Timestamp(dpts + m_loopOffset, m_sync->seekGeneration()),
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

            if (m_av.videoCodec && m_av.decodedAudioBufferSamples < 44100*6) {
              Radiant::Guard g(m_decodedVideoFramesMutex);
              if (m_decodedVideoFrames.size() <= 1) {
                m_av.decodedAudioBufferSamples += 22050;
                continue;
              }
            }

            Radiant::Sleep::sleepSome(0.01);
          }

          int samples = m_av.frame->nb_samples;

          decodedAudioBuffer->fill(Timestamp(dpts + m_loopOffset, m_sync->seekGeneration()),
                                   m_av.audioCodecContext->channels, samples,
                                   reinterpret_cast<const int16_t *>(m_av.frame->data[0]));
          audioTransfer->putReadyBuffer(samples);
        }
      } else {
        flush = false;
      }
      packet.data += consumedBytes;
      packet.size -= consumedBytes;
      // Clearing packet pts and dts since they shouldn't be used for
      // second time calling decoder with the same packet
      packet.dts = packet.pts = AV_NOPTS_VALUE;
    }
    return gotFrames;
  }

  bool FfmpegDecoder::D::checkSeek()
  {
    SeekRequest req;
    int seekRequestGeneration;
    {
      Radiant::Guard g(m_seekRequestMutex);
      req = m_seekRequest;
      seekRequestGeneration = m_seekRequestGeneration;
    }

    bool didSeek = false;
    if((req.type() != SEEK_NONE)) {
      if(seek(req, seekRequestGeneration)) {
        m_loopOffset = 0;
        didSeek = true;
      }
      Radiant::Guard g(m_seekRequestMutex);
      if (seekRequestGeneration == m_seekRequestGeneration)
        m_seekRequest.setType(SEEK_NONE);
    }
    return didSeek;
  }


  // -------------------------------------------------------------------------


  void FfmpegDecoder::setTlsLogHandler(const FfmpegDecoder::LogHandler * handlerFunc)
  {
    s_logHandler = handlerFunc;
  }

  FfmpegDecoder::FfmpegDecoder()
    : m_d(new D(this))
  {
    Thread::setName("FfmpegDecoder");
 }

  FfmpegDecoder::~FfmpegDecoder()
  {
    close();
    if(isRunning())
      waitEnd();
    m_d->close();
  }

  AVSync::PlayMode FfmpegDecoder::playMode() const
  {
    return m_d->m_sync->playMode();
  }

  void FfmpegDecoder::setPlayMode(AVSync::PlayMode mode)
  {
    m_d->m_sync->setPlayMode(mode);
  }

  std::shared_ptr<VideoFrame> FfmpegDecoder::playFrame(Radiant::TimeStamp presentTimestamp, ErrorFlags & errors,
                                                       PlayFlags flags)
  {
    auto current = m_d->playFrame(presentTimestamp, errors, flags);

    bool changed = false;
    if (current && !(flags & PLAY_FLAG_NO_SYNC)) {
      Radiant::Guard g(m_d->m_decodedVideoFramesMutex);
      for (auto it = m_d->m_decodedVideoFrames.begin(); it != m_d->m_decodedVideoFrames.end();) {
        VideoFrameFfmpeg & frame = **it;

        if (frame.timestamp() < current->timestamp()) {
          it = m_d->m_decodedVideoFrames.erase(it);
          changed = true;
        } else {
          break;
        }
      }
    }

    if (changed)
      m_d->m_decodedVideoFramesCond.wakeAll();

    return current;
  }

  std::shared_ptr<VideoFrame> FfmpegDecoder::peekFrame(std::shared_ptr<VideoFrame> ref, int offset)
  {
    bool found = false;
    Radiant::Guard g(m_d->m_decodedVideoFramesMutex);
    for (auto it = m_d->m_decodedVideoFrames.begin(); it != m_d->m_decodedVideoFrames.end();) {
      std::shared_ptr<VideoFrameFfmpeg> & frame = *it;
      if (!found)
        found = frame == ref;

      if (found)
        if (offset-- == 0)
          return frame;
    }
    return nullptr;
  }

  bool FfmpegDecoder::isEof() const
  {
    if (!finished())
      return false;

    {
      Radiant::Guard g(m_d->m_decodedVideoFramesMutex);
      if (m_d->m_decodedVideoFrames.size() > 1)
        return false;
    }

    if (AudioTransferPtr audioTransfer = m_d->m_audioTransfer)
      return audioTransfer->bufferStateSeconds() <= 0.0f;

    return true;
  }

  Nimble::Matrix4f FfmpegDecoder::yuvMatrix() const
  {
    if(!m_d->m_av.videoCodecContext)
      return Nimble::Matrix4f::IDENTITY;

    const AVColorSpace colorspace = m_d->m_av.videoCodecContext->colorspace;
    const int * coeffs = sws_getCoefficients(colorspace);
    int l = 16, h = 235;

    if (m_d->m_allowJpegRange && (
          m_d->m_av.videoCodecContext->color_range == AVCOL_RANGE_JPEG ||
          m_d->m_av.videoCodecContext->color_range == AVCOL_RANGE_UNSPECIFIED)) {
      l = 0;
      h = 255;
    }

    // a and b scale the y value from [l, h] -> [0, 1]
    const float a = 255.0f/(h-l);
    const float b = l/255.0f;

    const float c[4] = {  coeffs[0]/65536.0f, -coeffs[2]/65536.0f,
                         -coeffs[3]/65536.0f,  coeffs[1]/65536.0f };

    // Last column transform uv from 0..1 to -0.5..0.5
    return Nimble::Matrix4f(
        a, 0.0f, c[0], -b*a - 0.5f * c[0],
        a, c[1], c[2], -b*a - 0.5f * (c[2]+c[1]),
        a, c[3],    0, -b*a - 0.5f * c[3],
        0,    0,    0, 1);
  }

  QByteArray FfmpegDecoder::audioPannerSourceId() const
  {
    if (auto audioTransfer = m_d->m_audioTransfer) {
      return audioTransfer->id();
    }
    return QByteArray();
  }

  bool FfmpegDecoder::setAudioGain(float gain)
  {
    m_d->m_audioGain = gain;
    AudioTransferPtr audioTransfer(m_d->m_audioTransfer);

    if (audioTransfer)
      audioTransfer->setGain(gain);

    return true;
  }

  QString FfmpegDecoder::source() const
  {
    return m_d->m_options.source();
  }

  BufferState FfmpegDecoder::bufferState() const
  {
    BufferState b;
    {
      Radiant::Guard g(m_d->m_decodedVideoFramesMutex);
      b.decodedVideoFrames = static_cast<int>(m_d->m_decodedVideoFrames.size());
    }
    b.decodedVideoFrameBufferSize = m_d->m_options.videoBufferFrames();
    if (m_d->m_av.audioCodecContext)
      b.decodedAudioBufferSizeSeconds = (float)m_d->m_av.decodedAudioBufferSamples /
          m_d->m_av.audioCodecContext->sample_rate;

    if (AudioTransferPtr audioTransfer = m_d->m_audioTransfer)
      b.decodedAudioSeconds = audioTransfer->bufferStateSeconds();
    return b;
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
    if (auto sync = m_d->m_options.externalSync()) {
      m_d->m_sync = sync;
      m_d->m_hasExternalSync = true;
    } else if (m_d->m_hasExternalSync) {
      m_d->m_hasExternalSync = false;
      m_d->m_sync = std::make_shared<AVSync>();
    }
    m_d->m_sync->setPlayMode(options.playMode());
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

  bool FfmpegDecoder::setLooping(bool doLoop)
  {
    m_d->m_options.setLooping(doLoop);
    return true;
  }

  double FfmpegDecoder::duration() const
  {
    return m_d->m_av.hasReliableDuration ? m_d->m_av.duration : 0;
  }

  int FfmpegDecoder::seek(const SeekRequest & req)
  {
    Radiant::Guard g(m_d->m_seekRequestMutex);
    m_d->m_seekRequestGeneration = std::max(m_d->m_sync->seekGeneration(),
                                            m_d->m_seekRequestGeneration);
    int gen = ++m_d->m_seekRequestGeneration;
    m_d->m_seekRequest = req;
    return gen;
  }

  bool FfmpegDecoder::realTimeSeeking() const
  {
    return m_d->m_realTimeSeeking;
  }

  bool FfmpegDecoder::setRealTimeSeeking(bool value)
  {
    AudioTransferPtr audioTransfer(m_d->m_audioTransfer);

    m_d->m_realTimeSeeking = value;
    if(audioTransfer)
      audioTransfer->setSeeking(value);

    return true;
  }

  void FfmpegDecoder::runDecoder()
  {
    QByteArray errorMsg("FfmpegDecoder::D::runDecoder # " + m_d->m_options.source().toUtf8() + ":");
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

    double videoDpts = std::numeric_limits<double>::quiet_NaN();
    double audioDpts = std::numeric_limits<double>::quiet_NaN();

    auto & av = m_d->m_av;

    AudioTransferPtr audioTransfer(m_d->m_audioTransfer);

    if (av.videoCodec && audioTransfer)
      audioTransfer->setEnabled(false);

    int lastError = 0;
    int consecutiveErrorCount = 0;
    /// With v4l2 streams on some devices (like Inogeni DVI capture cards) lots
    /// of errors in the beginning is normal
    int maxConsecutiveErrors = 50;

    m_d->m_decodingStartTime.start();

    while(m_d->m_running) {
      int err = 0;

      if (m_d->checkSeek())
        videoDpts = audioDpts = std::numeric_limits<double>::quiet_NaN();

      if(m_d->m_running && m_d->m_realTimeSeeking && av.videoCodec) {
        std::shared_ptr<VideoFrameFfmpeg> frame = m_d->lastReadyDecodedFrame();
        if(frame && frame->timestamp().seekGeneration() == m_d->m_sync->seekGeneration()) {
          /// frame done, give some break for this thread
          Radiant::Sleep::sleepSome(0.001);
          continue;
        }
      }

      if(eof == EofState::Normal) {
        err = av_read_frame(av.formatContext.get(), &av.packet);
        if (s_forceNewestFrame) {
          m_d->m_forceNewestFrame = true;
          s_forceNewestFrame = false;
        }
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
          if (m_d->seekToBeginning())
            videoDpts = audioDpts = std::numeric_limits<double>::quiet_NaN();
          else
            break; // We are requested to loop, but seek failed and reopening the source failed.
          eof = EofState::Normal;

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
        double prevVideoDpts = videoDpts;
        gotVideoFrame = m_d->decodeVideoPacket(videoDpts);
        if(gotVideoFrame && audioTransfer)
          audioTransfer->setEnabled(true);

        if (gotVideoFrame && std::isfinite(av.start) && std::isfinite(videoDpts) &&
            std::isfinite(prevVideoDpts) && videoDpts > prevVideoDpts) {
          double newDuration = videoDpts + (videoDpts - prevVideoDpts) - av.start;
          if (newDuration > m_d->m_av.duration) {
            m_d->m_av.duration = newDuration;
          }
        }
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
        double prevAudioDpts = audioDpts;
        gotAudioFrame = m_d->decodeAudioPacket(audioDpts);

        if (gotAudioFrame && std::isfinite(av.start) && std::isfinite(audioDpts) &&
            std::isfinite(prevAudioDpts) && audioDpts > prevAudioDpts) {
          double newDuration = audioDpts + (audioDpts - prevAudioDpts) - av.start;
          if (newDuration > m_d->m_av.duration) {
            m_d->m_av.duration = newDuration;
          }
        }
      }

      const bool gotFrames = gotAudioFrame || gotVideoFrame;

      // Flush is done if there are no more frames
      if(eof == EofState::Flush && !gotFrames)
        eof = EofState::Eof;

      if (!std::isfinite(av.start) && gotFrames) {
        if (std::isfinite(videoDpts) && std::isfinite(audioDpts)) {
          av.start = std::min(videoDpts, audioDpts);
        } else if (std::isfinite(videoDpts)) {
          av.start = videoDpts;
        } else if (std::isfinite(audioDpts)) {
          av.start = audioDpts;
        }
      }

      av_packet_unref(&av.packet);

      if (gotAudioFrame)
        m_d->m_hasDecodedAudioFrames = true;

      if(gotFrames)
        state() = STATE_READY;
    }

    state() = STATE_FINISHED;
    s_src = nullptr;

    if (audioTransfer) {
      // Tell audio transfer that there are no more samples coming, so that it
      // knows that it can disable itself when it runs out of the decoded
      // buffer.
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
      avdevice_register_all();
      avformat_network_init();
    }
  }
}
