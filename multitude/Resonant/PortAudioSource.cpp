#include "PortAudioSource.hpp"

#include <portaudio.h>

static const int s_sampleRate = 44100;

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
  class PortAudioSource::D
  {
  public:
    PaStreamCallbackResult capture(const float * const * input, unsigned long frameCount,
                                   const PaStreamCallbackTimeInfo * timeInfo, PaStreamCallbackFlags statusFlags);

  public:
    bool m_initialized = false;
    PaStream * m_stream = nullptr;

    ModuleBufferPlayerPtr m_module = std::make_shared<ModuleBufferPlayer>();
  };

  PaStreamCallbackResult PortAudioSource::D::capture(const float * const * input, unsigned long frameCount,
                                                     const PaStreamCallbackTimeInfo *,
                                                     PaStreamCallbackFlags)
  {
    auto & buffers = m_module->buffers();
    const int channels = m_module->channelCount();

    for (int c = 0; c < channels; ++c) {
      int wrote = buffers[c].write(input[c], frameCount);
      (void)wrote;
#ifdef BUFFER_WARNINGS
      if (wrote != frameCount) {
        Radiant::warning("PortAudioSource::D::capture # Buffer overflow (%d frames)",
                         frameCount - wrote);
      }
#endif
    }
    return paContinue;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  PortAudioSource::PortAudioSource()
    : m_d(new D())
  {
  }

  PortAudioSource::~PortAudioSource()
  {
    close();
    if (m_d->m_initialized) {
      Pa_Terminate();
    }
  }

  PortAudioSource::OpenResult PortAudioSource::open(const QString & deviceName, QString * error)
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

    int channels = info->maxInputChannels;

    params.channelCount = channels;
    params.sampleFormat = paFloat32 | paNonInterleaved;
    params.suggestedLatency = info->defaultLowInputLatency;
    params.hostApiSpecificStreamInfo = nullptr;

    auto callback = [](const void * input, void * /*output*/, unsigned long frameCount,
        const PaStreamCallbackTimeInfo * timeInfo, PaStreamCallbackFlags statusFlags,
        void * d) -> int {
      return static_cast<PortAudioSource::D*>(d)->capture(static_cast<const float* const *>(input),
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

    m_d->m_module->setChannelCount(channels);

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

  void PortAudioSource::close()
  {
    if (m_d->m_stream) {
      Pa_CloseStream(m_d->m_stream);
      m_d->m_stream = nullptr;
    }
  }
} // namespace Resonant
