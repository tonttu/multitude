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
  /// @todo shouldn't be hard-coded
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
    D(AVDecoder * avff, int channels, std::shared_ptr<AVSync> avsync)
      : m_avff(avff)
      , m_channels(channels)
      , m_seekGeneration(0)
      , m_seeking(false)
      , m_decodedBuffers(s_decodedBufferCount)
      , m_buffersReader(0)
      , m_buffersWriter(0)
      , m_usedSeekGeneration(0)
      , m_samplesInGeneration(0)
      , m_gain(1.0f)
      , m_enabled(true)
      , m_decodingFinished(false)
      , m_sync(std::move(avsync))
      /*, samplesProcessed(0)*/
    {}

    AVDecoder * m_avff;
    const int m_channels;
    int m_seekGeneration;
    bool m_seeking;

    std::vector<DecodedAudioBuffer> m_decodedBuffers;
    /// Locked while handling m_decodedBuffers
    Radiant::Mutex m_decodedBuffersMutex;

    // decodedBuffers[buffersReader % s_decodedBufferCount] points to the next
    // buffer that could be processed to Resonant (iff readyBuffers > 0).
    // Only used from process()
    int m_buffersReader;
    // decodedBuffers[buffersWriter % s_decodedBufferCount] points to the next
    // buffer that could be filled with new decoded audio from the AVDecoder
    int m_buffersWriter;

    QAtomicInt m_readyBuffers;
    QAtomicInt m_samplesInBuffers;

    int m_usedSeekGeneration;
    int m_samplesInGeneration;

    float m_gain;
    bool m_enabled;
    bool m_decodingFinished;

    std::shared_ptr<AVSync> m_sync;

    /// If the measured average latency doesn't keep inside these threshold, we
    /// start skipping or dropping samples.
    double m_maxAllowedAverageLatency = 0.2;
    double m_minAllowedAverageLatency = -0.2;

    /// Used for calculating average latency (latency == accumulator / counter)
    double m_averageMeasuredLatencyAccumulator = 0;
    int m_latencySamplesCounter = 0;

    /// Used for calculating the minimum buffer size (in samples) since
    /// m_latencySamplesCounter was set to zero. If we ever need to drop
    /// samples to sync audio, make sure we always have at least
    /// m_minAllowedBufferSamples samples in the buffer.
    int m_minMeasuredBufferSamples = 0;
    int m_minAllowedBufferSamples = 0.020 * s_sampleRate;

    /// Set to true to force process() to perform syncing by dropping or
    /// skipping samples.
    bool m_forceSync = true;

    DecodedAudioBuffer * getReadyBuffer();
    void bufferConsumed(int samples, bool buffer);
    void dropOldSamples(double maxLatency, Radiant::TimeStamp currentTime);
    void dropOldSamples(int samplesToDrop);

    /// Syncs the audio to m_sync, called from process()
    ///
    /// When latency is positive, audio comes after the video. If the
    /// latency is too large for a period of time or if we have been
    /// forced to do sync, we can fix the latency by dropping decoded
    /// samples from the beginning of the buffer.
    ///
    /// If the latency is negative, audio comes before the video.
    /// Fixing this requires us to stop consuming buffers and just
    /// waiting for us to catch up. While doing that we keep
    /// m_forceSync true and just write zeroes to output until we are
    /// done.
    /// @param checkNow should we perform syncing regardless of when we last
    ///        did it or how much error there is. Set this after any
    ///        interruptions in the playback.
    /// @param pts Presentation timestamp in video time
    /// @param samples number of samples we are processing.
    /// @returns The number of samples we would wish process() to skip.
    ///          Positive number means that we write zero to process() output
    ///          buffers and increase the playback latency by doing that.
    ///          Negative number means that we decrease latency by dropping
    ///          old samples from the beginning of the decoded audio buffer.
    int sync(bool checkNow, double pts, int samples, const Resonant::CallbackTime & time);
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

  void AudioTransfer::D::dropOldSamples(double maxLatency, Radiant::TimeStamp currentTime)
  {
    while (DecodedAudioBuffer * decodedBuffer = getReadyBuffer()) {
      const int offset = decodedBuffer->offset();
      const int samples = decodedBuffer->samples() - offset;
      const double endPts = decodedBuffer->timestamp().pts() + samples / double(s_sampleRate);
      const Radiant::TimeStamp presentTs = m_sync->map(endPts);

      const double latency = (currentTime - presentTs).secondsD();
      if (latency >= maxLatency) {
        bufferConsumed(samples, true);
        continue;
      }

      break;
    }
  }

  void AudioTransfer::D::dropOldSamples(int samplesToDrop)
  {
    while (samplesToDrop > 0) {
      if (DecodedAudioBuffer * decodedBuffer = getReadyBuffer()) {
        const int offset = decodedBuffer->offset();
        const int samples = std::min<int>(samplesToDrop, decodedBuffer->samples() - offset);

        samplesToDrop -= samples;

        if (offset + samples == decodedBuffer->samples()) {
          bufferConsumed(samples, true);
        } else {
          bufferConsumed(samples, false);
          decodedBuffer->setOffset(offset + samples);
        }
      } else {
        break;
      }
    }
  }

  int AudioTransfer::D::sync(bool checkNow, double pts, int samples, const Resonant::CallbackTime & time)
  {
    if (!m_sync->isValid())
      return 0;

    const Radiant::TimeStamp presentTs = m_sync->map(pts);
    const double latencySample = (time.outputTime - presentTs).secondsD();
    const int latencyCheckSecs = 3;

    if (m_latencySamplesCounter == 0) {
      m_averageMeasuredLatencyAccumulator = latencySample * samples;
      m_minMeasuredBufferSamples = m_samplesInBuffers.load() - samples;
    } else {
      m_averageMeasuredLatencyAccumulator += latencySample * samples;
      m_minMeasuredBufferSamples = std::min(m_samplesInBuffers.load() - samples,
                                            m_minMeasuredBufferSamples);
    }
    m_latencySamplesCounter += samples;

    if (m_latencySamplesCounter < s_sampleRate * latencyCheckSecs && !checkNow)
      return 0;

    const double averageLatency = m_averageMeasuredLatencyAccumulator / m_latencySamplesCounter;
    m_latencySamplesCounter = 0;

    // Normally we allow the latency to fluctuate a bit, since the
    // timing information we have is not perfect, and every time we
    // do any kind of adjustment, it can be heard as a loud pop or
    // sudden silence in the output. However, if we are forced to do
    // the check, we will then do full sync as well as possible.

    const bool adjustLatency = checkNow || averageLatency < m_minAllowedAverageLatency
        || averageLatency > m_maxAllowedAverageLatency;

    if (!adjustLatency || averageLatency == 0)
      return 0;

    if (averageLatency < 0) {
      // Wait to increase latency (which is now negative)
      return -averageLatency * s_sampleRate;
    }

    if (averageLatency > 0) {
      // Drop samples to reduce latency
      int samplesToDrop = averageLatency * s_sampleRate;
      // We need to check that we don't drop too many samples so that we would
      // run out of buffer.
      int maxSamplesAllowedtoDrop = m_minMeasuredBufferSamples - m_minAllowedBufferSamples;
      if (samplesToDrop == 0 || maxSamplesAllowedtoDrop <= 0)
        return 0;

      return -std::min(samplesToDrop, maxSamplesAllowedtoDrop);
    }

    return 0;
  }

  AudioTransfer::AudioTransfer(AVDecoder * avff, int channels, std::shared_ptr<AVSync> avsync)
    : m_d(new D(avff, channels, std::move(avsync)))
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
    if (m_d->m_sync->playMode() != AVSync::PLAY || !m_d->m_enabled) {
      zero(out, m_d->m_channels, n, 0);
      m_d->m_forceSync = true;
      m_d->m_latencySamplesCounter = 0;
      return;
    }

    Radiant::Guard g(m_d->m_decodedBuffersMutex);

    if (m_d->m_seekGeneration != m_d->m_sync->seekGeneration()) {
      m_d->m_samplesInGeneration = 0;
      m_d->m_seekGeneration = m_d->m_sync->seekGeneration();
      m_d->m_latencySamplesCounter = 0;
      m_d->m_forceSync = true;
    }

    /// When seeking, only play a couple of samples
    /// @todo shouldn't be hard-coded
    if (m_d->m_seeking && m_d->m_samplesInGeneration > s_sampleRate/24.0) {
      zero(out, m_d->m_channels, n, 0);
      return;
    }

    int processed = 0;
    int remaining = n;

    bool first = true;

    while(remaining > 0) {
      DecodedAudioBuffer * decodedBuffer = m_d->getReadyBuffer();
      if (decodedBuffer) {
        const int offset = decodedBuffer->offset();
        const int samples = std::min<int>(remaining, decodedBuffer->samples() - offset);

        const Timestamp ts = decodedBuffer->timestamp();

        // Presentation time of the decoded audio buffer, at the time
        // when currently processed audio will be written to audio hardware
        const double pts = ts.pts() + offset / double(s_sampleRate);

        if (first) {
          bool checkNow = m_d->m_forceSync || time.flags != Resonant::CallbackTime::FLAG_NONE;
          m_d->m_forceSync = false;
          int skipSamples = m_d->sync(checkNow, pts, n, time);
          if (skipSamples > n) {
            /// We would like to skip more samples than we are currently
            /// processing. Skip what we can, and force syncing next time.
            m_d->m_forceSync = true;
            zero(out, m_d->m_channels, n, 0);
            return;
          } else if (skipSamples > 0) {
            zero(out, m_d->m_channels, skipSamples, 0);
            processed += skipSamples;
            remaining -= skipSamples;
            continue;
          } else if (skipSamples < 0) {
            m_d->dropOldSamples(-skipSamples);
            continue;
          }

          m_d->m_usedSeekGeneration = ts.seekGeneration();
          first = false;
        }

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

  float AudioTransfer::bufferStateSeconds() const
  {
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
    for (int it = 0; it < 2; ++it) {
      if (m_d->m_readyBuffers.load() < int(m_d->m_decodedBuffers.size()) &&
          m_d->m_samplesInBuffers.load() <= samples) {
        int b = m_d->m_buffersWriter++;
        return & m_d->m_decodedBuffers[b % s_decodedBufferCount];
      }

      if (it == 1)
        return nullptr;

      /// If the buffer is full, remove any old samples that couldn't be played anyway.
      /// This fixes stalling issues if the buffer is full because of audio playback issues.
      if (m_d->m_seeking) {
        int remove = m_d->m_samplesInBuffers.load() - m_d->m_maxAllowedAverageLatency * s_sampleRate;
        if (remove > 0) {
          Radiant::Guard g(m_d->m_decodedBuffersMutex);
          m_d->dropOldSamples(remove);
        }
      } else {
        auto now = Radiant::TimeStamp::currentTime();
        Radiant::Guard g(m_d->m_decodedBuffersMutex);
        m_d->dropOldSamples(m_d->m_maxAllowedAverageLatency, now);
      }
    }
    return nullptr;
  }

  void AudioTransfer::putReadyBuffer(int samples)
  {
    m_d->m_readyBuffers.ref();
    m_d->m_samplesInBuffers.fetchAndAddRelaxed(samples);
  }

  void AudioTransfer::setSeeking(bool seeking)
  {
    m_d->m_seeking = seeking;
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
    if (m_d->m_enabled == enabled)
      return;
    m_d->m_forceSync = true;
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

  uint64_t AudioTransfer::bufferUnderrun()
  {
    return s_bufferUnderrun;
  }
}
