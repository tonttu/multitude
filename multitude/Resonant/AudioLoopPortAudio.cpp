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

const int s_framesPerBuffer = 128;
const int s_interleavedBufferSize = s_framesPerBuffer * 10;

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
    struct Stream {
      Stream() {
        memset( &inParams,  0, sizeof(inParams));
        memset( &outParams, 0, sizeof(outParams));
      }
      PaStreamParameters inParams;
      PaStreamParameters outParams;
      PaStream * stream = nullptr;
      const PaStreamInfo * streamInfo = nullptr;

      uint64_t frames = 0;

      AudioLoopPortAudio::D * host = nullptr;
      int streamNum = 0;
    };

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

    std::set<int> m_written;

    bool m_isRunning = false;
    bool m_initialized = false;

    unsigned int m_sampleRate = 44100;

    /// This is locked while m_interleaved is being written and the primary
    /// stream is being selected each frame
    Radiant::Mutex m_streamMutex;
    std::atomic<uint64_t> m_frames { 0 };
    std::vector<float> m_interleaved;

    DSPNetwork & m_dsp;
    std::shared_ptr<ModuleOutCollect> m_collect;

    struct
    {
      Radiant::TimeStamp baseTime;
      uint64_t framesProcessed = 0;
    } m_syncinfo;

  public:
    D(DSPNetwork & dsp, const std::shared_ptr<ModuleOutCollect> & collect)
      : m_dsp(dsp)
      , m_collect(collect)
    {}

    /// This is called from PortAudio thread when the stream becomes inactive
    /// @param streamid Device / Stream number, @see setDevicesFile()
    void finished(int streamid);

    CallbackTime createCallbackTime(const PaStreamCallbackTimeInfo & time,
                                    unsigned long flags);

    /// Writes data to audio device output
    void writeFrames(float *& out, int streamNum,
                     uint64_t available, int & remaining,
                     int sourceChannels, int targetChannels);

    /// Callback function that is called from the PortAudio thread
    /// @param[out] out Array of interleaved input samples for each channel,
    ///                 this should be filled by the callback function.
    /// @param streamNum Device / Stream number, @see setDevicesFile()
    /// @see PaStreamCallback in PortAudio documentation
    void callback(float * out, int streamNum,
                  const PaStreamCallbackTimeInfo & time,
                  unsigned long flags);
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  int AudioLoopPortAudio::D::paCallback(const void * /* in */, void *out,
                                               unsigned long framesPerBuffer,
                                               const PaStreamCallbackTimeInfo * time,
                                               PaStreamCallbackFlags status,
                                               void * user)
  {
    auto stream = static_cast<Stream*>(user);
    AudioLoopPortAudio::D * self = stream->host;
    const int streamNum = stream->streamNum;

    if (!self->m_isRunning)
      return paComplete;

    if (self->m_isRunning && framesPerBuffer == s_framesPerBuffer)
      self->callback((float*)out, streamNum, *time, status);
    return paContinue;
  }

  void AudioLoopPortAudio::D::paFinished(void * user)
  {
    auto stream = static_cast<Stream*>(user);
    AudioLoopPortAudio::D * self = stream->host;
    const int streamNum = stream->streamNum;

    self->finished(streamNum);
    debugResonant("AudioLoopPortAudio::paFinished # %p %d", self, streamNum);
  }

  void AudioLoopPortAudio::D::finished(int streamNum)
  {
    if (m_isRunning)
      Radiant::error("AudioLoopPortAudio # Stream %d closed", streamNum);
  }

  CallbackTime AudioLoopPortAudio::D::createCallbackTime(const PaStreamCallbackTimeInfo & time,
                                                         unsigned long flags)
  {
    Radiant::TimeStamp outputTime;
    double latency;

    const bool outputError = (flags & paOutputUnderflow) || (flags & paOutputOverflow);

    if (s_underflowWarnings) {
      s_underflowWarningsTimer += s_framesPerBuffer;
    }

    CallbackTime::CallbackFlags callbackFlags;
    if(flags & paOutputUnderflow) {
      callbackFlags |= CallbackTime::FLAG_BUFFER_UNDERFLOW;
      ++s_underflowWarnings;
    }
    if(flags & paOutputOverflow) {
      callbackFlags |= CallbackTime::FLAG_BUFFER_OVERFLOW;
      Radiant::warning("DSPNetwork::callback # output overflow");
    }

    if (s_underflowWarnings > 0 && s_underflowWarningsTimer >= (m_sampleRate*10)) {
      if (s_previousUnderflowWarning.value() != 0) {
        double secs = s_previousUnderflowWarning.sinceSecondsD();
        int sampleRate = (m_frames - s_framesOnLastWarning) / secs;
        if (sampleRate > 50000) {
          if (++s_sampleRateWarnings > 1) {
            Radiant::warning("DSPNetwork # Unexpected sample rate, current sample rate is %d, expected about %d",
                             sampleRate, m_sampleRate);
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
    if (time.outputBufferDacTime < 1.0) {
      latency = 0.030;
      if (m_syncinfo.baseTime == Radiant::TimeStamp(0) || outputError) {
        m_syncinfo.baseTime = Radiant::TimeStamp(Radiant::TimeStamp::currentTime()) +
            Radiant::TimeStamp::createSeconds(latency);
        outputTime = m_syncinfo.baseTime;
        m_syncinfo.framesProcessed = 0;
      } else {
        outputTime = m_syncinfo.baseTime + Radiant::TimeStamp::createSeconds(
              double(m_syncinfo.framesProcessed) / m_sampleRate);
        m_syncinfo.framesProcessed += s_framesPerBuffer;
      }
    } else {
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

      if(m_syncinfo.baseTime == Radiant::TimeStamp(0) || m_syncinfo.framesProcessed > m_sampleRate * 60 ||
         outputError) {
        m_syncinfo.baseTime = Radiant::TimeStamp::currentTime() +
            Radiant::TimeStamp::createSeconds(latency);
        m_syncinfo.framesProcessed = 0;
      }

      outputTime = m_syncinfo.baseTime +
          Radiant::TimeStamp::createSeconds(double(m_syncinfo.framesProcessed) / m_sampleRate);
      m_syncinfo.framesProcessed += s_framesPerBuffer;
    }

    return CallbackTime(outputTime, latency, callbackFlags);
  }

  void AudioLoopPortAudio::D::writeFrames(float *& out, int streamNum,
                                          uint64_t available, int & remaining,
                                          int sourceChannels, int targetChannels)
  {
    Stream & stream = m_streams[streamNum];

    if (available > s_interleavedBufferSize) {
      available = s_framesPerBuffer;
      stream.frames = m_frames - available;
    }
    const float * buffer = m_interleaved.data() + (stream.frames % s_interleavedBufferSize) * sourceChannels;
    const uint32_t frames = std::min<uint64_t>(s_framesPerBuffer, available);
    stream.frames += frames;
    remaining -= frames;

    for (auto it = m_channels.begin(); it != m_channels.end(); ++it) {
      if (streamNum != it->second.device) continue;
      const int sourceChannel = it->first;
      const int targetChannel = it->second.channel;

      const float * source = buffer + sourceChannel;
      float * target = out + targetChannel;
      const float * targetEnd = target + frames * targetChannels;

      while (target < targetEnd) {
        *target = *source;
        target += targetChannels;
        source += sourceChannels;
      }
    }
    out += frames * targetChannels;
  }

  void AudioLoopPortAudio::D::callback(float * out, int streamnum,
                                       const PaStreamCallbackTimeInfo & time,
                                       unsigned long flags)
  {
    const size_t streamCount = m_streams.size();

    // See the code where m_channels is created, with one device we can skip
    // copying data and just directly write to the target device.
    if (streamCount == 1) {
      m_collect->setInterleavedBuffer(out);
      m_dsp.doCycle(s_framesPerBuffer, createCallbackTime(time, flags));
      return;
    }

    Stream & stream = m_streams[streamnum];

    const int sourceChannels = m_channels.size();
    const int targetChannels = stream.outParams.channelCount;

    int remaining = s_framesPerBuffer;

    bool primaryStream = streamCount == 1;
    if (!primaryStream) {
      uint64_t available = m_frames - stream.frames;
      if (available > 0)
        writeFrames(out, streamnum, available, remaining, sourceChannels, targetChannels);
      if (remaining == 0)
        return;
    }

    {
      Radiant::Guard g(m_streamMutex);
      if (!primaryStream)
        primaryStream = stream.frames == m_frames;

      if (primaryStream) {
        CallbackTime cb = createCallbackTime(time, flags);
        int offset = m_frames % s_interleavedBufferSize;
        m_collect->setInterleavedBuffer(m_interleaved.data() + offset * sourceChannels);
        m_dsp.doCycle(s_framesPerBuffer, cb);
        m_frames += s_framesPerBuffer;
      }
    }

    uint64_t available = m_frames - stream.frames;
    if (available)
      writeFrames(out, streamnum, available, remaining, sourceChannels, targetChannels);

    if (remaining > 0) {
      memset(out, 0, sizeof(float) * remaining * targetChannels);
    }
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
      m_d->m_streams.push_back(D::Stream());
      D::Stream & s = m_d->m_streams.back();

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
        m_d->m_streams.push_back(D::Stream());
        D::Stream & s = m_d->m_streams.back();

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
      D::Stream & s = m_d->m_streams[streamnum];
      s.host = m_d.get();
      s.streamNum = streamnum;
      channels = devices[streamnum].second;

      const PaDeviceInfo * info = Pa_GetDeviceInfo(s.outParams.device);

      if (!info) {
        Radiant::error("AudioLoopPortAudio::startReadWrite # Pa_GetDeviceInfo(%d) failed",
                       s.outParams.device);
        continue;
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

      PaError err = Pa_OpenStream(& s.stream,
                                  0, // & m_inParams,
                                  & s.outParams,
                                  samplerate,
                                  s_framesPerBuffer,
                                  paClipOff,
                                  &D::paCallback,
                                  &s );

      if( err != paNoError ) {
        Radiant::error("AudioLoopPortAudio::startReadWrite # Pa_OpenStream failed (device %d, channels %d, sample rate %d)",
                       s.outParams.device, channels, samplerate);
        if (s.stream)
          Pa_CloseStream(s.stream);
        s.stream = nullptr;
        continue;
      }

      err = Pa_SetStreamFinishedCallback(s.stream, & m_d->paFinished );

      s.streamInfo = Pa_GetStreamInfo(s.stream);

      /// Right now we assume in D::callback that with one device the channels
      /// are ordered and have direct mapping.
      for (int i = 0; i < s.outParams.channelCount; ++i)
        m_d->m_channels[static_cast<int> (m_d->m_channels.size())] = Channel(static_cast<int> (streamnum), i);

      debugResonant("AudioLoopPortAudio::startReadWrite # %d channels lt = %lf, EXIT OK",
                    (int) s.outParams.channelCount,
                    (double) s.streamInfo->outputLatency);
    }

    m_d->m_isRunning = true;
    m_d->m_sampleRate = samplerate;
    m_d->m_interleaved.resize(m_d->m_channels.size() * s_interleavedBufferSize);

    bool ok = false;
    for (size_t streamnum = 0; streamnum < m_d->m_streams.size(); ++streamnum) {
      D::Stream & s = m_d->m_streams[streamnum];

      if (!s.stream) continue;

      PaError err = Pa_StartStream(s.stream);

      if (err != paNoError) {
        Radiant::error("AudioLoopPortAudio::startReadWrite # Pa_StartStream failed");
      } else {
        ok = true;
      }
    }

    return ok;
  }

  bool AudioLoopPortAudio::stop()
  {
    if(!isRunning())
      return true;

    m_d->m_isRunning = false;

    {
      /* Hack to get the audio closed in all cases (mostly for Linux). */
      Radiant::Sleep::sleepMs(200);
    }

    for (size_t num = 0; num < m_d->m_streams.size(); ++num) {
      D::Stream & s = m_d->m_streams[num];
      if (!s.stream)
        continue;
      int err = Pa_CloseStream(s.stream);
      if(err != paNoError) {
        Radiant::error("AudioLoopPortAudio::stop # Could not close stream");
      }
      s.stream = nullptr;
      s.streamInfo = nullptr;
    }
    m_d->m_streams.clear();
    m_d->m_channels.clear();

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
