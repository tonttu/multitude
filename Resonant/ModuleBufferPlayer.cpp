#include "ModuleBufferPlayer.hpp"

static const int s_sampleRate = 44100;
static const float s_bufferSizeSecs = .5f;

// Uncomment if you want to see buffer underrun or overflow errors
// #define BUFFER_WARNINGS

namespace Resonant
{
  class ModuleBufferPlayer::D
  {
  public:
    float m_gain = 1.0f;
    int m_channelCount = 0;

    // One buffer per channel
    std::vector<Radiant::BlockRingBuffer<float>> m_buffers;

    // Configurable maximum latency (in frames)
    int m_maxLatency = static_cast<int>(0.020 * s_sampleRate);

    // Measured minimum latency == extra buffer size
    int m_minLatency = 0;
    int m_latencyFrames = 0;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ModuleBufferPlayer::ModuleBufferPlayer(const QString & name)
    : m_d(new D())
  {
    static std::atomic<int> counter {0};
    setId(QString("%2.%1").arg(counter++).arg(name).toUtf8());
  }

  ModuleBufferPlayer::~ModuleBufferPlayer()
  {
  }

  void ModuleBufferPlayer::setChannelCount(int channelCount)
  {
    m_d->m_channelCount = channelCount;
    m_d->m_buffers.resize(m_d->m_channelCount, static_cast<int>(s_bufferSizeSecs * s_sampleRate));
  }

  int ModuleBufferPlayer::channelCount() const
  {
    return m_d->m_channelCount;
  }

  std::vector<Radiant::BlockRingBuffer<float>> & ModuleBufferPlayer::buffers()
  {
    return m_d->m_buffers;
  }

  void ModuleBufferPlayer::setGain(float gain)
  {
    m_d->m_gain = gain;
  }

  float ModuleBufferPlayer::gain() const
  {
    return m_d->m_gain;
  }

  void ModuleBufferPlayer::setMaxLatency(float secs)
  {
    m_d->m_maxLatency = static_cast<int>(std::round(secs * s_sampleRate));
  }

  float ModuleBufferPlayer::maxLatency() const
  {
    return static_cast<float>(m_d->m_maxLatency) / s_sampleRate;
  }

  bool ModuleBufferPlayer::prepare(int & channelsIn, int & channelsOut)
  {
    if (m_d->m_channelCount) {
      channelsIn = 0;
      channelsOut = m_d->m_channelCount;
      return true;
    } else {
      return false;
    }
  }

  void ModuleBufferPlayer::process(float **, float ** out, int n, const CallbackTime &)
  {
    if (m_d->m_channelCount > 0) {
      const int latencyCheckSecs = 3;

      if (m_d->m_latencyFrames == 0) {
        m_d->m_minLatency = m_d->m_buffers[0].size();
        m_d->m_latencyFrames = n;
      } else {
        m_d->m_minLatency = std::min(m_d->m_buffers[0].size(), m_d->m_minLatency);
        m_d->m_latencyFrames += n;
      }

      if (m_d->m_latencyFrames >= s_sampleRate * latencyCheckSecs) {
        m_d->m_latencyFrames = 0;

        if (m_d->m_minLatency > m_d->m_maxLatency) {
          // Drop frames to reduce latency
          for (auto & input: m_d->m_buffers) {
            input.consume(m_d->m_minLatency - m_d->m_maxLatency);
          }
        }
      }
    }

    for (int c = 0; c < m_d->m_channelCount; ++c) {
      auto & input = m_d->m_buffers[c];
      float * output = out[c];
      const float gain = m_d->m_gain;

      // If gain is close to one, we can just directly memcpy the data to output,
      // which can be a performance gain.
      if (std::abs(gain-1.0) < 0.001f) {
        int size = input.read(output, n);
        if (size < n) {
          std::fill_n(output + size, n - size, 0.f);
#ifdef BUFFER_WARNINGS
          Radiant::warning("ModuleBufferPlayer::process # Buffer underrun (%d frames)", n - size);
#endif
        }
      } else if (gain < 0.001f) {
        // If the gain is close to zero, we can just consume the buffer and
        // fill the output with zeroes. If we wouldn't consume the buffer,
        // there would be old data there when we set gain back to non-zero
        input.consume(std::min(n, input.size()));
        std::fill_n(output, n, 0.f);
      } else {
        int remaining = n;

        // Normal case is when gain is something else than zero or one, and we
        // need to multiple every sample with the gain. Do this using the reader
        // object, which allows us to make the whole thing without an extra copy.
        while (remaining > 0) {
          auto reader = input.read(remaining);
          if (reader.size() == 0) break;

          for (float * in = reader.data(), * end = in + reader.size(); in != end; ++in) {
            *output++ = *in * gain;
          }
          remaining -= reader.size();
        }

        if (remaining > 0) {
          std::fill_n(output, remaining, 0.f);
#ifdef BUFFER_WARNINGS
          Radiant::warning("ModuleBufferPlayer::process # Buffer underrun (%d frames)", remaining);
#endif
        }
      }
    }
  }
} // namespace Resonant
