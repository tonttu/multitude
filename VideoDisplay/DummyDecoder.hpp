/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

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

    virtual std::shared_ptr<VideoFrame> playFrame(Radiant::TimeStamp presentTimestamp, ErrorFlags & errors,
                                                  PlayFlags flags) override;
    virtual std::shared_ptr<VideoFrame> peekFrame(std::shared_ptr<VideoFrame> ref, int offset) override;
    virtual bool isEof() const override;

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
