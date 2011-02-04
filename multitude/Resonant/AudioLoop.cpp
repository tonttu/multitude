/* COPYRIGHT
 *
 * This file is part of Resonant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Resonant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "AudioLoop.hpp"
#include "AudioLoopPriv.hpp"

#include <Nimble/Math.hpp>

#include <Radiant/Trace.hpp>
#include <Radiant/StringUtils.hpp>

#include <string>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>


#define FRAMES_PER_BUFFER 128

namespace Resonant {
#ifdef RADIANT_WIN32
  __declspec(thread) int AudioLoop::AudioLoopInternal::s_currentStream = 0;
#else
  __thread int AudioLoop::AudioLoopInternal::s_currentStream = 0;
#endif

  using Radiant::FAILURE;

  AudioLoop::AudioLoop()
    : m_isRunning(false),
    m_initialized(false)
  {
    m_d = new AudioLoopInternal();

    // Initializes PortAudio / increases the usage counter.
    PaError e = Pa_Initialize();
    if(e == paNoError) {
      m_initialized = true;
    } else {
      Radiant::error("AudioLoop::init # %s", Pa_GetErrorText(e));
    }
  }

  AudioLoop::~AudioLoop()
  {
    if (isRunning())
      Radiant::error("AudioLoop::~AudioLoop(): audio still running");

    delete m_d;

    if(m_initialized) {
      // Decreases usage counter, if this is the last AudioLoop using PA, the library is closed.
      // From Pa API: Pa_Terminate() MUST be called before exiting a program
      //              which uses PortAudio. Failure to do so may result in
      //              serious resource leaks, such as audio devices not being
      //              available until the next reboot.
      PaError e = Pa_Terminate();
      if(e != paNoError) {
        Radiant::error("AudioLoop::cleanup # %s", Pa_GetErrorText(e));
      }
    }
  }

  int AudioLoop::outChannels() const
  {
    return m_d->m_channels.size();
  }

  bool AudioLoop::startReadWrite(int samplerate, int channels)
  {
    assert(!isRunning());

    const char * chankey = getenv("RESONANT_OUTCHANNELS");
    int forcechans = -1;
    if(chankey != 0) {
      forcechans = atoi(chankey);
    }

    std::vector<std::string> devicenames;
    std::vector<int> channel_requests;

    const char * devs = getenv("RESONANT_DEVICES");
    const char * devname = getenv("RESONANT_DEVICE");
    if (devs) {
      using namespace Radiant::StringUtils;
      forcechans = -1;
      StringList list;
      split(devs, ";", list);

      for (StringList::iterator it = list.begin(); it != list.end(); ++it) {
        const char * dev = it->c_str();
        char * end = 0;
        int c = strtol(dev, &end, 10);
        if (*end++ != ':') {
          Radiant::error("Invalid RESONANT_DEVICES, should be:  CHANNELS:DEVICE;CHANNELS:DEVICE...");
          return false;
        }
        Radiant::info("Requesting device %s with %d channels", end, c);
        devicenames.push_back(end);
        channel_requests.push_back(c);
      }
    } else if (devname) {
      devicenames.push_back(devname);
      channel_requests.push_back(channels);
    }

    if(devicenames.empty()) {
      m_d->m_streams.push_back(Stream());
      Stream & s = m_d->m_streams.back();
      channel_requests.push_back(channels);

      s.outParams.device = Pa_GetDefaultOutputDevice();
      if(s.outParams.device == paNoDevice) {
        Radiant::error("AudioLoop::startReadWrite # No default output device available");
        return false;
      }
      Radiant::debug("AudioLoop::startReadWrite # Selected default output device %d", s.outParams.device);
    }
    else {
      for (size_t dev = 0; dev < devicenames.size(); ++dev) {
        m_d->m_streams.push_back(Stream());
        Stream & s = m_d->m_streams.back();

        const char * devkey = devicenames[dev].c_str();
        char * end = 0;
        int i = strtol(devkey, & end, 10);

        long decoded = end - devkey;
        if(decoded == (long) strlen(devkey)) {
          s.outParams.device = i;
          Radiant::info("AudioLoop::startReadWrite # Selected device %d (%s)", (int) s.outParams.device, devkey);
        }
        else {
          int n = Pa_GetDeviceCount();

          for( i = 0; i < n; i++) {
            const PaDeviceInfo * info = Pa_GetDeviceInfo(i);
            if(strstr(info->name, devkey) != 0) {
              if (channel_requests[dev] > info->maxOutputChannels) {
                Radiant::info("Skipping device %d, not enough output channels (%d < %d)",
                              info->maxOutputChannels, channel_requests[dev]);
                continue;
              }

              s.outParams.device = i;

              Radiant::info("AudioLoop::startReadWrite # Selected device %d %s",
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

    for (size_t streamnum = 0; streamnum < m_d->m_streams.size(); ++streamnum) {
      Stream & s = m_d->m_streams[streamnum];
      /// @todo m_barrier isn't released ever
      s.m_barrier = new Radiant::Semaphore(0);
      channels = channel_requests[streamnum];
      Radiant::info("channels: %d", channels);

      const PaDeviceInfo * info = Pa_GetDeviceInfo(s.outParams.device);

      Radiant::info("AudioLoop::startReadWrite # Got audio device %d = %s",
                    (int) s.outParams.device, info->name);

      if(Radiant::enabledVerboseOutput()) {
        int n = Pa_GetDeviceCount();

        for(int i = 0; i < n; i++) {
           const PaDeviceInfo * info2 = Pa_GetDeviceInfo(i);
           const PaHostApiInfo * apiinfo = Pa_GetHostApiInfo(info2->hostApi);
          Radiant::info("AudioLoop::startReadWrite # Available %d: %s (API = %s)",
                        i, info2->name, apiinfo->name);
        }
      }

      // int minchans = Nimble::Math::Min(info->maxInputChannels, info->maxOutputChannels);
      int minchans = info->maxOutputChannels;

      if(forcechans > 0) {
        channels = forcechans;
      }
      else if(minchans < channels || (!devs && channels != minchans)) {
        Radiant::info("AudioLoop::startReadWrite # Expanding to %d channels",
                       minchans);
        channels = minchans;
      }

      Radiant::info("AudioLoop::startReadWrite # channels = %d limits = %d %d",
                     channels, info->maxInputChannels, info->maxOutputChannels);

      // channels = 26;

      s.outParams.channelCount = channels;
      s.outParams.sampleFormat = paFloat32;
      s.outParams.suggestedLatency =
        Pa_GetDeviceInfo( s.outParams.device )->defaultLowOutputLatency;
      s.outParams.hostApiSpecificStreamInfo = 0;

      s.inParams = s.outParams;
      s.inParams.device = Pa_GetDefaultInputDevice();

      m_continue = true;

      m_d->cb.push_back(std::make_pair(this, streamnum));

      PaError err = Pa_OpenStream(& s.stream,
                                  0, // & m_inParams,
                                  & s.outParams,
                                  samplerate,
                                  FRAMES_PER_BUFFER,
                                  paClipOff,
                                  m_d->paCallback,
                                  &m_d->cb.back() );

      if( err != paNoError ) {
        Radiant::error("AudioLoop::startReadWrite # Pa_OpenStream failed (device %d, channels %d, sample rate %d)",
                       s.outParams.device, channels, samplerate);
        return false;
      }

      err = Pa_SetStreamFinishedCallback(s.stream, & m_d->paFinished );

      s.streamInfo = Pa_GetStreamInfo(s.stream);

      for (int i = 0; i < s.outParams.channelCount; ++i)
        m_d->m_channels[m_d->m_channels.size()] = Channel(streamnum, i);

      Radiant::info("AudioLoop::startReadWrite # %d channels lt = %lf, EXIT OK",
         (int) s.outParams.channelCount,
         (double) s.streamInfo->outputLatency);
    }

    m_d->m_streamBuffers.resize(m_d->m_streams.size());
    m_d->m_sem.release(m_d->m_streams.size());

    for (size_t streamnum = 0; streamnum < m_d->m_streams.size(); ++streamnum) {
      Stream & s = m_d->m_streams[streamnum];

      PaError err = Pa_StartStream(s.stream);

      if( err != paNoError ) {
        Radiant::error("AudioLoop::startReadWrite # Pa_StartStream failed");
        return false;
      }

      s.startTime = Pa_GetStreamTime(s.stream);
    }

    m_isRunning = true;

    return true;
  }

  bool AudioLoop::stop()
  {
    if(!isRunning())
      return true;

    m_isRunning = false;

    for (size_t num = 0; num < m_d->m_streams.size(); ++num) {
      Stream & s = m_d->m_streams[num];
      int err = Pa_CloseStream(s.stream);
      if(err != paNoError) {
        Radiant::error("AudioLoop::stop # Could not close stream");
      }
      s.stream = 0;
      s.streamInfo = 0;
      m_d->m_channels.erase(num);
    }

    return true;
  }

  int AudioLoop::AudioLoopInternal::paCallback(const void *in, void *out,
                unsigned long framesPerBuffer,
                const PaStreamCallbackTimeInfo * /*time*/,
                PaStreamCallbackFlags /*status*/,
                void * self)
  {
    std::pair<AudioLoop*, int> stream = *reinterpret_cast<std::pair<AudioLoop*, int>*>(self);
    stream.first->m_d->s_currentStream = stream.second;

    int r = stream.first->callback(in, out, framesPerBuffer/*, time, status*/);

    return stream.first->m_continue ? r : paComplete;
  }

  void AudioLoop::AudioLoopInternal::paFinished(void * self)
  {
    std::pair<AudioLoop*, int> stream = *reinterpret_cast<std::pair<AudioLoop*, int>*>(self);
    stream.first->finished();
    Radiant::debug("AudioLoop::paFinished # %p", stream.first);
  }


  void AudioLoop::finished()
  {
    m_isRunning = false;
  }

}
