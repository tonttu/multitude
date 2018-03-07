#ifndef VIDEO_DISPLAY_DUMMY_DECODER_HPP
#define VIDEO_DISPLAY_DUMMY_DECODER_HPP

#include "AVDecoder.hpp"

namespace VideoDisplay
{
  /// Decoder that just generates noise
  class DummyDecoder : public AVDecoder
  {
  public:
    DummyDecoder();
    virtual ~DummyDecoder();

    virtual void close() override;

    virtual AVSync::PlayMode playMode() const override;
    virtual void setPlayMode(AVSync::PlayMode mode) override;

    virtual int seek(const SeekRequest & req) override;

    virtual Nimble::Size videoSize() const override;

    virtual VideoFrame * playFrame(Radiant::TimeStamp presentTimestamp, ErrorFlags & errors,
                                   PlayFlags flags) override;
    virtual int releaseOldVideoFrames(const Timestamp & ts, bool * eof = nullptr) override;

    virtual Nimble::Matrix4f yuvMatrix() const override;

    virtual QString source() const override;

  protected:
    virtual void load(const Options & options) override;
    virtual void runDecoder() override;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
} // namespace VideoDisplay

#endif // VIDEO_DISPLAY_DUMMY_DECODER_HPP
