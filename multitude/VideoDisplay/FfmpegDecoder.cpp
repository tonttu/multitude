// For PRIx64
#define __STDC_FORMAT_MACROS

#include "FfmpegDecoder.hpp"

#include "Utils.hpp"
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
      if (!msg.contains("max_analyze_duration reached") && !msg.contains("First timestamp is missing,")) {
        Radiant::warning("%s", msg.toUtf8().data());
      }
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

  class VideoFrameFfmpeg : public VideoFrame
  {
  public:
    VideoFrameFfmpeg() : VideoFrame(), frame(nullptr) {}
    AVFrame* frame;
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

    bool dr1;
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

    void close();


    FfmpegDecoder * m_host;
    int m_seekGeneration;

    bool m_running;

    MyAV m_av;
    PtsCorrectionContext m_ptsCorrection;

    Utils::MemoryPool<DecodedImageBuffer, 80> m_imageBuffers;

    bool m_realTimeSeeking;
    SeekRequest m_seekRequest;

    AVDecoder::Options m_options;
    Radiant::TimeStamp m_pauseTimestamp;

    QList<AVPixelFormat> m_pixelFormats;

    // only used when there is no audio or the audio track has ended
    double m_radiantTimestampToPts;

    double m_loopOffset;

    float m_audioGain;
    AudioTransferPtr m_audioTransfer;

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
    Utils::LockFreeQueue<AVFrame*, 40> m_consumedBufferRefs;

    Utils::LockFreeQueue<VideoFrameFfmpeg, 40> m_decodedVideoFrames;

    int m_index;
  };


  // -------------------------------------------------------------------------

  FfmpegDecoder::D::D(FfmpegDecoder *decoder)
    : m_host(decoder)
  {
  }

  void FfmpegDecoder::D::close()
  {

  }

  // -------------------------------------------------------------------------


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
    while(true) {

      /// TODO go through consumed buffers and release them

    }
    m_d->close();
  }

  AVDecoder::PlayMode FfmpegDecoder::playMode() const
  {
    return PlayMode(); /// TODO options -> play mode
  }

  void FfmpegDecoder::setPlayMode(AVDecoder::PlayMode mode)
  {
    /// TODO set
  }

  Timestamp FfmpegDecoder::getTimestampAt(const Radiant::TimeStamp &ts) const
  {
    /// TODO timestamp
    return Timestamp();
  }

  Timestamp FfmpegDecoder::latestDecodedVideoTimestamp() const
  {
    return Timestamp();
  }

  VideoFrame* FfmpegDecoder::getFrame(const Timestamp &ts, ErrorFlags &errors) const
  {
    return nullptr;
  }

  int FfmpegDecoder::releaseOldVideoFrames(const Timestamp &ts, bool *eof)
  {
    return 0;
  }

  Nimble::Matrix4f FfmpegDecoder::yuvMatrix() const
  {
    return Nimble::Matrix4::IDENTITY;
  }

  void FfmpegDecoder::panAudioTo(Nimble::Vector2f location) const
  {

  }

  void FfmpegDecoder::setAudioGain(float gain)
  {

  }

  void FfmpegDecoder::audioTransferDeleted()
  {

  }

  void FfmpegDecoder::load(const Options &options)
  {

  }

  void FfmpegDecoder::close()
  {

  }

  Nimble::Size FfmpegDecoder::videoSize() const
  {
    return Nimble::Size();
  }

  bool FfmpegDecoder::isLooping() const
  {
    return false;
  }

  void FfmpegDecoder::setLooping(bool doLoop)
  {
  }

  double FfmpegDecoder::duration() const
  {
    return 0;
  }

  void FfmpegDecoder::seek(const SeekRequest & req)
  {
  }

  bool FfmpegDecoder::realTimeSeeking() const
  {
    return false;
  }

  void FfmpegDecoder::setRealTimeSeeking(bool value)
  {
  }

  void FfmpegDecoder::runDecoder()
  {

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
