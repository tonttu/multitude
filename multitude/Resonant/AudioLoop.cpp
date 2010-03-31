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

#include <Nimble/Math.hpp>

#include <Radiant/Trace.hpp>

#include <portaudio.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>


#define FRAMES_PER_BUFFER 128

namespace Resonant {

  class AudioLoop::AudioLoopInternal
  {
    public:
      AudioLoopInternal()
      : m_stream(0),
        m_streamInfo(0),
        m_startTime(0)
      {}

      static int paCallback(const void *in, void *out,
          unsigned long framesPerBuffer,
          const PaStreamCallbackTimeInfo* time,
          PaStreamCallbackFlags status,
          void * self);

      static void paFinished(void * self);

      PaStreamParameters m_inParams;
      PaStreamParameters m_outParams;

      PaStream * m_stream;
      const PaStreamInfo * m_streamInfo;
      PaTime     m_startTime;
  };

  using Radiant::FAILURE;

  AudioLoop::AudioLoop()
  : m_isRunning(false)
  {
    m_d = new AudioLoopInternal();

    init();

    bzero( &m_d->m_inParams,  sizeof(PaStreamParameters));
    bzero( &m_d->m_outParams, sizeof(PaStreamParameters));
  }

  AudioLoop::~AudioLoop()
  {
    assert(isRunning() == false);

    delete m_d;
  }

  int AudioLoop::outChannels() const
  {
    return m_d->m_outParams.channelCount;
  }

  bool AudioLoop::init()
  {
    static bool once = false;

    if(once)
      return true;

    once = true;

    PaError e = Pa_Initialize();
    if(e != paNoError) {
      Radiant::error("AudioLoop::init # %s", Pa_GetErrorText(e));
      return false;
    }

    return true;
  }

  bool AudioLoop::cleanup()
  {
    PaError e = Pa_Terminate();
    if(e != paNoError) {
      Radiant::error("AudioLoop::cleanup # %s", Pa_GetErrorText(e));
      return false;
    }

    return true;
  }

  bool AudioLoop::startReadWrite(int samplerate, int channels)
  {
    assert(!isRunning());

    m_d->m_stream = 0;
    m_d->m_streamInfo = 0;

    bzero( &m_d->m_inParams,  sizeof(m_d->m_inParams));
    bzero( &m_d->m_outParams, sizeof(m_d->m_outParams));

    const char * devkey = getenv("RESONANT_DEVICE");

    if(!devkey) {

      m_d->m_outParams.device = Pa_GetDefaultOutputDevice();
      if(m_d->m_outParams.device == paNoDevice) {
        Radiant::error("AudioLoop::startReadWrite # No default output device available");
        return false;
      }
    }
    else {

      char * end = 0;
      int i = strtol(devkey, & end, 10);

      long decoded = end - devkey;
      if(decoded == (long) strlen(devkey)) {
        m_d->m_outParams.device = i;
        Radiant::info("AudioLoop::startReadWrite # Selected device %d (%s)",
                       (int) m_d->m_outParams.device, devkey);

      }
      else {
        int n = Pa_GetDeviceCount();

        for( i = 0; i < n; i++) {
          const PaDeviceInfo * info = Pa_GetDeviceInfo(i);
          if(strstr(info->name, devkey) != 0) {
            m_d->m_outParams.device = i;

            Radiant::debug("AudioLoop::startReadWrite # Selected device %d %s",
                           (int) m_d->m_outParams.device, info->name);
            break;
          }
        }
      }
    }

    const PaDeviceInfo * info = Pa_GetDeviceInfo(m_d->m_outParams.device);

    Radiant::info("AudioLoop::startReadWrite # Got audio device %d = %s",
          (int) m_d->m_outParams.device, info->name);

    if(Radiant::enabledVerboseOutput()) {
      int n = Pa_GetDeviceCount();

      for(int i = 0; i < n; i++) {
         const PaDeviceInfo * info2 = Pa_GetDeviceInfo(i);
         const PaHostApiInfo * apiinfo = Pa_GetHostApiInfo(info2->hostApi);
         Radiant::debug("AudioLoop::startReadWrite # Available %d: %s (API = %s)",
                        i, info2->name, apiinfo->name);
      }
    }

    // int minchans = Nimble::Math::Min(info->maxInputChannels, info->maxOutputChannels);
    int minchans = info->maxOutputChannels;

    if(channels < minchans) {
      Radiant::info("AudioLoop::startReadWrite # Expanding to %d channels",
                    minchans);
      channels = minchans;
    }

    Radiant::info("AudioLoop::startReadWrite # channels = %d limits = %d %d",
                  channels, info->maxInputChannels, info->maxOutputChannels);

    // channels = 26;

    m_d->m_outParams.channelCount = channels;
    m_d->m_outParams.sampleFormat = paFloat32;
    m_d->m_outParams.suggestedLatency =
      Pa_GetDeviceInfo( m_d->m_outParams.device )->defaultLowOutputLatency;
    m_d->m_outParams.hostApiSpecificStreamInfo = 0;

    m_d->m_inParams = m_d->m_outParams;
    m_d->m_inParams.device = Pa_GetDefaultInputDevice();

    m_continue = true;

    PaError err = Pa_OpenStream(& m_d->m_stream,
                                0, // & m_inParams,
                                & m_d->m_outParams,
                                samplerate,
                                FRAMES_PER_BUFFER,
                                paClipOff,
                                m_d->paCallback,
                                this );

    if( err != paNoError ) {
      Radiant::error("AudioLoop::startReadWrite # Pa_OpenStream failed");
      return false;
    }

    err = Pa_SetStreamFinishedCallback(m_d->m_stream, & m_d->paFinished );

    m_d->m_streamInfo = Pa_GetStreamInfo(m_d->m_stream);

    err = Pa_StartStream(m_d->m_stream);

    if( err != paNoError ) {
      Radiant::error("AudioLoop::startReadWrite # Pa_StartStream failed");
      return false;
    }

    m_d->m_startTime = Pa_GetStreamTime(m_d->m_stream);

    m_isRunning = true;

    Radiant::debug("AudioLoop::startReadWrite # %d channels lt = %lf, EXIT OK",
		   (int) m_d->m_outParams.channelCount, 
		   (double) m_d->m_streamInfo->outputLatency);

    return true;
  }

  bool AudioLoop::stop()
  {
    if(!isRunning())
      return true;

    m_continue = false;

    int i = 0;

    int err = Pa_StopStream(m_d->m_stream);
    if(err != paNoError) {
      trace(Radiant::FAILURE, "AudioLoop::stop # Could not stop the stream");

      while(isRunning() && i < 100) {
        Pa_Sleep(5);
        i++;
      }
    }
    else
      m_isRunning = false;

    err = Pa_CloseStream(m_d->m_stream);
    if(err != paNoError) {
      trace(Radiant::FAILURE, "AudioLoop::stop # Could not close stream");
    }

    m_d->m_stream = 0;
    m_d->m_streamInfo = 0;

    return true;
  }

  int AudioLoop::AudioLoopInternal::paCallback(const void *in, void *out,
                unsigned long framesPerBuffer,
                const PaStreamCallbackTimeInfo * /*time*/,
                PaStreamCallbackFlags /*status*/,
                void * self)
  {
    AudioLoop * au = (AudioLoop *) self;

    int r = au->callback(in, out, framesPerBuffer/*, time, status*/);

    return au->m_continue ? r : paComplete;
  }

  void AudioLoop::AudioLoopInternal::paFinished(void * self)
  {
    ((AudioLoop *) self)->finished();
    Radiant::debug("AudioLoop::paFinished # %p", self);
  }


  void AudioLoop::finished()
  {
    m_isRunning = false;
  }

}
