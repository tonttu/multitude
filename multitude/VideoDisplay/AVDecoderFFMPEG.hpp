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

#ifndef VIDEODISPLAY_AVDECODER_FFMPEG_HPP
#define VIDEODISPLAY_AVDECODER_FFMPEG_HPP

#include "AVDecoder.hpp"

namespace VideoDisplay
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

    virtual Nimble::Size videoSize() const OVERRIDE;

    virtual void setLooping(bool doLoop) OVERRIDE;

    virtual double duration() const OVERRIDE;

    virtual Timestamp getTimestampAt(const Radiant::TimeStamp & ts) const OVERRIDE;
    virtual Timestamp latestDecodedTimestamp() const OVERRIDE;
    virtual VideoFrame * getFrame(const Timestamp & ts) const OVERRIDE;
    virtual int releaseOldVideoFrames(const Timestamp & ts, bool * eof = nullptr) OVERRIDE;

    virtual Nimble::Matrix4f yuvMatrix() const OVERRIDE;

    virtual void panAudioTo(Nimble::Vector2f location) const OVERRIDE;

    virtual bool isReady() const OVERRIDE;
    virtual bool hasError() const OVERRIDE;

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
