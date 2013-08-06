/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VIDEODISPLAY_LIBAV_DECODER_HPP
#define VIDEODISPLAY_LIBAV_DECODER_HPP

#include "AVDecoder.hpp"

namespace VideoDisplay
{
  /// Initialize Libav. This is called automatically from LibavDecoder, but
  /// should also be called manually if there is a need to call raw Libav
  /// functions outside VideoDisplay library.
  /// This will:
  ///  * Register Cornerstone log handlers
  ///  * Register Cornerstone lock manager
  ///  * Initialize avcodec, avdevice, libavformat, avformat_network and avfilter
  VIDEODISPLAY_API void libavInit();

  /// Audio/Video decoder implementation that uses Libav as a backend
  class LibavDecoder : public AVDecoder
  {
  public:
    LibavDecoder();
    ~LibavDecoder();

    virtual void close() OVERRIDE;

    virtual PlayMode playMode() const OVERRIDE;
    virtual void setPlayMode(PlayMode mode) OVERRIDE;

    virtual void seek(const SeekRequest & req) OVERRIDE;
    virtual bool realTimeSeeking() const OVERRIDE;
    virtual void setRealTimeSeeking(bool value) OVERRIDE;

    virtual Nimble::Size videoSize() const OVERRIDE;

    virtual bool isLooping() const OVERRIDE;
    virtual void setLooping(bool doLoop) OVERRIDE;

    virtual double duration() const OVERRIDE;

    virtual Timestamp getTimestampAt(const Radiant::TimeStamp & ts) const OVERRIDE;
    virtual Timestamp latestDecodedVideoTimestamp() const OVERRIDE;
    virtual VideoFrame * getFrame(const Timestamp & ts) const OVERRIDE;
    virtual int releaseOldVideoFrames(const Timestamp & ts, bool * eof = nullptr) OVERRIDE;

    virtual Nimble::Matrix4f yuvMatrix() const OVERRIDE;

    virtual void panAudioTo(Nimble::Vector2f location) const OVERRIDE;

    virtual void setAudioGain(float gain) OVERRIDE;

    /// @cond

    /// Called from AudioTransfer::~AudioTransfer
    /// @todo we should make non-intrusive AudioTransfer monitoring instead of this
    void audioTransferDeleted();

    /// @endcond

  protected:
    virtual void load(const Options & options) OVERRIDE;

    virtual void runDecoder() OVERRIDE;

  private:
    class D;
    D * m_d;
  };
}

#endif // VIDEODISPLAY_LIBAV_DECODER_HPP
