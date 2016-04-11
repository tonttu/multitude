#include "ModuleInputPlayer.hpp"

#include <Radiant/BlockRingBuffer.hpp>

#include <portaudio.h>

static const int s_sampleRate = 44100;
static const float s_bufferSizeSecs = .5f;

// Uncomment if you want to see buffer underrun or overflow errors
// #define BUFFER_WARNINGS

namespace
{
  PaDeviceIndex findPaDeviceIndex(const QString & deviceName)
  {
    for (PaDeviceIndex i = 0, c = Pa_GetDeviceCount(); i < c; ++i) {
      const PaDeviceInfo * info = Pa_GetDeviceInfo(i);
      if (deviceName == info->name || QString(info->name).contains("("+deviceName+")")) {
        return i;
      }
    }
    return -1;
  }
}

namespace Resonant
{
  class ModuleInputPlayer::D
  {
  public:
    PaStreamCallbackResult capture(const float * const * input, unsigned long frameCount,
                                   const PaStreamCallbackTimeInfo * timeInfo, PaStreamCallbackFlags statusFlags);

  public:
    float m_gain = 1.0f;
    int m_channels = 0;
    bool m_initialized = false;
    PaStream * m_stream = nullptr;

    // One buffer per channel
    std::vector<Radiant::BlockRingBuffer<float>> m_buffers;

    // Configurable maximum latency (in frames)
    int m_maxLatency = 0.020 * s_sampleRate;

    // Measured minimum latency == extra buffer size
    int m_minLatency = 0;
    int m_latencyFrames = 0;

    // Set to true in prepare() once all buffers have been initialized and
    // ready for use
    std::atomic<bool> m_buffersInitialized{false};
  };

  PaStreamCallbackResult ModuleInputPlayer::D::capture(const float * const * input, unsigned long frameCount,
                                                       const PaStreamCallbackTimeInfo *,
                                                       PaStreamCallbackFlags)
  {
    if (!m_buffersInitialized) {
      return paContinue;
    }

    for (int c = 0; c < m_channels; ++c) {
      int wrote = m_buffers[c].write(input[c], frameCount);
      (void)wrote;
#ifdef BUFFER_WARNINGS
      if (wrote != frameCount) {
        Radiant::warning("ModuleInputPlayer::D::capture # Buffer overflow (%d frames)",
                         frameCount - wrote);
      }
#endif
    }
    return paContinue;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ModuleInputPlayer::ModuleInputPlayer()
    : m_d(new D())
  {
    setId(QString("ModuleInputPlayer.%1").arg(reinterpret_cast<uintptr_t>(this)).toUtf8());
  }

  ModuleInputPlayer::~ModuleInputPlayer()
  {
    close();
    if (m_d->m_initialized) {
      Pa_Terminate();
    }
  }

  ModuleInputPlayer::OpenResult ModuleInputPlayer::open(const QString & deviceName, QString * error)
  {
    if (!m_d->m_initialized) {
      PaError e = Pa_Initialize();
      if(e == paNoError) {
        m_d->m_initialized = true;
      } else {
        if (error) {
          *error = Pa_GetErrorText(e);
        }
        return OpenResult::PA_INIT_ERROR;
      }
    }

    PaStreamParameters params;
    params.device = findPaDeviceIndex(deviceName);
    if (params.device < 0) {
      if (error) {
        *error = QString("Failed to find portaudio stream for device %1").
            arg(deviceName);
      }
      return OpenResult::PA_DEVICE_NOT_FOUND;
    }

    const PaDeviceInfo * info = Pa_GetDeviceInfo(params.device);

    if (info->maxInputChannels <= 0) {
      if (error) {
        *error = QString("Device %1 doesn't have any input channels").
            arg(info->name);
      }
      return OpenResult::NO_INPUT_CHANNELS;
    }

    m_d->m_channels = info->maxInputChannels;

    params.channelCount = m_d->m_channels;
    params.sampleFormat = paFloat32 | paNonInterleaved;
    params.suggestedLatency = info->defaultLowInputLatency;
    params.hostApiSpecificStreamInfo = nullptr;

    auto callback = [](const void * input, void * /*output*/, unsigned long frameCount,
        const PaStreamCallbackTimeInfo * timeInfo, PaStreamCallbackFlags statusFlags,
        void * d) -> int {
      return static_cast<ModuleInputPlayer::D*>(d)->capture(static_cast<const float* const *>(input),
                                                            frameCount, timeInfo, statusFlags);
    };

    PaError e = Pa_OpenStream(&m_d->m_stream, &params, nullptr, s_sampleRate, 0,
                              paClipOff, callback, m_d.get());
    if (e != paNoError) {
      if (error) {
        *error = QString("Failed to open %1: %2").
            arg(info->name, Pa_GetErrorText(e));
      }
      m_d->m_stream = nullptr;
      return OpenResult::PA_OPEN_ERROR;
    }

    /// @todo Pa_SetStreamFinishedCallback
    e = Pa_StartStream(m_d->m_stream);
    if (e != paNoError) {
      if (error) {
        *error = QString("Failed to start stream %1: %2").
            arg(info->name, Pa_GetErrorText(e));
      }
      close();
      return OpenResult::PA_START_ERROR;
    }

    return OpenResult::SUCCESS;
  }

  void ModuleInputPlayer::close()
  {
    if (m_d->m_stream) {
      Pa_CloseStream(m_d->m_stream);
      m_d->m_stream = nullptr;
      m_d->m_channels = 0;
    }
  }

  void ModuleInputPlayer::setGain(float gain)
  {
    m_d->m_gain = gain;
  }

  float ModuleInputPlayer::gain() const
  {
    return m_d->m_gain;
  }

  void ModuleInputPlayer::setMaxLatency(float secs)
  {
    m_d->m_maxLatency = std::round(secs * s_sampleRate);
  }

  float ModuleInputPlayer::maxLatency() const
  {
    return m_d->m_maxLatency / s_sampleRate;
  }

  bool ModuleInputPlayer::prepare(int & channelsIn, int & channelsOut)
  {
    if (m_d->m_stream) {
      channelsIn = 0;
      channelsOut = m_d->m_channels;
      m_d->m_buffers.resize(m_d->m_channels, s_bufferSizeSecs * s_sampleRate);
      m_d->m_buffersInitialized = true;
      return true;
    } else {
      return false;
    }
  }

  void ModuleInputPlayer::process(float **, float ** out, int n, const CallbackTime &)
  {
    if (m_d->m_channels > 0) {
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

    for (int c = 0; c < m_d->m_channels; ++c) {
      auto & input = m_d->m_buffers[c];
      float * output = out[c];
      const float gain = m_d->m_gain;

      // If gain is close to one, we can just directly memcpy the data to output,
      // which can be a performance gain.
      if (std::abs(gain-1.0) < 0.001f) {
        int size = input.read(output, n);
        if (size < n) {
          std::fill_n(output + size, n - size, 0);
#ifdef BUFFER_WARNINGS
          Radiant::warning("ModuleInputPlayer::process # Buffer underrun (%d frames)", n - size);
#endif
        }
      } else if (gain < 0.001f) {
        // If the gain is close to zero, we can just consume the buffer and
        // fill the output with zeroes. If we wouldn't consume the buffer,
        // there would be old data there when we set gain back to non-zero
        input.consume(std::min(n, input.size()));
        std::fill_n(output, n, 0);
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
          std::fill_n(output, remaining, 0);
#ifdef BUFFER_WARNINGS
          Radiant::warning("ModuleInputPlayer::process # Buffer underrun (%d frames)", remaining);
#endif
        }
      }
    }
  }
} // namespace Resonant
