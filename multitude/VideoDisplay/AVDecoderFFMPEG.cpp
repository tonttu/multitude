/* COPYRIGHT
 *
 * This file is part of VideoDisplay.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

// For PRIx64
#define __STDC_FORMAT_MACROS

#include "AVDecoderFFMPEG.hpp"
#include "MemoryPool.hpp"
#include "AudioTransfer.hpp"

#include <Nimble/Vector2.hpp>

#include <Radiant/Allocators.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/ResourceLocator.hpp>
#include <Radiant/Sleep.hpp>

#include <Resonant/DSPNetwork.hpp>

#include <Valuable/AttributeBool.hpp>
#include <Valuable/State.hpp>

#include <QSet>
#include <QString>
#include <QThread>

#include <array>
#include <cassert>

extern "C" {
  typedef uint64_t UINT64_C;
  typedef int64_t INT64_C;

# include <libavdevice/avdevice.h>

# include <libavutil/audioconvert.h>
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

namespace
{
  template <typename T, size_t N>
  class LockFreeQueue
  {
  public:
    LockFreeQueue();

    bool setSize(int items);
    int size() const;

    T * takeFree();
    void put();

    int itemCount() const;

    T * readyItem(int index = 0);
    T * lastReadyItem();
    void next();

  private:
    std::array<T, N> m_data;
    QAtomicInt m_readyItems;
    // index of the current queue head, "next ready item" (if m_readyItems > 0)
    int m_reader;
    // index of the next free item (if m_readyItems < m_size)
    int m_writer;
    int m_size;
  };

  template <typename T, size_t N>
  LockFreeQueue<T, N>::LockFreeQueue()
    : m_reader(0)
    , m_writer(0)
    , m_size(N)
  {}

  template <typename T, size_t N>
  bool LockFreeQueue<T, N>::setSize(int items)
  {
    m_size = std::min<int>(items, m_data.size());
    return m_size == items;
  }

  template <typename T, size_t N>
  int LockFreeQueue<T, N>::size() const
  {
    return m_size;
  }

  template <typename T, size_t N>
  T * LockFreeQueue<T, N>::takeFree()
  {
    if(m_readyItems >= m_size)
      return 0;

    int index = m_writer++;

    return & m_data[index % N];
  }

  template <typename T, size_t N>
  void LockFreeQueue<T, N>::put()
  {
    m_readyItems.ref();
  }

  template <typename T, size_t N>
  int LockFreeQueue<T, N>::itemCount() const
  {
    return m_readyItems;
  }

  template <typename T, size_t N>
  T * LockFreeQueue<T, N>::readyItem(int index)
  {
    if(index >= m_readyItems) return nullptr;
    return & m_data[(m_reader + index) % N];
  }

  template <typename T, size_t N>
  T * LockFreeQueue<T, N>::lastReadyItem()
  {
    if(m_readyItems < 1) return nullptr;
    return & m_data[(m_reader + m_readyItems - 1) % N];
  }

  template <typename T, size_t N>
  void LockFreeQueue<T, N>::next()
  {
    m_readyItems.deref();
    ++m_reader;
  }

  int ffmpegLock(void ** mutexPtr, enum AVLockOp op)
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

  void ffmpegLog(void *, int level, const char * fmt, va_list vl)
  {
    if(level > AV_LOG_INFO) return;
    //AVClass ** avclass = reinterpret_cast<AVClass**>(ptr);

    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), fmt, vl);
    for(int i = strlen(buffer) - 1; i >= 0; --i) {
      if(buffer[i] == '\r' || buffer[i] == '\n')
        buffer[i] = '\0';
      else
        break;
    }

    QString msg = QString("%1: %2").arg((const char*)s_src).arg(buffer);

    if(level > AV_LOG_WARNING) {
      Radiant::info("%s", msg.toUtf8().data());
    } else if(level > AV_LOG_ERROR) {
      Radiant::warning("%s", msg.toUtf8().data());
    } else {
      Radiant::error("%s", msg.toUtf8().data());
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
  class VideoFrameFFMPEG : public VideoFrame
  {
  public:
    VideoFrameFFMPEG() : VideoFrame(), bufferRef(nullptr) {}
    AVFilterBufferRef * bufferRef;
  };

  class AVDecoderFFMPEG::D
  {
  public:

    struct MyAV
    {
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

      bool dr1;
    };

    // Borrowed from libav/avplay
    struct PtsCorrectionContext
    {
      int64_t num_faulty_pts; /// Number of incorrect PTS values so far
      int64_t num_faulty_dts; /// Number of incorrect DTS values so far
      int64_t last_pts;       /// PTS of the last frame
      int64_t last_dts;       /// DTS of the last frame
    };

    D(AVDecoderFFMPEG * host)
      : m_host(host)
      , m_seekGeneration(0)
      , m_running(true)
      , m_av()
      , m_ptsCorrection()
      , m_realTimeSeeking(false)
      , m_pauseTimestamp(Radiant::TimeStamp::currentTime())
      , m_videoFilter()
      , m_audioFilter()
      , m_radiantTimestampToPts(std::numeric_limits<double>::quiet_NaN())
      , m_loopOffset(0)
      , m_audioTransfer(nullptr)
      , m_audioTrackHasEnded(false)
      , m_maxAudioDelay(0.3)
      , m_lastDecodedAudioPts(std::numeric_limits<double>::quiet_NaN())
      , m_lastDecodedVideoPts(std::numeric_limits<double>::quiet_NaN())
    {
      m_av.videoStreamIndex = -1;
      m_av.audioStreamIndex = -1;
      m_av.videoSize = Nimble::Size();
    }

    AVDecoderFFMPEG * m_host;
    int m_seekGeneration;

    bool m_running;

    MyAV m_av;
    PtsCorrectionContext m_ptsCorrection;

    MemoryPool<DecodedImageBuffer, 80> m_imageBuffers;

    bool m_realTimeSeeking;
    SeekRequest m_seekRequest;

    AVDecoder::Options m_options;
    Radiant::TimeStamp m_pauseTimestamp;

    QList<PixelFormat> m_pixelFormats;

    struct FilterGraph
    {
      AVFilterContext * bufferSourceFilter;
      AVFilterContext * bufferSinkFilter;
      AVFilterContext * formatFilter;
      AVFilterGraph * graph;
    };
    FilterGraph m_videoFilter;
    FilterGraph m_audioFilter;

    // only used when there is no audio or the audio track has ended
    double m_radiantTimestampToPts;

    double m_loopOffset;

    AudioTransfer * m_audioTransfer;

    /// In some videos, the audio track might be shorter than the video track
    /// We have some heuristic to determine when the audio track has actually ended,
    /// we really can't rely on some header-information, we just detect when
    /// there are not audio frames coming out from the av packets.
    bool m_audioTrackHasEnded;
    double m_maxAudioDelay;
    double m_lastDecodedAudioPts;
    double m_lastDecodedVideoPts;

    /// From main thread to decoder thread, list of BufferRefs that should be
    /// released. Can't run that in the main thread without locking.
    LockFreeQueue<AVFilterBufferRef*, 40> m_consumedBufferRefs;

    LockFreeQueue<VideoFrameFFMPEG, 40> m_decodedVideoFrames;

  public:
    bool initFilters(FilterGraph & filterGraph, const QString & description,
                     bool video);
    bool open();
    void close();

    bool seekToBeginning();
    bool seek();

    QByteArray supportedPixFormatsStr();
    void updateSupportedPixFormats();

    // Partially borrowed from libav / ffplay
    int64_t guessCorrectPts(AVFrame * frame);

    bool decodeVideoPacket(double & dpts, double & nextDpts);
    bool decodeAudioPacket(double & dpts, double & nextDpts);
    VideoFrameFFMPEG * getFreeFrame(bool & setTimestampToPts, double & dpts);
    void checkSeek(double & nextVideoDpts, double & videoDpts, double & nextAudioDpts);

    static int getBuffer(AVCodecContext * context, AVFrame * frame);
    ///static int regetBuffer(struct AVCodecContext * context, AVFrame * frame);
    static void releaseBuffer(AVCodecContext * context, AVFrame * frame);
    static void releaseFilterBuffer(AVFilterBuffer * filterBuffer);
    void setFormat(VideoFrameFFMPEG & frame, const AVPixFmtDescriptor & fmtDescriptor,
                   Nimble::Vector2i size);
  };

  void AVDecoderFFMPEG::D::updateSupportedPixFormats()
  {
    // Supported video formats
    // We support:
    //   - all 8 bit planar YUV formats
    //   - grayscale formats
    // We don't support on purpose:
    //   - packed YUV, rendering those is silly and slow
    //   - any other RGB-style format except bgr24 and bgra, better convert it
    //     here than in drivers / render thread - except with OpenGL ES.
    //     And GL_ARB_texture_swizzle isn't supported in OS X
    //   - palette formats
    //   - 1 bit monowhite/monoblack
    //   - accelerated formats like xvmc / va api / vdpau, they don't work with multi-threaded rendering
    //   - nv12 / nv21 (first plane for Y, second plane for UV) - rendering would be slow and weird
    m_pixelFormats.clear();

    if (m_options.pixelFormat == VideoFrame::UNKNOWN || m_options.pixelFormat == VideoFrame::GRAY) {
      m_pixelFormats << AV_PIX_FMT_GRAY8;     ///<        Y        ,  8bpp
    }

    if (m_options.pixelFormat == VideoFrame::UNKNOWN || m_options.pixelFormat == VideoFrame::GRAY_ALPHA) {
      m_pixelFormats << AV_PIX_FMT_Y400A;     ///< 8bit gray, 8bit alpha
    }

    if (m_options.pixelFormat == VideoFrame::UNKNOWN || m_options.pixelFormat == VideoFrame::RGB) {
#ifdef LUMINOUS_OPENGLES
      pixelFormats << AV_PIX_FMT_RGB24;     ///< packed RGB 8:8:8, 24bpp, RGBRGB...
#else
      m_pixelFormats << AV_PIX_FMT_BGR24;     ///< packed RGB 8:8:8, 24bpp, BGRBGR...
#endif
    }

    if (m_options.pixelFormat == VideoFrame::UNKNOWN || m_options.pixelFormat == VideoFrame::RGBA) {
#ifdef LUMINOUS_OPENGLES
      pixelFormats << AV_PIX_FMT_RGBA;      ///< packed RGBA 8:8:8:8, 32bpp, RGBARGBA...
#else
      m_pixelFormats << AV_PIX_FMT_BGRA;      ///< packed BGRA 8:8:8:8, 32bpp, BGRABGRA...
#endif
    }

    if (m_options.pixelFormat == VideoFrame::UNKNOWN || m_options.pixelFormat == VideoFrame::YUV) {
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

    if (m_options.pixelFormat == VideoFrame::UNKNOWN || m_options.pixelFormat == VideoFrame::YUVA) {
      m_pixelFormats << AV_PIX_FMT_YUVA420P;  ///< planar YUV 4:2:0, 20bpp, (1 Cr & Cb sample per 2x2 Y & A samples)
      m_pixelFormats << AV_PIX_FMT_YUVA444P;  ///< planar YUV 4:4:4 32bpp, (1 Cr & Cb sample per 1x1 Y & A samples)
      m_pixelFormats << AV_PIX_FMT_YUVA422P;  ///< planar YUV 4:2:2 24bpp, (1 Cr & Cb sample per 2x1 Y & A samples)
    }
  }

  QByteArray AVDecoderFFMPEG::D::supportedPixFormatsStr()
  {
    QByteArray lst;
    for (auto format: m_pixelFormats) {
      const char * str = av_get_pix_fmt_name(format);
      if (!str) {
        Radiant::error("supportedPixFormatsStr # Failed to convert pixel format %d to string", format);
      } else {
        if (!lst.isEmpty())
          lst += ":";
        lst += str;
      }
    }
    return lst;
  }

  bool AVDecoderFFMPEG::D::initFilters(FilterGraph & filterGraph,
                                       const QString & description, bool video)
  {
    QByteArray errorMsg("AVDecoderFFMPEG::D::initFilters # " + m_options.src.toUtf8() +
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

      QString args;
      if(video) {
        args.sprintf("%d:%d:%d:%d:%d:%d:%d",
                     m_av.videoCodecContext->width, m_av.videoCodecContext->height,
                     m_av.videoCodecContext->pix_fmt,
                     m_av.videoCodecContext->time_base.num, m_av.videoCodecContext->time_base.den,
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

        /// @todo ffmpeg application uses AVStream instead of codec context to
        ///       read time_base, is this wrong?
        args.sprintf("time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s",
                     m_av.audioCodecContext->time_base.num,
                     m_av.audioCodecContext->time_base.den,
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
                     m_options.channelLayout.data());
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

        err = avfilter_graph_parse(filterGraph.graph, description.toUtf8().data(),
                                   inputs, outputs, nullptr);
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

  bool AVDecoderFFMPEG::D::open()
  {
    AVInputFormat * inputFormat = nullptr;
    AVDictionary * avoptions = nullptr;

    QString src(m_options.src);
    QStringList srcs = Radiant::ResourceLocator::instance()->locate(m_options.src);

    if(!srcs.empty())
      src = srcs.front();


    QByteArray errorMsg("AVDecoderFFMPEG::D::open # " + src.toUtf8() + ":");

    if(!m_options.demuxerOptions.isEmpty()) {
      for(auto it = m_options.demuxerOptions.begin(); it != m_options.demuxerOptions.end(); ++it) {
        int err = av_dict_set(&avoptions, it.key().toUtf8().data(), it.value().toUtf8().data(), 0);
        if(err < 0) {
          Radiant::warning("%s av_dict_set(%s, %s): %d", errorMsg.data(),
                           it.key().toUtf8().data(), it.value().toUtf8().data(), err);
        }
      }
    }

    // If user specified any specific format, try to use that.
    // Otherwise avformat_open_input will just auto-detect the format.
    if(!m_options.format.isEmpty()) {
      inputFormat = av_find_input_format(m_options.format.toUtf8().data());
      if(!inputFormat)
        Radiant::warning("%s Failed to find input format '%s'", errorMsg.data(), m_options.format.toUtf8().data());
    }

    // Open the actual video, should be thread-safe
    int err = avformat_open_input(&m_av.formatContext, src.toUtf8().data(),
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
      avError(QString("%1 Failed to open the source file").arg(errorMsg.data()), err);
      return false;
    }

    // Retrieve stream information, avformat processes some stream data, so
    // this might take a while, and it might fail with some files (at least
    // with some mkv files), so we don't abort on error
    err = avformat_find_stream_info(m_av.formatContext, nullptr);
    if(err < 0)
      avError(QString("%1 Failed to find stream info").arg(errorMsg.data()), err);

    if(m_options.video) {
      m_av.videoStreamIndex = av_find_best_stream(m_av.formatContext, AVMEDIA_TYPE_VIDEO,
                                                m_options.videoStreamIndex, -1,
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
        m_av.videoCodecContext = m_av.formatContext->streams[m_av.videoStreamIndex]->codec;
        assert(m_av.videoCodecContext);
        m_av.videoCodecContext->opaque = this;
        m_av.videoCodecContext->thread_count = 1;
        /// @todo On slower computers having at least 4 threads is requirement for 4k videos.
        ///       It is unknown if this adds too much overhead if there are ~100 low quality videos.
        if (m_av.videoCodec && (m_av.videoCodec->capabilities & CODEC_CAP_SLICE_THREADS)) {
          m_av.videoCodecContext->thread_count = (m_av.videoCodec->capabilities & CODEC_CAP_AUTO_THREADS)
            ? 0 : 4;
        }
      }
    }

    if(m_options.audio) {
      m_av.audioStreamIndex = av_find_best_stream(m_av.formatContext, AVMEDIA_TYPE_AUDIO,
                                                m_options.audioStreamIndex, -1,
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
        m_av.audioCodecContext = m_av.formatContext->streams[m_av.audioStreamIndex]->codec;
        assert(m_av.audioCodecContext);
        m_av.audioCodecContext->opaque = this;
        m_av.audioCodecContext->thread_count = 1;
      }
    }

    if(!m_av.videoCodec && !m_av.audioCodec) {
      Radiant::error("%s Didn't open any media streams", errorMsg.data());
      avformat_close_input(&m_av.formatContext);
      return false;
    }

    // Open codecs
    if(m_av.videoCodec) {
      if(!m_options.videoOptions.isEmpty()) {
        for(auto it = m_options.videoOptions.begin(); it != m_options.videoOptions.end(); ++it) {
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
      if(!m_options.audioOptions.isEmpty()) {
        for(auto it = m_options.audioOptions.begin(); it != m_options.audioOptions.end(); ++it) {
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

    // We want to use our own ImageBuffers with AVFrames to avoid data copying
    // and to extend the lifetimes of the buffers outside this class
    // If the codec doesn't support that, we have to make a copy of the data
    // buffer after decoding. When using filters, we just use buffer refs

    if(m_av.videoCodecContext) {
      if(m_av.videoCodecContext && (m_av.videoCodec->capabilities & CODEC_CAP_DR1)) {
        m_av.videoCodecContext->get_buffer = getBuffer;
        //videoCodecContext->reget_buffer = AVDecoderFFMPEG::D::regetBuffer;
        m_av.videoCodecContext->release_buffer = releaseBuffer;
        m_av.dr1 = true;
      } else {
        Radiant::debug("%s Codec has no CODEC_CAP_DR1, need to copy the image data every frame", errorMsg.data());
        m_av.dr1 = false;
      }

      bool pixelFormatSupported = false;
      for (auto fmt: m_pixelFormats) {
        if(m_av.videoCodecContext->pix_fmt == fmt) {
          pixelFormatSupported = true;
          break;
        }
      }
      const bool useVideoFilters = !pixelFormatSupported || !m_options.videoFilters.isEmpty();

      if(useVideoFilters)
        initFilters(m_videoFilter, m_options.videoFilters, true);
    }

    if(m_av.audioCodecContext) {
      if (m_options.channelLayout.isEmpty()) {
        QByteArray channelLayout(255, '\0');
        av_get_channel_layout_string(channelLayout.data(), channelLayout.size(),
                                     m_av.audioCodecContext->channels,
                                     m_av.audioCodecContext->channel_layout);
        m_options.channelLayout = channelLayout.data();
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
          !m_options.audioFilters.isEmpty() ||
          m_av.audioCodecContext->sample_rate != targetSampleRate ||
          m_av.audioCodecContext->channel_layout != av_get_channel_layout(m_options.channelLayout.data());

      if(useAudioFilters)
        initFilters(m_audioFilter, m_options.audioFilters, false);
    }

    // pts/dts x video/audioTsToSecs == timestamp in seconds
    if(m_av.videoCodecContext) {
      const auto & time_base = m_av.formatContext->streams[m_av.videoStreamIndex]->time_base;
      m_av.videoTsToSecs = time_base.den != 0 ? av_q2d(time_base) :
                                              av_q2d(m_av.videoCodecContext->time_base) *
                                              m_av.videoCodecContext->ticks_per_frame;
    }

    if(m_av.audioCodecContext) {
      const auto & time_base = m_av.formatContext->streams[m_av.audioStreamIndex]->time_base;
      m_av.audioTsToSecs = time_base.den != 0 ? av_q2d(time_base) :
                                              av_q2d(m_av.audioCodecContext->time_base) *
                                              m_av.audioCodecContext->ticks_per_frame;
    }

    // Size of the decoded audio buffer, in samples (~44100 samples means one second buffer)
    m_av.decodedAudioBufferSamples = m_av.audioCodecContext ?
          m_options.audioBufferSeconds * m_av.audioCodecContext->sample_rate : 0;

    m_av.needFlushAtEof = (m_av.audioCodec && (m_av.audioCodec->capabilities & CODEC_CAP_DELAY)) ||
        (m_av.videoCodec && (m_av.videoCodec->capabilities & CODEC_CAP_DELAY));

    // We seek by bytes only if the input file has timestamp discontinuities
    // (seeking by timestamp doesn't really make sense in that case). If the
    // format doesn't support byte seek, we still use timestamp seeking as a
    // fallback, and then just hope for the best.
    m_av.seekByBytes = (m_av.formatContext->iformat->flags & AVFMT_TS_DISCONT) &&
        !(m_av.formatContext->iformat->flags & AVFMT_NO_BYTE_SEEK);

    /// @todo can seeking be supported even if format context doesn't have an IO Context?
    m_av.seekingSupported = m_av.formatContext->pb && m_av.formatContext->pb->seekable;

    av_init_packet(&m_av.packet);

    // In theory we could write this, but we wouldn't be binary-compatible with
    // different ffmpeg versions:
    // AVFrame avFrame;
    // avcodec_get_frame_defaults(&avFrame);
    m_av.frame = avcodec_alloc_frame();
    if(!m_av.frame) {
      Radiant::error("%s Failed to allocate new AVFrame", errorMsg.data());
      close();
      return false;
    }

    if(m_av.audioCodec) {
      int channelLayout = av_get_channel_layout(m_options.channelLayout);
      m_audioTransfer = new AudioTransfer(m_host, av_get_channel_layout_nb_channels(channelLayout));
      m_audioTransfer->setSeekGeneration(m_seekGeneration);
      m_audioTransfer->setPlayMode(m_options.playMode);

      static QAtomicInt counter;
      int value = counter.fetchAndAddRelease(1);
      m_audioTransfer->setId(QString("VideoDisplay.AudioTransfer.%1").arg(value).toUtf8());

      auto item = Resonant::DSPNetwork::Item();
      item.setModule(m_audioTransfer);
      item.setTargetChannel(0);

      Resonant::DSPNetwork::instance()->addModule(item);
    }

    if(m_av.videoCodecContext) {
      m_av.videoSize = Nimble::Size(m_av.videoCodecContext->width, m_av.videoCodecContext->height);
    } else {
      m_av.videoSize = Nimble::Size();
    }
    m_av.duration = m_av.formatContext->duration / double(AV_TIME_BASE);
    m_av.start = std::numeric_limits<double>::quiet_NaN();

    return true;
  }

  void AVDecoderFFMPEG::D::close()
  {
    m_av.duration = 0;
    m_av.videoSize = Nimble::Size();

    // Close the codecs
    if(m_av.audioCodecContext || m_av.videoCodecContext) {
      if(m_av.audioCodecContext)
        avcodec_close(m_av.audioCodecContext);
      if(m_av.videoCodecContext)
        avcodec_close(m_av.videoCodecContext);
    }

    // Close the video file
    if(m_av.formatContext)
      avformat_close_input(&m_av.formatContext);

    av_free(m_av.frame);

    if(m_audioTransfer) {
      m_audioTransfer->shutdown();
      Resonant::DSPNetwork::instance()->markDone(*m_audioTransfer);
    }
    
    m_audioTransfer = nullptr;
  }

  bool AVDecoderFFMPEG::D::seekToBeginning()
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
        avError(QString("AVDecoderFFMPEG::D::seekToBeginning # %1: Seek error, re-opening the stream").arg(m_options.src), err);
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

  bool AVDecoderFFMPEG::D::seek()
  {
    QByteArray errorMsg("AVDecoderFFMPEG::D::seek # " + m_options.src.toUtf8() + ":");

    if(m_seekRequest.value <= std::numeric_limits<double>::epsilon()) {
      bool ok = seekToBeginning();
      if(ok) {
        ++m_seekGeneration;
        if(m_audioTransfer)
          m_audioTransfer->setSeekGeneration(m_seekGeneration);
        m_radiantTimestampToPts = std::numeric_limits<double>::quiet_NaN();
        if(m_options.playMode == Pause)
          m_pauseTimestamp = Radiant::TimeStamp::currentTime();
      }
      return ok;
    }

    if(!m_av.seekingSupported)
      return false;

    bool seekByBytes = m_av.seekByBytes || m_seekRequest.type == SeekByBytes;

    if(m_seekRequest.type == SeekByBytes &&
       (m_av.formatContext->iformat->flags & AVFMT_NO_BYTE_SEEK)) {
      Radiant::error("%s Seek failed, media doesn't support byte seeking",
                     errorMsg.data());
      return false;
    }

    int64_t pos = 0;
    if(!seekByBytes) {
      if(m_seekRequest.type == SeekBySeconds) {
        pos = m_seekRequest.value * AV_TIME_BASE;
      } else {
        assert(m_seekRequest.type == SeekRelative);
        if(m_av.formatContext->duration > 0) {
          pos = m_seekRequest.value * m_av.formatContext->duration;
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
      if(m_seekRequest.type == SeekByBytes) {
        pos = m_seekRequest.value;
      } else if(m_seekRequest.type == SeekBySeconds) {
        int64_t size = avio_size(m_av.formatContext->pb);
        if(m_av.formatContext->duration <= 0 || size <= 0) {
          Radiant::error("%s Seek failed, couldn't get the media duration/size",
                         errorMsg.data());
          return false;
        }
        // This is just a guess, since there is no byte size and time 1:1 mapping
        pos = size * m_seekRequest.value / m_av.duration;

      } else {
        assert(m_seekRequest.type == SeekRelative);
        int64_t size = avio_size(m_av.formatContext->pb);
        if(size <= 0) {
          Radiant::error("%s Seek failed, couldn't get the media size",
                         errorMsg.data());
          return false;
        }
        pos = m_seekRequest.value * size;
      }
    }

    int64_t minTs = m_seekRequest.direction == SeekOnlyForward
        ? pos : std::numeric_limits<int64_t>::min();
    int64_t maxTs = m_seekRequest.direction == SeekOnlyBackward
        ? pos : std::numeric_limits<int64_t>::max();

    int err = avformat_seek_file(m_av.formatContext, -1, minTs, pos, maxTs,
                                 seekByBytes ? AVSEEK_FLAG_BYTE : 0);
    if(err < 0) {
      Radiant::error("%s Seek failed", errorMsg.data());
      return false;
    }

    if(m_av.audioCodecContext)
      avcodec_flush_buffers(m_av.audioCodecContext);
    if(m_av.videoCodecContext)
      avcodec_flush_buffers(m_av.videoCodecContext);
    ++m_seekGeneration;
    if(m_audioTransfer)
      m_audioTransfer->setSeekGeneration(m_seekGeneration);
    m_radiantTimestampToPts = std::numeric_limits<double>::quiet_NaN();
    if(m_options.playMode == Pause)
      m_pauseTimestamp = Radiant::TimeStamp::currentTime();
    m_audioTrackHasEnded = false;
    m_lastDecodedAudioPts = std::numeric_limits<double>::quiet_NaN();
    m_lastDecodedVideoPts = std::numeric_limits<double>::quiet_NaN();

    return true;
  }

  VideoFrameFFMPEG * AVDecoderFFMPEG::D::getFreeFrame(bool & setTimestampToPts, double & dpts)
  {
    while(m_running) {
      VideoFrameFFMPEG * frame = m_decodedVideoFrames.takeFree();
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
      if(m_audioTransfer && m_audioTransfer->bufferStateSeconds() < m_options.audioBufferSeconds * 0.15f) {
        if(m_decodedVideoFrames.setSize(m_decodedVideoFrames.size() + 1)) {
          m_options.videoBufferFrames = m_decodedVideoFrames.size();
          continue;
        }
      }

      Radiant::Sleep::sleepMs(10);
    }
    return nullptr;
  }

  void AVDecoderFFMPEG::D::setFormat(VideoFrameFFMPEG & frame, const AVPixFmtDescriptor & fmtDescriptor,
                                     Nimble::Vector2i size)
  {
    // not exactly true for all formats, but it is true for all formats that we support
    frame.planes = (fmtDescriptor.flags & PIX_FMT_PLANAR) ? fmtDescriptor.nb_components : 1;

    if(fmtDescriptor.nb_components == 1)
      frame.format = VideoFrame::GRAY;
    else if(fmtDescriptor.nb_components == 2)
      frame.format = VideoFrame::GRAY_ALPHA;
    else if(fmtDescriptor.nb_components == 3 && (fmtDescriptor.flags & PIX_FMT_RGB))
      frame.format = VideoFrame::RGB;
    else if(fmtDescriptor.nb_components == 3)
      frame.format = VideoFrame::YUV;
    else if(fmtDescriptor.nb_components == 4 && (fmtDescriptor.flags & PIX_FMT_RGB))
      frame.format = VideoFrame::RGBA;
    else if(fmtDescriptor.nb_components == 4)
      frame.format = VideoFrame::YUVA;
    else {
      frame.format = VideoFrame::UNKNOWN;
      frame.planes = 0;
    }

    for(int i = 0; i < frame.planes; ++i) {
      frame.planeSize[i] = size;
      if((frame.format == VideoFrame::YUV || frame.format == VideoFrame::YUVA) && (i == 1 || i == 2)) {
        frame.planeSize[i] = Nimble::Vector2i(
              -((-size.x) >> fmtDescriptor.log2_chroma_w),
              -((-size.y) >> fmtDescriptor.log2_chroma_h));
      }
      frame.lineSize[i] = 0;
      frame.data[i] = nullptr;
    }
    for(int i = frame.planes; i < 4; ++i) {
      frame.planeSize[i] = Nimble::Vector2i(0, 0);
      frame.lineSize[i] = 0;
      frame.data[i] = nullptr;
    }
  }

  int64_t AVDecoderFFMPEG::D::guessCorrectPts(AVFrame * frame)
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

  bool AVDecoderFFMPEG::D::decodeVideoPacket(double & dpts, double & nextDpts)
  {
    const double prevDpts = dpts;
    dpts = std::numeric_limits<double>::quiet_NaN();

    int gotPicture = 0;
    avcodec_get_frame_defaults(m_av.frame);
    int err = avcodec_decode_video2(m_av.videoCodecContext, m_av.frame, &gotPicture, &m_av.packet);
    if(err < 0) {
      avError(QString("AVDecoderFFMPEG::D::decodeVideoPacket # %1: Failed to decode a video frame").
              arg(m_options.src), err);
      return false;
    }

    if(!gotPicture)
      return false;

    int64_t pts = guessCorrectPts(m_av.frame);

    dpts = m_av.videoTsToSecs * pts;

    VideoFrameFFMPEG * frame = 0;
    bool setTimestampToPts = false;

    DecodedImageBuffer * buffer = nullptr;
    if(m_av.dr1 && m_av.frame->opaque) {
      buffer = static_cast<DecodedImageBuffer*>(m_av.frame->opaque);
      buffer->refcount.ref();
    }

    if(m_videoFilter.graph) {
      AVFilterBufferRef * ref = avfilter_get_video_buffer_ref_from_arrays(
            m_av.frame->data, m_av.frame->linesize, AV_PERM_READ | AV_PERM_WRITE,
            m_av.frame->width, m_av.frame->height,
            AVPixelFormat(m_av.frame->format));

      if (ref)
        avfilter_copy_frame_props(ref, m_av.frame);

      if(buffer) {
        ref->buf->priv = new std::pair<AVDecoderFFMPEG::D *, DecodedImageBuffer *>(this, buffer);
        ref->buf->free = releaseFilterBuffer;
      }

      int err = av_buffersrc_buffer(m_videoFilter.bufferSourceFilter, ref);
      if(err < 0) {
        avError(QString("AVDecoderFFMPEG::D::decodeVideoPacket # %1: av_buffersrc_add_ref failed").
                arg(m_options.src), err);
        avfilter_unref_buffer(ref);
      } else {
        while (true) {
          // we either use custom deleter (releaseFilterBuffer) or the
          // default one with the actual data cleared
          if(!buffer)
            m_av.packet.data = nullptr;

          AVFilterBufferRef * output = nullptr;
          err = av_buffersink_read(m_videoFilter.bufferSinkFilter, &output);
          if (err == AVERROR(EAGAIN) || err == AVERROR_EOF)
            break;
          if (err < 0) {
            avError(QString("AVDecoderFFMPEG::D::decodeVideoPacket # %1: av_buffersink_read failed").
                    arg(m_options.src), err);
            break;
          }

          if(output) {
            frame = getFreeFrame(setTimestampToPts, dpts);
            if(!frame)
              return false;

            frame->bufferRef = output;
            frame->imageBuffer = nullptr;

            auto fmtDescriptor = av_pix_fmt_desc_get(AVPixelFormat(output->format));
            setFormat(*frame, *fmtDescriptor, Nimble::Vector2i(output->video->w, output->video->h));
            for (int i = 0; i < frame->planes; ++i) {
              if (output->linesize[i] < 0) {
                /// @todo if we have a negative linesize, we should just make a copy of the data,
                ///       since OpenGL doesn't support negative linesizes (GL_UNPACK_ROW_LENGTH
                ///       needs to be positive). For now some formats and filters (like vflip)
                ///       won't work.
              } else {
                frame->lineSize[i] = output->buf->linesize[i];
                frame->data[i] = output->buf->data[i];
              }
              frame->lineSize[i] = output->linesize[i];
              frame->data[i] = output->data[i];
            }

            /// output->pts should be AV_NOPTS_VALUE if not defined,
            /// but some filters just set it always to zero
            if(output->pts != (int64_t) AV_NOPTS_VALUE && output->pts != 0) {
              pts = output->pts;
              dpts = m_av.videoTsToSecs * output->pts;
            }

            frame->imageSize = Nimble::Vector2i(output->video->w, output->video->h);
            frame->timestamp = Timestamp(dpts + m_loopOffset, m_seekGeneration);

            m_decodedVideoFrames.put();
          }
        }
      }
    } else {
      frame = getFreeFrame(setTimestampToPts, dpts);
      if(!frame)
        return false;

      frame->bufferRef = nullptr;
      frame->imageBuffer = buffer;

      auto fmtDescriptor = av_pix_fmt_desc_get(AVPixelFormat(m_av.frame->format));
      int bytes = 0;
      setFormat(*frame, *fmtDescriptor, Nimble::Vector2i(m_av.frame->width, m_av.frame->height));
      for(int i = 0; i < frame->planes; ++i) {
        frame->lineSize[i] = m_av.frame->linesize[i];
        frame->data[i] = m_av.frame->data[i];
        bytes += frame->lineSize[i] * frame->planeSize[i].y;
      }

      if(!buffer) {
        buffer = m_imageBuffers.get();
        if(!buffer) {
          Radiant::error("AVDecoderFFMPEG::D::decodeVideoPacket # %s: Not enough ImageBuffers",
                         m_options.src.toUtf8().data());
          for(int i = 0; i < frame->planes; ++i)
            frame->data[i] = nullptr;
          frame->planes = 0;
        } else {
          buffer->refcount = 1;
          frame->imageBuffer = buffer;
          buffer->data.resize(bytes);
          for(int offset = 0, i = 0; i < frame->planes; ++i) {
            uint8_t * dst = buffer->data.data() + offset;
            bytes = frame->lineSize[i] * frame->planeSize[i].y;
            offset += bytes;
            memcpy(dst, m_av.frame->data[i], bytes);
            frame->data[i] = dst;
          }
        }
      }

      frame->imageSize = Nimble::Vector2i(m_av.frame->width, m_av.frame->height);
      frame->timestamp = Timestamp(dpts + m_loopOffset, m_seekGeneration);
      m_decodedVideoFrames.put();
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

  bool AVDecoderFFMPEG::D::decodeAudioPacket(double & dpts, double & nextDpts)
  {
    AVPacket packet = m_av.packet;
    bool gotFrames = false;
    bool flush = packet.size == 0;

    while(m_running && (packet.size > 0 || flush)) {
      int gotFrame = 0;
      avcodec_get_frame_defaults(m_av.frame);
      const int consumedBytes = avcodec_decode_audio4(m_av.audioCodecContext, m_av.frame,
                                                &gotFrame, &packet);
      if(consumedBytes < 0) {
        avError(QString("AVDecoderFFMPEG::D::decodeAudioPacket # %1: Audio decoding error").
                arg(m_options.src), consumedBytes);
        break;
      }

      if(gotFrame) {
        gotFrames = true;
        int64_t pts = guessCorrectPts(m_av.frame);

        dpts = m_av.audioTsToSecs * pts;
        nextDpts = dpts + double(m_av.frame->nb_samples) / m_av.frame->sample_rate;

        DecodedAudioBuffer * decodedAudioBuffer = nullptr;
        if(m_audioFilter.graph) {
          AVFilterBufferRef * ref = avfilter_get_audio_buffer_ref_from_arrays(
                m_av.frame->data, m_av.frame->linesize[0], AV_PERM_READ | AV_PERM_WRITE,
              m_av.frame->nb_samples, (AVSampleFormat)m_av.frame->format, m_av.frame->channel_layout);
          ref->buf->free = [](AVFilterBuffer *ptr) { av_free(ptr); };
          if (!ref) {
            Radiant::error("AVDecoderFFMPEG::D::decodeAudioPacket # %s: avfilter_get_audio_buffer_ref_from_arrays failed",
                           m_options.src.toUtf8().data());
          } else {
            avfilter_copy_frame_props(ref, m_av.frame);
            av_buffersrc_buffer(m_audioFilter.bufferSourceFilter, ref);
            while (true) {
              AVFilterBufferRef * output = nullptr;
              int err = av_buffersink_read(m_audioFilter.bufferSinkFilter, &output);
              if (err == AVERROR(EAGAIN) || err == AVERROR_EOF)
                break;

              if (err < 0) {
                avError(QString("AVDecoderFFMPEG::D::decodeAudioPacket # %1: av_buffersink_read failed").
                        arg(m_options.src), err);
                break;
              }

              if(output) {
                while(true) {
                  decodedAudioBuffer = m_audioTransfer->takeFreeBuffer(
                        m_av.decodedAudioBufferSamples - output->audio->nb_samples);
                  if(decodedAudioBuffer) break;
                  if(!m_running) return gotFrames;
                  Radiant::Sleep::sleepMs(10);
                }

                /// This used to work in ffmpeg, in libav this pts has some weird values after seeking
                /*if(output->pts != (int64_t) AV_NOPTS_VALUE) {
                  pts = output->pts;
                  dpts = av.audioTsToSecs * output->pts;
                  nextDpts = dpts + double(output->audio->nb_samples) / output->audio->sample_rate;
                }*/

                decodedAudioBuffer->fillPlanar(Timestamp(dpts + m_loopOffset, m_seekGeneration),
                                               av_get_channel_layout_nb_channels(output->audio->channel_layout),
                                               output->audio->nb_samples, (const float **)(output->data));
                m_audioTransfer->putReadyBuffer(output->audio->nb_samples);
                avfilter_unref_buffer(output);
              }
            }
          }
        } else {
          while(true) {
            decodedAudioBuffer = m_audioTransfer->takeFreeBuffer(
                  m_av.decodedAudioBufferSamples - m_av.frame->nb_samples);
            if(decodedAudioBuffer) break;
            if(!m_running) return gotFrames;
            Radiant::Sleep::sleepMs(10);
          }

          decodedAudioBuffer->fill(Timestamp(dpts + m_loopOffset, m_seekGeneration),
                                   m_av.audioCodecContext->channels, m_av.frame->nb_samples,
                                   reinterpret_cast<const int16_t *>(m_av.frame->data[0]));
          m_audioTransfer->putReadyBuffer(m_av.frame->nb_samples);
        }
      } else {
        flush = false;
      }
      packet.data += consumedBytes;
      packet.size -= consumedBytes;
    }
    return gotFrames;
  }

  // This function basically follows the same pattern that is used inside ffmpeg,
  // with some fixes and our custom buffer memory management
  int AVDecoderFFMPEG::D::getBuffer(AVCodecContext * context, AVFrame * frame)
  {
    frame->opaque = nullptr;

    Nimble::Vector2i bufferSize(context->width, context->height);
    if(av_image_check_size(context->width, context->height, 0, context) || context->pix_fmt < 0)
      return -1;

    // ffplay nor the default get_buffer will check this (they might crash with
    // SVQ1 content), but "ffmpeg" application does always allocate too large
    // buffer just because there are some issues in SVQ1 decoder. We will just
    // check the type and then decide the size. A the current version, 32
    // pixels should be enough.
    static const unsigned avEdgeWidth = avcodec_get_edge_width();
    const unsigned edgeWidth = std::max(context->codec_id == CODEC_ID_SVQ1 ? 32u : 0u,
                                        avEdgeWidth);

    // For some reason ffplay and the default get_buffer will do this _after_
    // align_dimensions, even though that is probably wrong (since edgewidth ~ 16
    // and we usually align to 16, the result is the same, for now).
    if((context->flags & CODEC_FLAG_EMU_EDGE) == 0)
      bufferSize += Nimble::Vector2i(edgeWidth*2, edgeWidth*2);

    auto fmtDescriptor = av_pix_fmt_desc_get(context->pix_fmt);
    const int pixelSize = fmtDescriptor->comp[0].step_minus1+1;

    int hChromaShift, vChromaShift;
    avcodec_get_chroma_sub_sample(context->pix_fmt, &hChromaShift, &vChromaShift);

    int strideAlign[AV_NUM_DATA_POINTERS];
    avcodec_align_dimensions2(context, &bufferSize.x, &bufferSize.y, strideAlign);

    int unaligned = 0;
    AVPicture picture;
    do {
      // NOTE: do not align linesizes individually, this breaks e.g. assumptions
      // that linesize[0] == 2*linesize[1] in the MPEG-encoder for 4:2:2
      av_image_fill_linesizes(picture.linesize, context->pix_fmt, bufferSize.x);
      // increase alignment of w for next try (rhs gives the lowest bit set in w)
      bufferSize.x += bufferSize.x & ~(bufferSize.x-1);

      unaligned = 0;
      for(int i = 0; i < 4; ++i)
        unaligned |= picture.linesize[i] % strideAlign[i];
    } while (unaligned);

    // we use offsets to null pointer to calculate the number of image planes
    // and their size
    const int tmpsize = av_image_fill_pointers(picture.data, context->pix_fmt,
                                               bufferSize.y, NULL, picture.linesize);
    if(tmpsize < 0)
      return -1;

    int size[4] = {0, 0, 0, 0};
    int lastPlane = 0;
    for(; lastPlane < 3 && picture.data[lastPlane+1]; ++lastPlane)
      size[lastPlane] = picture.data[lastPlane+1] - picture.data[lastPlane];
    size[lastPlane] = tmpsize - (picture.data[lastPlane] - picture.data[0]);

    // For unknown reason the default get_buffer will have a 16 extra bytes in each line.
    // Maybe some codecs need it.
    const int totalsize = size[0] + size[1] + size[2] + size[3] + (lastPlane+1)*16;

    assert(context->opaque);
    AVDecoderFFMPEG::D & d = *static_cast<AVDecoderFFMPEG::D*>(context->opaque);
    DecodedImageBuffer * buffer = d.m_imageBuffers.get();
    if(!buffer) {
      Radiant::error("AVDecoderFFMPEG::D::getBuffer # %s: not enough ImageBuffers",
                     d.m_options.src.toUtf8().data());
      return -1;
    }

    buffer->refcount = 0;
    buffer->refcount.ref();
    frame->opaque = buffer;
    buffer->data.resize(totalsize);

    int offset = 0;
    int plane = 0;
    for(; plane < 4 && size[plane]; ++plane) {
      const int hShift = plane == 0 ? 0 : hChromaShift;
      const int vShift = plane == 0 ? 0 : vChromaShift;

      frame->linesize[plane] = picture.linesize[plane];

      frame->base[plane] = buffer->data.data() + offset;
      offset += size[plane] + 16;

      // no edge if EDGE EMU or not planar YUV
      if((context->flags & CODEC_FLAG_EMU_EDGE) || !size[2])
        frame->data[plane] = frame->base[plane];
      else
        frame->data[plane] = frame->base[plane] + FFALIGN(
              ((frame->linesize[plane] * edgeWidth) >> vShift) +
              ((pixelSize * edgeWidth) >> hShift), strideAlign[plane]);
    }
    for (; plane < AV_NUM_DATA_POINTERS; ++plane) {
      frame->base[plane] = frame->data[plane] = nullptr;
      frame->linesize[plane] = 0;
    }

    if(size[1] && !size[2])
      avpriv_set_systematic_pal2((uint32_t*)frame->data[1], context->pix_fmt);

    // Tell ffmpeg not to do anything weird with this buffer, since this is ours
    frame->type = FF_BUFFER_TYPE_USER;

    frame->extended_data = frame->data;
    frame->sample_aspect_ratio = context->sample_aspect_ratio;

    if(context->pkt) {
      frame->pkt_pts = context->pkt->pts;
      //frame->pkt_pos = contexts->pkt->pos;
    } else {
      frame->pkt_pts = AV_NOPTS_VALUE;
      //frame->pkt_pos = -1;
    }
    frame->reordered_opaque = context->reordered_opaque;
    frame->sample_aspect_ratio = context->sample_aspect_ratio;
    frame->width = context->width;
    frame->height = context->height;
    frame->format = context->pix_fmt;

    return 0;
  }

  /// @todo should implement this. However, it is unclear if we should
  ///       a) make a copy, b) increase use count, c) do nothing with the
  ///       actual ImageBuffer or d) use base implementation.
  ///       We don't know if releaseBuffer will be called twice or once.
  /*int AVDecoderFFMPEG::D::regetBuffer(AVCodecContext * context, AVFrame * frame)
  {
    if(!frame->data[0])
      return context->get_buffer(context, frame);

    frame->reordered_opaque = context->reordered_opaque;
    frame->pkt_pts = context->pkt ? context->pkt->pts : AV_NOPTS_VALUE;
    return 0;
  }*/

  void AVDecoderFFMPEG::D::releaseBuffer(struct AVCodecContext * context, AVFrame * frame)
  {
    assert(context->opaque);
    assert(frame->opaque);
    assert(frame->type == FF_BUFFER_TYPE_USER);

    DecodedImageBuffer & buffer = *static_cast<DecodedImageBuffer*>(frame->opaque);
    if(!buffer.refcount.deref()) {
      AVDecoderFFMPEG::D & d = *static_cast<AVDecoderFFMPEG::D *>(context->opaque);
      d.m_imageBuffers.put(buffer);
    }
    frame->opaque = nullptr;
    memset(frame->data, 0, sizeof(frame->data));
  }

  void AVDecoderFFMPEG::D::releaseFilterBuffer(AVFilterBuffer * filterBuffer)
  {
    auto * param = static_cast<std::pair<AVDecoderFFMPEG::D *, DecodedImageBuffer *> *>(filterBuffer->priv);

    if(!param->second->refcount.deref())
      param->first->m_imageBuffers.put(*param->second);

    av_free(filterBuffer);
    delete param;
  }

  void AVDecoderFFMPEG::D::checkSeek(double & nextVideoDpts, double & videoDpts, double & nextAudioDpts)
  {
    if((m_seekRequest.type != SeekNone)) {
      if(seek()) {
        m_loopOffset = 0;
        nextVideoDpts = std::numeric_limits<double>::quiet_NaN();
        nextAudioDpts = std::numeric_limits<double>::quiet_NaN();
        videoDpts = std::numeric_limits<double>::quiet_NaN();
      }
      m_seekRequest.type = SeekNone;
    }
  }

  AVDecoderFFMPEG::AVDecoderFFMPEG()
    : m_d(new D(this))
  {
    Thread::setName("AVDecoderFFMPEG");
  }

  AVDecoderFFMPEG::~AVDecoderFFMPEG()
  {
    /// @todo We might be forgetting something here, some buffers might leak?
    close();
    while(true) {
      AVFilterBufferRef ** ref = m_d->m_consumedBufferRefs.readyItem();
      if(!ref) break;
      avfilter_unref_buffer(*ref);
      m_d->m_consumedBufferRefs.next();
    }
    if(isRunning())
      waitEnd();
    m_d->close();
    delete m_d;
  }

  AVDecoder::PlayMode AVDecoderFFMPEG::playMode() const
  {
    return m_d->m_options.playMode;
  }

  void AVDecoderFFMPEG::setPlayMode(AVDecoder::PlayMode mode)
  {
    if(m_d->m_options.playMode == mode)
      return;

    m_d->m_options.playMode = mode;
    if(m_d->m_audioTransfer)
      m_d->m_audioTransfer->setPlayMode(mode);
    if(mode == Pause)
      m_d->m_pauseTimestamp = Radiant::TimeStamp::currentTime();
    if(mode == Play)
      m_d->m_radiantTimestampToPts -= m_d->m_pauseTimestamp.sinceSecondsD();
  }

  Timestamp AVDecoderFFMPEG::getTimestampAt(const Radiant::TimeStamp & ts) const
  {
    if(m_d->m_realTimeSeeking && m_d->m_av.videoCodec) {
      VideoFrameFFMPEG * frame = m_d->m_decodedVideoFrames.lastReadyItem();
      if(frame)
        return Timestamp(frame->timestamp.pts + 0.0001, m_d->m_seekGeneration);
    }

    if(m_d->m_audioTransfer && !m_d->m_audioTrackHasEnded) {
      Timestamp t = m_d->m_audioTransfer->toPts(ts);
      if(t.seekGeneration < m_d->m_seekGeneration)
        return Timestamp();
      return t;
    }

    if(Nimble::Math::isNAN(m_d->m_radiantTimestampToPts))
      return Timestamp();

    if(m_d->m_options.playMode == Pause)
      return Timestamp(m_d->m_pauseTimestamp.secondsD() + m_d->m_radiantTimestampToPts, m_d->m_seekGeneration);

    return Timestamp(ts.secondsD() + m_d->m_radiantTimestampToPts, m_d->m_seekGeneration);
  }

  Timestamp AVDecoderFFMPEG::latestDecodedTimestamp() const
  {
    VideoFrameFFMPEG * frame = m_d->m_decodedVideoFrames.lastReadyItem();
    if (frame) {
      return frame->timestamp;
    } else return Timestamp();
  }

  VideoFrame * AVDecoderFFMPEG::getFrame(const Timestamp & ts) const
  {
    VideoFrameFFMPEG * ret = 0;
    for(int i = 0;; ++i) {
      VideoFrameFFMPEG * frame = m_d->m_decodedVideoFrames.readyItem(i);
      if(!frame) break;

      if(frame->timestamp.seekGeneration < ts.seekGeneration)
        continue;

      if(frame->timestamp.pts > ts.pts) {
        if(ret) return ret;
        return frame;
      }
      ret = frame;
    }
    // Radiant::warning("Frame is late");
    return ret;
  }

  int AVDecoderFFMPEG::releaseOldVideoFrames(const Timestamp & ts, bool * eof)
  {
    int frameIndex = 0;
    for(;; ++frameIndex) {
      VideoFrameFFMPEG * frame = m_d->m_decodedVideoFrames.readyItem(frameIndex);
      if(!frame) break;

      if(frame->timestamp.seekGeneration >= ts.seekGeneration &&
         frame->timestamp.pts > ts.pts)
        break;
    }

    // always keep one frame alive
    --frameIndex;

    for(int i = 0; i < frameIndex; ++i) {
      VideoFrameFFMPEG * frame = m_d->m_decodedVideoFrames.readyItem();
      assert(frame);

      DecodedImageBuffer * buffer = frame->imageBuffer;
      if(buffer && !buffer->refcount.deref())
        m_d->m_imageBuffers.put(*buffer);

      if(frame->bufferRef) {
        AVFilterBufferRef ** ref = m_d->m_consumedBufferRefs.takeFree();
        if(ref) {
          *ref = frame->bufferRef;
          m_d->m_consumedBufferRefs.put();
        } else {
          Radiant::error("AVDecoderFFMPEG::releaseOldVideoFrames # consumedBufferRefs is full, leaking memory");
        }
        frame->bufferRef = nullptr;
      }

      m_d->m_decodedVideoFrames.next();
    }

    if(eof) {
      *eof = finished() && (!m_d->m_audioTransfer || m_d->m_audioTransfer->bufferStateSeconds() <= 0.0f)
          && m_d->m_decodedVideoFrames.itemCount() <= 1;
    }

    return frameIndex;
  }

  Nimble::Matrix4f AVDecoderFFMPEG::yuvMatrix() const
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

  void AVDecoderFFMPEG::panAudioTo(Nimble::Vector2f location) const
  {
    if (m_d->m_audioTransfer) {
      char buf[128];

      Radiant::BinaryData control;

      control.writeString("panner/setsourcelocation");

      snprintf(buf, sizeof(buf), "%s-%d", m_d->m_audioTransfer->id().data(), (int) 0);

      control.writeString(buf);
      control.writeVector2Float32(location); // sound source location

      Resonant::DSPNetwork::instance()->send(control);
    }
  }

  void AVDecoderFFMPEG::audioTransferDeleted()
  {
    m_d->m_audioTransfer = nullptr;
  }

  void AVDecoderFFMPEG::load(const Options & options)
  {
    assert(!isRunning());
    m_d->m_options = options;
    m_d->updateSupportedPixFormats();
    seek(m_d->m_options.seek);
  }

  void AVDecoderFFMPEG::close()
  {
    m_d->m_running = false;
  }

  Nimble::Size AVDecoderFFMPEG::videoSize() const
  {
    return m_d->m_av.videoSize;
  }

  void AVDecoderFFMPEG::setLooping(bool doLoop)
  {
    m_d->m_options.loop = doLoop;
  }

  double AVDecoderFFMPEG::duration() const
  {
    return m_d->m_av.duration;
  }

  void AVDecoderFFMPEG::seek(const SeekRequest & req)
  {
    m_d->m_seekRequest = req;
  }

  void AVDecoderFFMPEG::setRealTimeSeeking(bool value)
  {
    m_d->m_realTimeSeeking = value;
    if(m_d->m_audioTransfer)
      m_d->m_audioTransfer->setSeeking(value);
  }

  void AVDecoderFFMPEG::childLoop()
  {
    QByteArray errorMsg("AVDecoderFFMPEG::D::childLoop # " + m_d->m_options.src.toUtf8() + ":");
    QThread::currentThread()->setPriority(QThread::LowPriority);

    QByteArray src = m_d->m_options.src.toUtf8();
    s_src = src.data();

    ffmpegInit();

    if(!m_d->open()) {
      state() = STATE_ERROR;
      return;
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

    m_d->m_pauseTimestamp = Radiant::TimeStamp::currentTime();
    bool waitingFrame = false;
    while(m_d->m_running) {
      m_d->m_decodedVideoFrames.setSize(m_d->m_options.videoBufferFrames);

      while(true) {
        AVFilterBufferRef ** ref = m_d->m_consumedBufferRefs.readyItem();
        if(!ref) break;
        avfilter_unref_buffer(*ref);
        m_d->m_consumedBufferRefs.next();
      }

      int err = 0;

      if(!waitingFrame || !m_d->m_realTimeSeeking)
        m_d->checkSeek(nextVideoDpts, videoDpts, nextAudioDpts);

      if(m_d->m_running && m_d->m_realTimeSeeking && av.videoCodec) {
        VideoFrameFFMPEG * frame = m_d->m_decodedVideoFrames.lastReadyItem();
        if(frame && frame->timestamp.seekGeneration == m_d->m_seekGeneration) {
          Radiant::Sleep::sleepMs(1);
          continue;
        }
      }

      if(eof == EofState::Normal) {
        err = av_read_frame(av.formatContext, &av.packet);
      }

      if(err < 0) {
        /// @todo if we are reading a socket-based stream, it might be possible
        ///       to get eof if our input buffer just ends. We should now call
        ///       read_packet to make sure we actually are at eof
        if(err != AVERROR_EOF) {
          avError(QString("%1 Read error").arg(errorMsg.data()), err);
          state() = STATE_ERROR;
          s_src = nullptr;
          return;
        }

        if(av.needFlushAtEof) {
          eof = EofState::Flush;
        } else {
          eof = EofState::Eof;
        }
      }

      // We really are at the end of the stream and we have flushed all the packages
      if(eof == EofState::Eof) {
        if(m_d->m_realTimeSeeking) {
          Radiant::Sleep::sleepMs(1);
          continue;
        }
        if(m_d->m_options.loop) {
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
      bool gotFrames = false;
      double audioDpts = std::numeric_limits<double>::quiet_NaN();

      if(av.videoCodec && (
           (eof == EofState::Normal && av.packet.stream_index == av.videoStreamIndex) ||
           (eof == EofState::Flush && (av.videoCodec->capabilities & CODEC_CAP_DELAY)))) {
        if(eof == EofState::Flush) {
          av_init_packet(&av.packet);
          av.packet.data = nullptr;
          av.packet.size = 0;
          av.packet.stream_index = av.videoStreamIndex;
        }
        gotFrames = m_d->decodeVideoPacket(videoDpts, nextVideoDpts);
      }

      av.frame->opaque = nullptr;
      if(/*!m_d->realTimeSeeking &&*/ av.audioCodec && (
           (eof == EofState::Normal && av.packet.stream_index == av.audioStreamIndex) ||
           (eof == EofState::Flush && (av.audioCodec->capabilities & CODEC_CAP_DELAY)))) {
        if(eof == EofState::Flush) {
          av_init_packet(&av.packet);
          av.packet.data = nullptr;
          av.packet.size = 0;
          av.packet.stream_index = av.audioStreamIndex;
        }
        gotFrames |= m_d->decodeAudioPacket(audioDpts, nextAudioDpts);
      }

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

      // Free the packet that was allocated by av_read_frame
      av_free_packet(&av.packet);

      if (gotFrames)
        state() = STATE_READY;

      if (m_d->m_audioTransfer) {
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
            m_d->m_radiantTimestampToPts = m_d->m_audioTransfer->toPts(Radiant::TimeStamp(0)).pts;
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
  }

  void ffmpegInit()
  {
    MULTI_ONCE {
      av_log_set_callback(ffmpegLog);
      avcodec_register_all();
      avdevice_register_all();
      av_register_all();
      avformat_network_init();
      avfilter_register_all();

      int err = av_lockmgr_register(ffmpegLock);
      if(err != 0)
        Radiant::error("ffmpegInit # Failed to register new FFMPEG lock manager");
    }
  }
}
