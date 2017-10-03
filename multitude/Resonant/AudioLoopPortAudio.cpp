#include "AudioLoopPortAudio.hpp"
#include "Resonant.hpp"
#include "DSPNetwork.hpp"
#include "ModuleOutCollect.hpp"

#include <Radiant/Semaphore.hpp>
#include <Radiant/Sleep.hpp>
#include <Radiant/Trace.hpp>

#include <Valuable/AttributeContainer.hpp>

#include <portaudio.h>

#ifdef RADIANT_LINUX
#include <alsa/error.h>
#endif

#include <map>
#include <list>
#include <set>
#include <cassert>

#define FRAMES_PER_BUFFER 128

namespace
{
  static Radiant::TimeStamp s_previousUnderflowWarning;
  static long s_framesOnLastWarning = 0;
  static unsigned int s_sampleRateWarnings = 0;
  static unsigned int s_underflowWarningsTimer = 0;
  static unsigned int s_underflowWarnings = 0;

  QString s_xmlFilename;

#ifdef RADIANT_LINUX
  static void alsaError(const char *file, int line, const char *function, int, const char *fmt, ...)
  {
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // Ignore underrun warnings, since we are going to report those through portaudio
    if (strstr(buffer, "underrun occurred")) {
      return;
    }

    Radiant::warning("ALSA %s:%d [%s] %s", file, line, function, buffer);
  }
#endif
}

namespace Resonant
{
  struct Stream {
    Stream() : stream(0), streamInfo(0), startTime(0) {
      memset( &inParams,  0, sizeof(inParams));
      memset( &outParams, 0, sizeof(outParams));
    }
    PaStreamParameters inParams;
    PaStreamParameters outParams;
    PaStream * stream;
    const PaStreamInfo * streamInfo;
    PaTime startTime;
    Radiant::Semaphore * m_barrier;
  };
  // Channel c from device d
  struct Channel {
    Channel(int d = -1, int c = -1) : device(d), channel(c) {}
    int device;
    int channel;
  };
  typedef std::map<int, Channel> Channels;

  class AudioLoopPortAudio::D
  {
  public:
    static int paCallback(const void *in, void *out,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* time,
                          PaStreamCallbackFlags status,
                          void * user);

    static void paFinished(void * user);

  public:
    /// Maps [input channel] -> [device, channel]
    Channels m_channels;
    std::vector<Stream> m_streams;

    typedef std::pair<AudioLoopPortAudio::D*, int> Callback;
    typedef std::list<Callback> CallbackList;
    CallbackList cb;

    std::set<int> m_written;
    std::vector<void*> m_streamBuffers;

    Radiant::Semaphore m_sem;

    long m_frames = 0;

    bool m_isRunning = false;
    bool m_initialized = false;

    DSPNetwork & m_dsp;
    std::shared_ptr<ModuleOutCollect> m_collect;

    struct
    {
      Radiant::TimeStamp baseTime;
      int framesProcessed = 0;
    } m_syncinfo;

  public:
    D(DSPNetwork & dsp, const std::shared_ptr<ModuleOutCollect> & collect)
      : m_dsp(dsp)
      , m_collect(collect)
    {}

    /// This is called from PortAudio thread when the stream becomes inactive
    /// @param streamid Device / Stream number, @see setDevicesFile()
    void finished(int streamid);

    /// Callback function that is called from the PortAudio thread
    /// @param in Array of interleaved input samples for each channel
    /// @param[out] out Array of interleaved input samples for each channel,
    ///                 this should be filled by the callback function.
    /// @param framesPerBuffer The number of sample frames to be processed
    /// @param streamid Device / Stream number, @see setDevicesFile()
    /// @return paContinue, paComplete or paAbort. See PaStreamCallbackResult
    ///         for more information
    /// @see PaStreamCallback in PortAudio documentation
    int callback(const void * in, void * out,
                 unsigned long framesPerBuffer, int streamid,
                 const PaStreamCallbackTimeInfo & time,
                 unsigned long flags);
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  int AudioLoopPortAudio::D::paCallback(const void *in, void *out,
                                               unsigned long framesPerBuffer,
                                               const PaStreamCallbackTimeInfo * time,
                                               PaStreamCallbackFlags status,
                                               void * user)
  {
    AudioLoopPortAudio::D * self;
    int stream;
    std::tie(self, stream) = *static_cast<Callback*>(user);

    if (self->m_isRunning) {
      int r = self->callback(in, out, framesPerBuffer, stream, *time, status);
      return self->m_isRunning ? r : paComplete;
    } else {
      return paComplete;
    }
  }

  void AudioLoopPortAudio::D::paFinished(void * user)
  {
    AudioLoopPortAudio::D * self;
    int stream;
    std::tie(self, stream) = *static_cast<Callback*>(user);

    self->finished(stream);
    debugResonant("AudioLoopPortAudio::paFinished # %p %d", self, stream);
  }

  void AudioLoopPortAudio::D::finished(int /*streamid*/)
  {
    m_isRunning = false;
  }

  int AudioLoopPortAudio::D::callback(const void * in, void * out,
                                      unsigned long framesPerBuffer, int streamnum,
                                      const PaStreamCallbackTimeInfo & time,
                                      unsigned long flags)
  {
    (void) in;

    size_t streams = m_streams.size();

    Radiant::TimeStamp outputTime;
    double latency;

    const bool outputError = (flags & paOutputUnderflow) || (flags & paOutputOverflow);

    if (streamnum == 0 && s_underflowWarnings) {
      s_underflowWarningsTimer += framesPerBuffer;
    }

    if(flags & paOutputUnderflow) {
      ++s_underflowWarnings;
    }
    if(flags & paOutputOverflow) {
      Radiant::warning("DSPNetwork::callback # output overflow");
    }

    if (streamnum == 0 && s_underflowWarnings > 0 && s_underflowWarningsTimer >= (44100*10)) {
      if (s_previousUnderflowWarning.value() != 0) {
        double secs = s_previousUnderflowWarning.sinceSecondsD();
        int sampleRate = (m_frames - s_framesOnLastWarning) / secs;
        if (sampleRate > 50000) {
          if (++s_sampleRateWarnings > 1) {
            Radiant::warning("DSPNetwork # Unexpected sample rate, current sample rate is %d, expected about 44100",
                             sampleRate);
          }
        } else {
          s_sampleRateWarnings = 0;
        }
      }
      s_previousUnderflowWarning = Radiant::TimeStamp::currentTime();
      Radiant::warning("DSPNetwork # %d output underflow errors in the last 10 seconds", s_underflowWarnings);
      s_underflowWarnings = 0;
      s_underflowWarningsTimer = 0;
      s_framesOnLastWarning = m_frames;
    }

    // We either have multiple streams or have a broken implementation
    // (Linux/Pulseaudio). In that case we just generate the timing information
    // ourselves. In this case we also guess that the latency is ~30 ms
    // (on Linux/Alsa it seems to be ~20-30 ms when having some cheap hw)
    // On some broken platforms outputBufferDacTime is just not implemented
    // and is always 0, but on some other platforms it seems to be just a small
    // number (~0.001..0.005), too small and random to be the audio latency or
    // anything like that.
    if(time.outputBufferDacTime < 1.0) {
      latency = 0.030;
      if(streamnum == 0 && (m_syncinfo.baseTime == Radiant::TimeStamp(0) || outputError)) {
        m_syncinfo.baseTime = Radiant::TimeStamp(Radiant::TimeStamp::currentTime()) +
            Radiant::TimeStamp::createSeconds(latency);
        outputTime = m_syncinfo.baseTime;
        m_syncinfo.framesProcessed = 0;
      } else {
        outputTime = m_syncinfo.baseTime + Radiant::TimeStamp::createSeconds(
              m_syncinfo.framesProcessed / 44100.0);
      }
    } else if (streamnum == 0) {
      // On some ALSA implementations, time.currentTime is zero but time.outputBufferDacTime is valid
      if (time.currentTime == 0) {
        latency = time.outputBufferDacTime - Radiant::TimeStamp::currentTime().secondsD();
        /// In principle outputBufferDacTime and timestamps are incompatible as PaTimes
        /// have undefined starting point. For example in Windows the origin seems to
        /// be the startup time of the PC. However in Linux they seem to be compatible.
        assert(std::abs(latency) < 120.f);
      } else {
        latency = time.outputBufferDacTime - time.currentTime;
      }

      if(m_syncinfo.baseTime == Radiant::TimeStamp(0) || m_syncinfo.framesProcessed > 44100 * 60 ||
         outputError) {
        m_syncinfo.baseTime = Radiant::TimeStamp::currentTime() +
            Radiant::TimeStamp::createSeconds(latency);
        m_syncinfo.framesProcessed = 0;
      }

      outputTime = m_syncinfo.baseTime +
          Radiant::TimeStamp::createSeconds(m_syncinfo.framesProcessed/44100.0);
      m_syncinfo.framesProcessed += framesPerBuffer;
    } else {
      outputTime = m_syncinfo.baseTime +
          Radiant::TimeStamp::createSeconds(m_syncinfo.framesProcessed/44100.0);
    }

    /// Here we assume that every stream (== audio device) is running in its
    /// own separate thread, that is, this callback is called from multiple
    /// different threads at the same time, one for each audio device.
    /// The first thread is responsible for filling the buffer
    /// (m_collect->interleaved()) by calling doCycle. This thread first waits
    /// until all other threads have finished processing the previous data,
    /// then runs the next cycle and informs everyone else that they can continue
    /// running from the barrier.
    /// We also assume, that framesPerBuffer is somewhat constant in different
    /// threads at the same time.
    if(streams == 1) {
      m_dsp.doCycle(framesPerBuffer, CallbackTime(outputTime, latency, flags));
    } else if(streamnum == 0) {
      m_sem.acquire(static_cast<int> (streams));
      m_dsp.doCycle(framesPerBuffer, CallbackTime(outputTime, latency, flags));
      for (size_t i = 1; i < streams; ++i)
        m_streams[i].m_barrier->release();
    } else {
      m_streams[streamnum].m_barrier->acquire();
    }

    int outChannels = m_streams[streamnum].outParams.channelCount;

    const float * res = m_collect->interleaved();
    if(res != nullptr) {
      for (Channels::iterator it = m_channels.begin(); it != m_channels.end(); ++it) {
        if (streamnum != it->second.device) continue;
        int from = it->first;
        int to = it->second.channel;

        const float * data = res + from;
        float* target = (float*)out;
        target += to;

        size_t chans_from = m_collect->channels();

        for (size_t i = 0; i < framesPerBuffer; ++i) {
          *target = *data;
          target += outChannels;
          data += chans_from;
        }
      }
      //memcpy(out, res, 4 * framesPerBuffer * outChannels);
    }
    else {
      Radiant::error("DSPNetwork::callback # No data to play");
      memset(out, 0, 4 * framesPerBuffer * outChannels);
    }
    if(streams > 1) m_sem.release();

    if (streamnum == 0) {
      m_frames += framesPerBuffer;
      if(m_frames < 40000) {
        debugResonant("DSPNetwork::callback # %lu", framesPerBuffer);
      }
    }

    return paContinue;
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  AudioLoopPortAudio::AudioLoopPortAudio(DSPNetwork & dsp, const std::shared_ptr<ModuleOutCollect> & collect)
    : m_d(new D(dsp, collect))
  {
#ifdef RADIANT_LINUX
    MULTI_ONCE {
      snd_lib_error_set_handler(alsaError);
    }
#endif

    // Initializes PortAudio / increases the usage counter.
    PaError e = Pa_Initialize();
    if(e == paNoError) {
      m_d->m_initialized = true;
    } else {
      Radiant::error("AudioLoopPortAudio::init # %s", Pa_GetErrorText(e));
    }
  }

  AudioLoopPortAudio::~AudioLoopPortAudio()
  {
    if (isRunning())
      Radiant::error("AudioLoopPortAudio::~AudioLoopPortAudio(): audio still running");

    if(m_d->m_initialized) {
      // Decreases usage counter, if this is the last AudioLoopPortAudio using PA, the library is closed.
      // From Pa API: Pa_Terminate() MUST be called before exiting a program
      //              which uses PortAudio. Failure to do so may result in
      //              serious resource leaks, such as audio devices not being
      //              available until the next reboot.
      PaError e = Pa_Terminate();
      if(e != paNoError) {
        Radiant::error("AudioLoopPortAudio::cleanup # %s", Pa_GetErrorText(e));
      }
    }
  }

  bool AudioLoopPortAudio::start(int samplerate, int channels)
  {
    assert(!isRunning());

    /// List of device/channel request pairs
    /// For example [("Sound Blaster 1", 3), ("Turtle Beach", 2), (7, 1)]
    /// means that channels 0..2 will be mapped to Sound Blaster channels 0..2,
    /// channels 3..4 are mapped to Turtle Beach channels 0..1, and channel 5 is mapped
    /// to sound device number 7 channel 0.
    /// It seems that portaudio doesn't allow opening any random channels devices,
    /// just n first channels.
    typedef std::pair<QString, int> Device;
    typedef std::vector<Device> Devices;
    Devices devices;

    const char * devname = getenv("RESONANT_DEVICE");
    if(devname) {
      devices.push_back(Device(devname, channels));
    } else if(!s_xmlFilename.isEmpty()) {
      devices = *Valuable::Serializer::deserializeXML<Valuable::AttributeContainer<Devices> >(s_xmlFilename);
    }

    if(devices.empty()) {
      m_d->m_streams.push_back(Stream());
      Stream & s = m_d->m_streams.back();

      s.outParams.device = Pa_GetDefaultOutputDevice();
      if(s.outParams.device == paNoDevice) {
        for (int i = 0; i < Pa_GetDeviceCount();++ i) {
          const PaDeviceInfo * info = Pa_GetDeviceInfo(i);
          if (QByteArray("default") == info->name) {
            s.outParams.device = i;
            break;
          }
        }

        if (s.outParams.device == paNoDevice) {
          if (Pa_GetDeviceCount() > 0) {
            s.outParams.device = 0;
          } else {
            Radiant::error("AudioLoopPortAudio::startReadWrite # No default output device available");
            return false;
          }
        }
      }

      devices.push_back(Device("", channels));

      debugResonant("AudioLoopPortAudio::startReadWrite # Selected default output device %d", s.outParams.device);
    }
    else {
      for (size_t dev = 0; dev < devices.size(); ++dev) {
        m_d->m_streams.push_back(Stream());
        Stream & s = m_d->m_streams.back();

        QByteArray tmp = devices[dev].first.toUtf8();
        const char * devkey = tmp.data();
        int channel_requests = devices[dev].second;
        char * end = 0;
        int i = strtol(devkey, & end, 10);

        long decoded = end - devkey;
        if(decoded == (long) strlen(devkey)) {
          s.outParams.device = i;
          debugResonant("AudioLoopPortAudio::startReadWrite # Selected device %d (%s)", (int) s.outParams.device, devkey);
        }
        else {
          int n = Pa_GetDeviceCount();

          for( i = 0; i < n; i++) {
            const PaDeviceInfo * info = Pa_GetDeviceInfo(i);
            if(strstr(info->name, devkey) != 0) {
              if (channel_requests > info->maxOutputChannels) {
                debugResonant("Skipping device %d, not enough output channels (%d < %d)",
                              (int)dev, info->maxOutputChannels, channel_requests);
                continue;
              }

              s.outParams.device = i;

              debugResonant("AudioLoopPortAudio::startReadWrite # Selected device %d %s",
                            (int) s.outParams.device, info->name);
              break;
            }
          }
          if (i == n) {
            Radiant::error("Couldn't find device %s", devkey);
          }
        }
      }
    }

    for (size_t streamnum = 0, streams = m_d->m_streams.size(); streamnum < streams; ++streamnum) {
      Stream & s = m_d->m_streams[streamnum];
      /// @todo m_barrier isn't released ever
      s.m_barrier = streams == 1 ? nullptr : new Radiant::Semaphore(0);
      channels = devices[streamnum].second;

      const PaDeviceInfo * info = Pa_GetDeviceInfo(s.outParams.device);

      if (!info) {
        Radiant::error("AudioLoopPortAudio::startReadWrite # Pa_GetDeviceInfo(%d) failed",
                       s.outParams.device);
        return false;
      }

      debugResonant("AudioLoopPortAudio::startReadWrite # Got audio device %d = %s",
                    (int) s.outParams.device, info->name);

      int n = Pa_GetDeviceCount();

      for(int i = 0; i < n; i++) {
        const PaDeviceInfo * info2 = Pa_GetDeviceInfo(i);
        const PaHostApiInfo * apiinfo = Pa_GetHostApiInfo(info2->hostApi);
        debugResonant("AudioLoopPortAudio::startReadWrite # Available %d: %s (API = %s)",
                      i, info2->name, apiinfo->name);
      }

      debugResonant("AudioLoopPortAudio::startReadWrite # channels = %d limits = %d %d",
                    channels, info->maxInputChannels, info->maxOutputChannels);


      s.outParams.channelCount = channels;
      s.outParams.sampleFormat = paFloat32;
      // Use 30ms minimum latency
      s.outParams.suggestedLatency =
          std::max(0.03, info->defaultLowOutputLatency);
      s.outParams.hostApiSpecificStreamInfo = 0;

      s.inParams = s.outParams;
      s.inParams.device = Pa_GetDefaultInputDevice();

      m_d->cb.push_back(std::make_pair(m_d.get(), static_cast<int> (streamnum)));

      PaError err = Pa_OpenStream(& s.stream,
                                  0, // & m_inParams,
                                  & s.outParams,
                                  samplerate,
                                  FRAMES_PER_BUFFER,
                                  paClipOff,
                                  &D::paCallback,
                                  &m_d->cb.back() );

      if( err != paNoError ) {
        Radiant::error("AudioLoopPortAudio::startReadWrite # Pa_OpenStream failed (device %d, channels %d, sample rate %d)",
                       s.outParams.device, channels, samplerate);
        return false;
      }

      err = Pa_SetStreamFinishedCallback(s.stream, & m_d->paFinished );

      s.streamInfo = Pa_GetStreamInfo(s.stream);

      for (int i = 0; i < s.outParams.channelCount; ++i)
        m_d->m_channels[static_cast<int> (m_d->m_channels.size())] = Channel(static_cast<int> (streamnum), i);

      debugResonant("AudioLoopPortAudio::startReadWrite # %d channels lt = %lf, EXIT OK",
                    (int) s.outParams.channelCount,
                    (double) s.streamInfo->outputLatency);
    }

    m_d->m_streamBuffers.resize(m_d->m_streams.size());
    m_d->m_sem.release(static_cast<int> (m_d->m_streams.size()));

    m_d->m_isRunning = true;

    for (size_t streamnum = 0; streamnum < m_d->m_streams.size(); ++streamnum) {
      Stream & s = m_d->m_streams[streamnum];

      PaError err = Pa_StartStream(s.stream);

      if( err != paNoError ) {
        Radiant::error("AudioLoopPortAudio::startReadWrite # Pa_StartStream failed");
        return false;
      }

      s.startTime = Pa_GetStreamTime(s.stream);
    }

    return true;
  }

  bool AudioLoopPortAudio::stop()
  {
    if(!isRunning())
      return true;

    m_d->m_isRunning = false;
    m_d->m_sem.release(m_d->m_streams.size());

    {
      /* Hack to get the audio closed in all cases (mostly for Linux). */
      Radiant::Sleep::sleepMs(200);
    }

    for (size_t num = 0; num < m_d->m_streams.size(); ++num) {
      Stream & s = m_d->m_streams[num];
      int err = Pa_CloseStream(s.stream);
      if(err != paNoError) {
        Radiant::error("AudioLoopPortAudio::stop # Could not close stream");
      }
      s.stream = 0;
      s.streamInfo = 0;
      m_d->m_channels.erase(static_cast<int> (num));
    }

    return true;
  }

  bool AudioLoopPortAudio::isRunning() const
  {
    return m_d->m_isRunning;
  }

  size_t AudioLoopPortAudio::outChannels() const
  {
    return m_d->m_channels.size();
  }

  void AudioLoopPortAudio::setDevicesFile(const QString & xmlFilename)
  {
    s_xmlFilename = xmlFilename;
  }
}
