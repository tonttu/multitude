#ifndef FFMPEGDECODER_HPP
#define FFMPEGDECODER_HPP

#include "AVDecoder.hpp"

#include <functional>

struct AVFrame;
struct AVFormatContext;

namespace VideoDisplay
{

  /// Initialize Ffmpeg. This is called automatically from FfmpegDecoder, but
  /// should also be called manually if there is a need to call raw Ffmpeg
  /// functions outside VideoDisplay library.
  /// This will:
  ///  * Register Cornerstone log handlers
  ///  * Register Cornerstone lock manager
  ///  * Initialize avcodec, avdevice, libavformat, avformat_network and avfilter
  VIDEODISPLAY_API void ffmpegInit();

  typedef std::shared_ptr<AVFormatContext> AVFormatContextPtr;

  /// Holds a single ffmpeg video frame and keeps the decoder context alive so
  /// that the frame can freely be used and deleted.
  struct AVFrameWrapper
  {
    AVFrameWrapper(const AVFrameWrapper & copied) = delete;
    AVFrameWrapper & operator=(const AVFrameWrapper & copied) = delete;

    AVFrameWrapper(AVFrameWrapper && moved)
      : avframe(moved.avframe)
      , referenced(moved.referenced)
      , context(std::move(moved.context))
    {
      moved.avframe = nullptr;
      moved.referenced = false;
    }

    AVFrameWrapper & operator=(AVFrameWrapper && moved)
    {
      std::swap(avframe, moved.avframe);
      std::swap(referenced, moved.referenced);
      std::swap(context, moved.context);
      return *this;
    }

    AVFrameWrapper() = default;

    AVFrameWrapper(AVFrame * frame, bool ref, AVFormatContextPtr ctx)
      : avframe(frame)
      , referenced(ref)
      , context(std::move(ctx))
    {}

    VIDEODISPLAY_API ~AVFrameWrapper();

    AVFrame * avframe = nullptr;
    bool referenced = false;
    AVFormatContextPtr context;
  };

  struct DeallocatedFrames
  {
    std::vector<AVFrameWrapper> frames;
    Radiant::Mutex mutex;
  };

  class VideoFrameFfmpeg : public VideoFrame
  {
  public:
    ~VideoFrameFfmpeg();

    std::weak_ptr<DeallocatedFrames> deallocatedFrames;
    bool frameUnrefMightBlock = false;
    AVFrameWrapper frame;
  };

  /// Audio/Video decoder implementation that uses Ffmpeg as a backend
  class FfmpegDecoder : public AVDecoder
  {
  public:
    /// Parameters:
    /// int level (see https://ffmpeg.org/doxygen/3.1/group__lavu__log__constants.html)
    /// const char * message
    /// Return value true means that the message was handled and won't be
    /// forwarded to Radiant trace functions.
    typedef std::function<bool(int, const char*)> LogHandler;

    /// Set temporary log handler for this thread only
    static void setTlsLogHandler(const LogHandler * handlerFunc);

  public:
    FfmpegDecoder();
    ~FfmpegDecoder();

    virtual void close() OVERRIDE;

    virtual AVSync::PlayMode playMode() const OVERRIDE;
    virtual void setPlayMode(AVSync::PlayMode mode) OVERRIDE;

    virtual int seek(const SeekRequest & req) OVERRIDE;
    virtual bool realTimeSeeking() const OVERRIDE;
    virtual bool setRealTimeSeeking(bool value) OVERRIDE;

    virtual Nimble::Size videoSize() const OVERRIDE;

    virtual bool isLooping() const OVERRIDE;
    virtual bool setLooping(bool doLoop) OVERRIDE;

    virtual double duration() const OVERRIDE;

    virtual std::shared_ptr<VideoFrame> playFrame(Radiant::TimeStamp presentTimestamp, ErrorFlags & errors,
                                                  PlayFlags flags) override;
    virtual std::shared_ptr<VideoFrame> peekFrame(std::shared_ptr<VideoFrame> ref, int offset) override;
    virtual bool isEof() const override;

    virtual Nimble::Matrix4f yuvMatrix() const OVERRIDE;

    virtual QByteArray audioPannerSourceId() const override;
    virtual bool setAudioGain(float gain) OVERRIDE;

    virtual QString source() const override;

    /// @cond

    /// Called from AudioTransfer::~AudioTransfer
    /// @todo we should make non-intrusive AudioTransfer monitoring instead of this
    virtual void audioTransferDeleted() OVERRIDE;

    /// @endcond

  protected:
    virtual void load(const Options & options) OVERRIDE;

    virtual void runDecoder() OVERRIDE;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };

}

#endif // FFMPEGDECODER_HPP
