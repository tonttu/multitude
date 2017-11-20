/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "AudioTransfer.hpp"

#include "AVDecoder.hpp"

#include <Resonant/AudioLoop.hpp>

namespace
{
  static const int s_sampleRate = 44100;

  void zero(float ** dest, int channels, int frames, int offset)
  {
    assert(frames >= 0);

    for(int channel = 0; channel < channels; ++channel)
      memset(dest[channel] + offset, 0, frames * sizeof(float));
  }
}

namespace VideoDisplay
{
  uint64_t s_bufferUnderrun = 0;
  const int s_decodedBufferCount = 200;

  void DecodedAudioBuffer::fill(Timestamp timestamp, int channels, int samples,
                                const int16_t * interleavedData)
  {
    m_timestamp = timestamp;
    m_offset = 0;
    m_data.resize(channels);

    const float gain = 1.0f;
    const float factor = (gain / (1 << 16));

    for(int c = 0; c < channels; ++c) {
      m_data[c].resize(samples);
      float * destination = m_data[c].data();
      const int16_t * source = interleavedData + c;
      for(int s = 0; s < samples; ++s) {
        *destination++ = *source * factor;
        source += channels;
      }
    }
  }

  /// @todo should do this without copying any data! (using libav buffer refs)
  void DecodedAudioBuffer::fillPlanar(Timestamp timestamp, int channels,
                                      int samples, const float ** src)
  {
    m_timestamp = timestamp;
    m_offset = 0;
    m_data.resize(channels);

    for(int c = 0; c < channels; ++c) {
      m_data[c].resize(samples);
      memcpy(m_data[c].data(), src[c], samples*sizeof(float));
    }
  }

  class AudioTransfer::D
  {
  public:
    D(AVDecoder * avff, int channels)
      : m_avff(avff)
      , m_channels(channels)
      , m_seekGeneration(0)
      , m_playMode(AVDecoder::PAUSE)
      , m_seeking(false)
      , m_decodedBuffers(s_decodedBufferCount)
      , m_buffersReader(0)
      , m_buffersWriter(0)
      , m_resonantToPts(0)
      , m_usedSeekGeneration(0)
      , m_samplesInGeneration(0)
      , m_gain(1.0f)
      , m_enabled(true)
      , m_decodingFinished(false)
      /*, samplesProcessed(0)*/
    {}

    AVDecoder * m_avff;
    const int m_channels;
    int m_seekGeneration;
    AVDecoder::PlayMode m_playMode;
    bool m_seeking;

    Timestamp m_pts;

    std::vector<DecodedAudioBuffer> m_decodedBuffers;

    // decodedBuffers[buffersReader % s_decodedBufferCount] points to the next
    // buffer that could be processed to Resonant (iff readyBuffers > 0).
    // Only used from process()
    int m_buffersReader;
    // decodedBuffers[buffersWriter % s_decodedBufferCount] points to the next
    // buffer that could be filled with new decoded audio from the AVDecoder
    int m_buffersWriter;

    QAtomicInt m_readyBuffers;
    QAtomicInt m_samplesInBuffers;

    double m_resonantToPts;
    int m_usedSeekGeneration;
    int m_samplesInGeneration;

    float m_gain;
    bool m_enabled;
    bool m_decodingFinished;
    /*long samplesProcessed;*/

    // Should we try to minimize the latency by dropping samples. This should
    // only be used with streaming sources
    bool m_minimizeLatency = false;

    // Maximum latency (in frames), when m_minimizeLatency is enabled
    int m_maxLatency = 0.020 * s_sampleRate;

    // Measured minimum latency == extra buffer size
    int m_minLatency = 0;
    // How many frames we have gathered latency information
    int m_latencyFrames = 0;

    DecodedAudioBuffer * getReadyBuffer();
    void bufferConsumed(int samples, bool buffer);
  };

  DecodedAudioBuffer * AudioTransfer::D::getReadyBuffer()
  {
    while (m_readyBuffers.load() > 0) {
      DecodedAudioBuffer * buffer = & m_decodedBuffers[m_buffersReader % s_decodedBufferCount];
      if(buffer->timestamp().seekGeneration() < m_seekGeneration) {
        m_samplesInBuffers.fetchAndAddRelaxed(-(buffer->samples() - buffer->offset()));
        m_readyBuffers.deref();
        ++m_buffersReader;
        continue;
      }
      /// @todo shouldn't be hard-coded
      if(m_seeking && m_samplesInGeneration > s_sampleRate/24.0)
        return nullptr;
      return buffer;
    }
    return nullptr;
  }

  void AudioTransfer::D::bufferConsumed(int samples, bool buffer)
  {
    if (buffer) {
      m_readyBuffers.deref();
      m_samplesInBuffers.fetchAndAddRelaxed(-samples);
      ++m_buffersReader;
    } else {
      m_samplesInBuffers.fetchAndAddRelaxed(-samples);
    }
  }

  AudioTransfer::AudioTransfer(AVDecoder * avff, int channels)
    : m_d(new D(avff, channels))
  {
    assert(channels > 0);
  }

  AudioTransfer::~AudioTransfer()
  {
    if(m_d->m_avff) {
      m_d->m_avff->audioTransferDeleted();
    }
    // Radiant::info("AudioTransfer::~AudioTransfer # %p", this);
    delete m_d;
  }

  bool AudioTransfer::prepare(int & channelsIn, int & channelsOut)
  {
    channelsIn = 0;
    channelsOut = m_d->m_channels;
    return true;
  }

  void AudioTransfer::process(float **, float ** out, int n, const Resonant::CallbackTime & time)
  {
    if (!m_d->m_enabled) {
      zero(out, m_d->m_channels, n, 0);
      return;
    }

    if (m_d->m_minimizeLatency && m_d->m_playMode == AVDecoder::PLAY) {
      const int latencyCheckSecs = 3;

      if (m_d->m_latencyFrames == 0) {
        m_d->m_minLatency = m_d->m_samplesInBuffers.load();
        m_d->m_latencyFrames = n;
      } else {
        m_d->m_minLatency = std::min(m_d->m_samplesInBuffers.load(), m_d->m_minLatency);
        m_d->m_latencyFrames += n;
      }

      if (m_d->m_latencyFrames >= s_sampleRate * latencyCheckSecs) {
        m_d->m_latencyFrames = 0;

        if (m_d->m_minLatency > m_d->m_maxLatency) {
          // Drop frames to reduce latency
          int framesToDrop = m_d->m_minLatency - m_d->m_maxLatency;

          while (framesToDrop > 0) {
            if (DecodedAudioBuffer * decodedBuffer = m_d->getReadyBuffer()) {
              const int offset = decodedBuffer->offset();
              const int samples = std::min<int>(framesToDrop, decodedBuffer->samples() - offset);

              framesToDrop -= samples;

              if (offset + samples == decodedBuffer->samples()) {
                m_d->bufferConsumed(samples, true);
              } else {
                m_d->bufferConsumed(samples, false);
                decodedBuffer->setOffset(offset + samples);
              }
            } else {
              break;
            }
          }
        }
      }
    }

    /**
     * We need to implement toPts -function, that converts absolute
     * timestamp (Radiant::TimeStamp) to video pts (presentation timestamp).
     * This is used by AV-synchronization code, VideoWidget has an estimate
     * Radiant::TimeStamp of the time when the currently rendered frame will be
     * actually displayed on the screen. In the audio thread we save a offset
     * value that can be used to make the actual conversion between
     * Radiant::TimeStamp and pts.
     */

    int processed = 0;
    int remaining = n;

    bool first = true;

    while(remaining > 0) {
      DecodedAudioBuffer * decodedBuffer = m_d->getReadyBuffer();
      if (decodedBuffer && m_d->m_playMode == AVDecoder::PAUSE &&
          decodedBuffer->timestamp().seekGeneration() == m_d->m_usedSeekGeneration) {
        // Paused, and decoded buffer seek generation matches used seek generation, nothing to do
        decodedBuffer = nullptr;
      }

      if (decodedBuffer) {
        const int offset = decodedBuffer->offset();
        const int samples = std::min<int>(remaining, decodedBuffer->samples() - offset);

        const Timestamp ts = decodedBuffer->timestamp();

        // Presentation time of the decoded audio buffer, at the time
        // when currently processed audio will be written to audio hardware
        const double pts = ts.pts() + offset / double(s_sampleRate);

        m_d->m_pts = ts;
        m_d->m_pts.setPts(pts + samples / double(s_sampleRate));

        if(first) {
          // We can convert Resonant times to pts with this offset
          m_d->m_resonantToPts = pts - time.outputTime.secondsD();
          m_d->m_usedSeekGeneration = ts.seekGeneration();

          first = false;
        }

        if (m_d->m_playMode == AVDecoder::PLAY) {
          m_d->m_samplesInGeneration += samples;

          // We could take the gain into account in the preprocessing stage,
          // in another thread with more resources, but then changing gain
          // would have a noticeably latency
          const float gain = m_d->m_seeking ? m_d->m_gain * 0.35 : m_d->m_gain;
          if(std::abs(gain - 1.0f) < 1e-5f) {
            for(int channel = 0; channel < m_d->m_channels; ++channel)
              memcpy(out[channel] + processed, decodedBuffer->data(channel) + offset, samples * sizeof(float));
          } else {
            for(int channel = 0; channel < m_d->m_channels; ++channel) {
              float * destination = out[channel] + processed;
              const float * source = decodedBuffer->data(channel) + offset;
              for(int s = 0; s < samples; ++s)
                *destination++ = *source++ * gain;
            }
          }

          processed += samples;
          remaining -= samples;

          if(offset + samples == decodedBuffer->samples()) {
            m_d->bufferConsumed(samples, true);
          } else {
            m_d->bufferConsumed(samples, false);
            decodedBuffer->setOffset(offset + samples);
          }
        } else {
          // We don't want to write this to the output, we just wanted to
          // remove old samples with old seek generation from the queue
          decodedBuffer = nullptr;
        }
      }

      if (!decodedBuffer) {
        zero(out, m_d->m_channels, remaining, processed);
        if (m_d->m_decodingFinished) {
          setEnabled(false);
        } else {
          s_bufferUnderrun += remaining;
        }
        break;
      }
    }
  }

  Timestamp AudioTransfer::toPts(const Radiant::TimeStamp & ts) const
  {
    const Timestamp newts(ts.secondsD() + m_d->m_resonantToPts, m_d->m_usedSeekGeneration);
    return std::min(m_d->m_pts, newts);
  }

  Timestamp AudioTransfer::lastPts() const
  {
    return m_d->m_pts;
  }

  float AudioTransfer::bufferStateSeconds() const
  {
    /// @todo shouldn't be hard-coded
    return m_d->m_samplesInBuffers.load() / float(s_sampleRate);
  }

  void AudioTransfer::shutdown()
  {
    m_d->m_avff = nullptr;
  }

  bool AudioTransfer::isShutdown() const
  {
    return m_d->m_avff == nullptr;
  }

  DecodedAudioBuffer * AudioTransfer::takeFreeBuffer(int samples)
  {
    if(m_d->m_readyBuffers.load() >= int(m_d->m_decodedBuffers.size()))
      return nullptr;

    if(m_d->m_samplesInBuffers.load() > samples)
      return nullptr;

    int b = m_d->m_buffersWriter++;

    return & m_d->m_decodedBuffers[b % s_decodedBufferCount];
  }

  void AudioTransfer::putReadyBuffer(int samples)
  {
    m_d->m_readyBuffers.ref();
    m_d->m_samplesInBuffers.fetchAndAddRelaxed(samples);
  }

  void AudioTransfer::setPlayMode(AVDecoder::PlayMode playMode)
  {
    m_d->m_playMode = playMode;
  }

  void AudioTransfer::setSeeking(bool seeking)
  {
    m_d->m_seeking = seeking;
  }

  void AudioTransfer::setSeekGeneration(int seekGeneration)
  {
    if (m_d->m_seekGeneration != seekGeneration) {
      m_d->m_samplesInGeneration = 0;
      m_d->m_seekGeneration = seekGeneration;
      m_d->m_latencyFrames = 0;
    }
  }

  float AudioTransfer::gain() const
  {
    return m_d->m_gain;
  }

  void AudioTransfer::setGain(float gain)
  {
    m_d->m_gain = gain;
  }

  void AudioTransfer::setEnabled(bool enabled)
  {
    m_d->m_enabled = enabled;
  }

  bool AudioTransfer::isEnabled() const
  {
    return m_d->m_enabled;
  }

  void AudioTransfer::setDecodingFinished(bool finished)
  {
    m_d->m_decodingFinished = finished;
  }

  bool AudioTransfer::isDecodingFinished() const
  {
    return m_d->m_decodingFinished;
  }

  void AudioTransfer::setMinimizeLatency(bool minimize)
  {
    m_d->m_minimizeLatency = minimize;
  }

  bool AudioTransfer::minimizeLatency() const
  {
    return m_d->m_minimizeLatency;
  }

  uint64_t AudioTransfer::bufferUnderrun()
  {
    return s_bufferUnderrun;
  }
}
