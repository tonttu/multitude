#ifndef FFMPEGDECODER_HPP
#define FFMPEGDECODER_HPP

#include "AVDecoder.hpp"

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
    FfmpegDecoder();
    ~FfmpegDecoder();

    virtual void close() OVERRIDE;

    virtual PlayMode playMode() const OVERRIDE;
    virtual void setPlayMode(PlayMode mode) OVERRIDE;

    virtual int seek(const SeekRequest & req) OVERRIDE;
    virtual bool realTimeSeeking() const OVERRIDE;
    virtual void setRealTimeSeeking(bool value) OVERRIDE;

    virtual Nimble::Size videoSize() const OVERRIDE;

    virtual bool isLooping() const OVERRIDE;
    virtual void setLooping(bool doLoop) OVERRIDE;

    virtual double duration() const OVERRIDE;

    virtual Timestamp getTimestampAt(const Radiant::TimeStamp & ts) const OVERRIDE;
    virtual Timestamp latestDecodedVideoTimestamp() const OVERRIDE;
    virtual VideoFrame * getFrame(const Timestamp & ts, ErrorFlags & errors) const OVERRIDE;
    virtual int releaseOldVideoFrames(const Timestamp & ts, bool * eof = nullptr) OVERRIDE;

    virtual Nimble::Matrix4f yuvMatrix() const OVERRIDE;

    virtual QByteArray audioPannerSourceId() const override;
    virtual void setAudioGain(float gain) OVERRIDE;

    virtual void setMinimizeAudioLatency(bool minimize) override;

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
