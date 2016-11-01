/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VIDEODISPLAY_AUDIO_TRANSFER2_HPP
#define VIDEODISPLAY_AUDIO_TRANSFER2_HPP

#include "AVDecoder.hpp"

#include <Radiant/Allocators.hpp>
#include <Radiant/IODefs.hpp>

#include <Resonant/Module.hpp>

/// @cond

namespace VideoDisplay
{
  class DecodedAudioBuffer
  {
  public:
    DecodedAudioBuffer() : m_timestamp(), m_offset(0) {}

    void fill(Timestamp timestamp, int channels, int samples, const int16_t * src);
    void fillPlanar(Timestamp timestamp, int channels, int samples, const float ** src);

    Timestamp timestamp() const { return m_timestamp; }

    int offset() const { return m_offset; }
    void setOffset(int offset) { m_offset = offset; }

    const float * data(unsigned channel) const
    {
      if(channel >= m_data.size()) return 0;
      return m_data[channel].data();
    }

    int samples() const { return m_data.empty() ? 0 : (int)m_data[0].size(); }

  private:
    //DecodedAudioBuffer(const DecodedAudioBuffer &);
    //DecodedAudioBuffer & operator=(DecodedAudioBuffer &);
  private:
    Timestamp m_timestamp;
    int m_offset;

    // vector of channels
    typedef std::vector<float, Radiant::aligned_allocator<float, 32>> AlignedFloatVector;
    std::vector<AlignedFloatVector> m_data;
  };

  class AVDecoder;

  class AudioTransfer : public Resonant::Module
  {
  public:
    AudioTransfer(AVDecoder *, int channels);
    virtual ~AudioTransfer();

    virtual bool prepare(int & channelsIn, int & channelsOut) OVERRIDE;
    virtual void process(float ** in, float ** out, int n, const Resonant::CallbackTime & time) OVERRIDE;

    Timestamp toPts(const Radiant::TimeStamp & ts) const;

    Timestamp lastPts() const;

    float bufferStateSeconds() const;

    void shutdown();
    bool isShutdown() const;

    DecodedAudioBuffer * takeFreeBuffer(int samples);
    void putReadyBuffer(int samples);

    void setPlayMode(AVDecoder::PlayMode playMode);
    void setSeeking(bool seeking);
    void setSeekGeneration(int seekGeneration);

    /// Gain factor for the sound-track
    float gain() const;
    void setGain(float gain);

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void setDecodingFinished(bool finished);
    bool isDecodingFinished() const;

    void setMinimizeLatency(bool minimize);
    bool minimizeLatency() const;

    VIDEODISPLAY_API static uint64_t bufferUnderrun();

  private:
    class D;
    D * m_d;
  };

  typedef std::shared_ptr<AudioTransfer> AudioTransferPtr;
}

/// @endcond

#endif // VIDEODISPLAY_AUDIO_TRANSFER2_HPP
