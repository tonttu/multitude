#include "AudioTransfer2.hpp"

#include <Resonant/AudioLoop.hpp>

namespace
{
  void zero(float ** dest, int channels, int frames, int offset)
  {
    assert(frames >= 0);

    for(int channel = 0; channel < channels; ++channel)
      memset(dest[channel] + offset, 0, frames * sizeof(float));
  }
}

namespace VideoPlayer2
{
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

  /// @todo should do this without copying any data! (using ffmpeg buffer refs)
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
    D(int channels, int & seekGeneration, AVDecoder::PlayMode & playMode)
      : channels(channels)
      , seekGeneration(seekGeneration)
      , playMode(playMode)
      , decodedBuffers(s_decodedBufferCount)
      , buffersReader(0)
      , buffersWriter(0)
      , resonantToPts(0)
      , usedSeekGeneration(0)
      , gain(1.0f)
      /*, samplesProcessed(0)*/
    {}

    const int channels;
    int & seekGeneration;
    AVDecoder::PlayMode & playMode;

    Timestamp pts;

    std::vector<DecodedAudioBuffer> decodedBuffers;

    // decodedBuffers[buffersReader % s_decodedBufferCount] points to the next
    // buffer that could be processed to Resonant (iff readyBuffers > 0).
    // Only used from process()
    int buffersReader;
    // decodedBuffers[buffersWriter % s_decodedBufferCount] points to the next
    // buffer that could be filled with new decoded audio from the AVDecoder
    int buffersWriter;

    QAtomicInt readyBuffers;
    QAtomicInt samplesInBuffers;

    double resonantToPts;
    int usedSeekGeneration;

    float gain;
    /*long samplesProcessed;*/

    DecodedAudioBuffer * getReadyBuffer();
    void bufferConsumed(int samples);
  };

  DecodedAudioBuffer * AudioTransfer::D::getReadyBuffer()
  {
    while(playMode == AVDecoder::Play && readyBuffers > 0) {
      DecodedAudioBuffer * buffer = & decodedBuffers[buffersReader % s_decodedBufferCount];
      if(buffer->timestamp().seekGeneration < seekGeneration) {
        samplesInBuffers.fetchAndAddRelaxed(-buffer->samples());
        readyBuffers.deref();
        ++buffersReader;
        continue;
      }
      return buffer;
    }
    return 0;
  }

  void AudioTransfer::D::bufferConsumed(int samples)
  {
    readyBuffers.deref();
    samplesInBuffers.fetchAndAddRelaxed(-samples);
    ++buffersReader;
  }

  AudioTransfer::AudioTransfer(int channels, int & seekGeneration, AVDecoder::PlayMode & playMode)
    : Module(0)
    , m_d(new D(channels, seekGeneration, playMode))
  {
    assert(channels > 0);
  }

  AudioTransfer::~AudioTransfer()
  {
    delete m_d;
  }

  bool AudioTransfer::prepare(int & channelsIn, int & channelsOut)
  {
    channelsIn = 0;
    channelsOut = m_d->channels;
    return true;
  }

  void AudioTransfer::process(float **, float ** out, int n, const Resonant::CallbackTime & time)
  {
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
      if(!decodedBuffer) {
        zero(out, m_d->channels, remaining, processed);
        break;
      } else {
        const int offset = decodedBuffer->offset();
        const int samples = std::min<int>(remaining, decodedBuffer->samples() - offset);

        const Timestamp ts = decodedBuffer->timestamp();

        // Presentation time of the decoded audio buffer, at the time
        // when currently processed audio will be written to audio hardware
        const double pts = ts.pts + offset / 44100.0;

        m_d->pts = ts;
        m_d->pts.pts = pts + samples / 44100.0;

        if(first) {
          // We can convert Resonant times to pts with this offset
          m_d->resonantToPts = pts - time.outputTime.secondsD();
          m_d->usedSeekGeneration = ts.seekGeneration;

          first = false;
        }


        // We could take the gain into account in the preprocessing stage,
        // in another thread with more resources, but then changing gain
        // would have a noticeably latency
        const float gain = m_d->gain;
        if(std::abs(gain - 1.0f) < 1e-5f) {
          for(int channel = 0; channel < m_d->channels; ++channel)
            memcpy(out[channel] + processed, decodedBuffer->data(channel) + offset, samples * sizeof(float));
        } else {
          for(int channel = 0; channel < m_d->channels; ++channel) {
            float * destination = out[channel] + processed;
            const float * source = decodedBuffer->data(channel) + offset;
            for(int s = 0; s < samples; ++s)
              *destination++ = *source++ * gain;
          }
        }

        processed += samples;
        remaining -= samples;

        if(offset + samples == decodedBuffer->samples())
          m_d->bufferConsumed(offset + samples);
        else
          decodedBuffer->setOffset(offset + samples);
      }
    }
  }

  Timestamp AudioTransfer::toPts(const Radiant::TimeStamp & ts) const
  {
    const Timestamp newts(ts.secondsD() + m_d->resonantToPts, m_d->usedSeekGeneration);
    return std::min(m_d->pts, newts);
  }

  Timestamp AudioTransfer::lastPts() const
  {
    return m_d->pts;
  }

  DecodedAudioBuffer * AudioTransfer::takeFreeBuffer(int samples)
  {
    if(m_d->readyBuffers >= m_d->decodedBuffers.size())
      return nullptr;

    if(m_d->samplesInBuffers > samples)
      return nullptr;

    int b = m_d->buffersWriter++;

    return & m_d->decodedBuffers[b % s_decodedBufferCount];
  }

  void AudioTransfer::putReadyBuffer(int samples)
  {
    m_d->samplesInBuffers.fetchAndAddRelaxed(samples);
    m_d->readyBuffers.ref();
  }
}
