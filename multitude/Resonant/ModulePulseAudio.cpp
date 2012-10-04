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

#include <Radiant/Platform.hpp>

#ifdef RADIANT_LINUX

#include "ModulePulseAudio.hpp"
#include "Resonant.hpp"

#include <Radiant/Sleep.hpp>

namespace
{
  void s_streamStateCb(pa_stream * p, void * self)
  {
    Resonant::ModulePulseAudio * mpa = static_cast<Resonant::ModulePulseAudio*>(self);
    mpa->streamState(pa_stream_get_state(p));
  }

  void s_streamRequestCb(pa_stream * p, size_t nbytes, void * self)
  {
    if(p && self) {
      Resonant::ModulePulseAudio * mpa = static_cast<Resonant::ModulePulseAudio*>(self);
      mpa->dataAvailable(p, nbytes);
    }
  }
}

namespace Resonant
{
  ModulePulseAudio::ModulePulseAudio(const QString & monitorName, uint32_t sinkInput)
    : Module(0), m_ready(false), m_stream(0), m_monitorName(monitorName),
    m_sinkInput(sinkInput), m_buffer(0), m_bufferSize(0),
    m_syncCount(0), m_canSync(false)
  {
    // Radiant::info("ModulePulseAudio::ModulePulseAudio # %p", this);
  }

  ModulePulseAudio::~ModulePulseAudio()
  {
    // Radiant::info("ModulePulseAudio::~ModulePulseAudio # %p", this);
  }

  void ModulePulseAudio::contextChange(pa_context_state_t state)
  {
    if(state == PA_CONTEXT_READY && !m_stream) {
      openStream();
    } else {
      PulseAudioCore::contextChange(state);
    }
  }

  void ModulePulseAudio::streamState(pa_stream_state_t state)
  {
    switch(state) {
    case PA_STREAM_UNCONNECTED:
    case PA_STREAM_CREATING:
      break;

    case PA_STREAM_READY:
      debugResonant("Audio recording started");
      m_ready = true;
      break;

    case PA_STREAM_FAILED:
      Radiant::error("Failed to open PulseAudio stream: %s",
                     pa_strerror(pa_context_errno(m_context)));
      //openStream();
      //break;

    case PA_STREAM_TERMINATED:
      /// @todo close stuff cleanly
      break;
    }
  }

  void ModulePulseAudio::openStream()
  {
    //if(m_stream)
      //closeStream();

    static const pa_sample_spec ss = {PA_SAMPLE_FLOAT32, 44100, 1};

    debugResonant("Starting capture %d", pa_sample_spec_valid(&ss));
    if(!(m_stream = pa_stream_new(m_context, "Cornerstone capture", &ss, NULL))) {
      restart();
      return;
    }

    debugResonant("setting callbacks");
    pa_stream_set_state_callback(m_stream, s_streamStateCb, this);
    pa_stream_set_read_callback(m_stream, s_streamRequestCb, this);

    debugResonant("monitoring %d", m_sinkInput);
    if(pa_stream_set_monitor_stream(m_stream, m_sinkInput) != 0) {
      restart();
      return;
    }

    debugResonant("starting record");
    pa_buffer_attr attr;
    attr.maxlength = attr.tlength = attr.prebuf = attr.minreq = (uint32_t)-1;
    double latency = getenv("MODULE_PULSEAUDIO_FRAGSIZE")
                     ? atof(getenv("MODULE_PULSEAUDIO_FRAGSIZE"))
                     : 30.0;
    attr.fragsize = pa_usec_to_bytes(latency * 1000, &ss);
    if(getenv("MODULE_PULSEAUDIO_BUFFERSIZE"))
      attr.maxlength = pa_usec_to_bytes(atof(getenv("MODULE_PULSEAUDIO_BUFFERSIZE")) * 1000, &ss);

    if(pa_stream_connect_record(m_stream, m_monitorName.toUtf8().data(), &attr,
                                (pa_stream_flags_t)
                                (PA_STREAM_INTERPOLATE_TIMING |
                                 PA_STREAM_ADJUST_LATENCY |
                                 PA_STREAM_AUTO_TIMING_UPDATE)) < 0) {
      restart();
      return;
    }
    debugResonant("record request done");
  }

  bool ModulePulseAudio::prepare(int & channelsIn, int & channelsOut)
  {
    channelsIn = 0;
    channelsOut = 1;
    return true;
  }

  void ModulePulseAudio::dataAvailable(pa_stream *, size_t)
  {
  }

  void ModulePulseAudio::process(float **, float ** out, int n, const CallbackTime &)
  {
    if(!m_ready) {
      memset(out[0], 0, n*4);
      return;
    }

    if(m_syncCount < 20 && m_bufferSize > 0 && m_canSync) {
      pa_threaded_mainloop_lock(m_mainloop);
      size_t bsize = pa_stream_readable_size(m_stream);
      pa_threaded_mainloop_unlock(m_mainloop);
      if(bsize > 0) {
        Radiant::info("ModulePulseAudio dropping %lu bytes", m_bufferSize);
        m_syncCount++;
        m_bufferSize = 0;
        m_canSync = false;
      }
    }

    int pos = std::min<int>(n, m_bufferSize);
    if(pos)
      memcpy(out[0], m_buffer, pos*4);

    m_bufferSize -= pos;
    m_buffer += pos;
    n -= pos;

    if(n) {
      const void * data;
      pa_threaded_mainloop_lock(m_mainloop);
      if(!m_stream) {
        m_bufferSize = 0;
      } else {
        pa_stream_peek(m_stream, &data, &m_bufferSize);
        if(m_bufferData.size() < m_bufferSize)
          m_bufferData.resize(m_bufferSize);
        m_buffer = &m_bufferData[0];

        memcpy(m_buffer, data, m_bufferSize);
        pa_stream_drop(m_stream);
        pa_threaded_mainloop_unlock(m_mainloop);

        m_bufferSize /= 4;

        if(m_bufferSize) {
          int tmp = std::min<int>(n, m_bufferSize);
          memcpy(out[0] + pos, m_buffer, tmp*4);
          m_bufferSize -= tmp;
          m_buffer += tmp;
          n -= tmp;
          pos += tmp;
          m_canSync = true;
        }
      }
    }

    if(n) {
      memset(out[0] + pos, 0, n*4);
    }
  }

  bool ModulePulseAudio::stop()
  {
    if(m_mainloop)
      pa_threaded_mainloop_lock(m_mainloop);
    m_running = false;
    m_ready = false;
    if(m_mainloop) {
      pa_threaded_mainloop_signal(m_mainloop, 0);
      pa_threaded_mainloop_unlock(m_mainloop);
    }
    waitEnd();
    return true;
  }

  void ModulePulseAudio::beforeShutdown()
  {
    if(m_stream && m_mainloop) {
      pa_threaded_mainloop_lock(m_mainloop);
      m_ready = false;
      pa_stream_disconnect(m_stream);
      pa_stream_unref(m_stream);
      m_stream = 0;
      pa_threaded_mainloop_unlock(m_mainloop);
    }
  }
}

#endif
