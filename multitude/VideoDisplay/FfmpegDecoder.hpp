#ifndef FFMPEGDECODER_HPP
#define FFMPEGDECODER_HPP

#include "AVDecoder.hpp"

#include <functional>

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
    virtual void setRealTimeSeeking(bool value) OVERRIDE;

    virtual Nimble::Size videoSize() const OVERRIDE;

    virtual bool isLooping() const OVERRIDE;
    virtual void setLooping(bool doLoop) OVERRIDE;

    virtual double duration() const OVERRIDE;

    virtual VideoFrame * playFrame(Radiant::TimeStamp presentTimestamp, ErrorFlags & errors,
                                   PlayFlags flags) override;
    virtual int releaseOldVideoFrames(const Timestamp & ts, bool * eof = nullptr) OVERRIDE;

    virtual Nimble::Matrix4f yuvMatrix() const OVERRIDE;

    virtual QByteArray audioPannerSourceId() const override;
    virtual void setAudioGain(float gain) OVERRIDE;

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
