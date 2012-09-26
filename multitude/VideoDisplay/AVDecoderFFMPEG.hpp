#ifndef VIDEODISPLAY_AVDECODER_FFMPEG_HPP
#define VIDEODISPLAY_AVDECODER_FFMPEG_HPP

#include "AVDecoder.hpp"

namespace VideoPlayer2
{
  VIDEODISPLAY_API void ffmpegInit();

  class AVDecoderFFMPEG : public AVDecoder
  {
  public:
    AVDecoderFFMPEG();
    ~AVDecoderFFMPEG();

    virtual void close() OVERRIDE;

    virtual PlayMode playMode() const OVERRIDE;
    virtual void setPlayMode(PlayMode mode) OVERRIDE;

    virtual void seek(const SeekRequest & req) OVERRIDE;
    virtual void setRealTimeSeeking(bool value) OVERRIDE;

    virtual Nimble::Vector2i videoSize() const OVERRIDE;

    virtual void setLooping(bool doLoop) OVERRIDE;

    virtual double duration() const OVERRIDE;

    virtual Timestamp getTimestampAt(const Radiant::TimeStamp & ts) const OVERRIDE;
    virtual Timestamp latestDecodedTimestamp() const OVERRIDE;
    virtual VideoFrame * getFrame(const Timestamp & ts) const OVERRIDE;
    virtual void releaseOldVideoFrames(const Timestamp & ts, bool * eof = nullptr) OVERRIDE;

    virtual Nimble::Matrix4f yuvMatrix() const OVERRIDE;


    void audioTransferDeleted();
  protected:
    virtual void load(const Options & options) OVERRIDE;

    virtual void childLoop() OVERRIDE;

  private:
    class D;
    D * m_d;
  };
}

#endif // VIDEODISPLAY_AVDECODER_FFMPEG_HPP
