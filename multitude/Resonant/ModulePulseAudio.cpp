#include "ModulePulseAudio.hpp"

#include <Radiant/Sleep.hpp>

namespace
{
  using namespace Resonant;
  void s_streamStateCb(pa_stream * p, void * self)
  {
    ModulePulseAudio * mpa = static_cast<ModulePulseAudio*>(self);
    mpa->streamState(pa_stream_get_state(p));
  }

  void s_streamRequestCb(pa_stream * p, size_t nbytes, void * self)
  {
    if(p && self) {
      ModulePulseAudio * mpa = static_cast<ModulePulseAudio*>(self);
      mpa->dataAvailable(p, nbytes);
    }
  }
}

namespace Resonant
{
  ModulePulseAudio::ModulePulseAudio(const std::string & monitorName, uint32_t sinkInput)
    : Module(0), m_ready(false), m_stream(0), m_monitorName(monitorName),
    m_sinkInput(sinkInput), m_buffer(0), m_bufferSize(0)
  {
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
      Radiant::debug("Audio recording started");
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

    Radiant::debug("Starting capture %d", pa_sample_spec_valid(&ss));
    if(!(m_stream = pa_stream_new(m_context, "Cornerstone capture", &ss, NULL))) {
      restart();
      return;
    }

    Radiant::debug("setting callbacks");
    pa_stream_set_state_callback(m_stream, s_streamStateCb, this);
    pa_stream_set_read_callback(m_stream, s_streamRequestCb, this);

    Radiant::debug("monitoring %d", m_sinkInput);
    if(pa_stream_set_monitor_stream(m_stream, m_sinkInput) != 0) {
      restart();
      return;
    }

    Radiant::debug("starting record");
    if(pa_stream_connect_record(m_stream, m_monitorName.c_str(), NULL,
                                (pa_stream_flags_t)
                                (PA_STREAM_INTERPOLATE_TIMING |
                                 PA_STREAM_ADJUST_LATENCY |
                                 PA_STREAM_AUTO_TIMING_UPDATE)) < 0) {
      restart();
      return;
    }
    Radiant::debug("record request done");
  }

  bool ModulePulseAudio::prepare(int & channelsIn, int & channelsOut)
  {
    channelsIn = 0;
    channelsOut = 1;
    return true;
  }

  void ModulePulseAudio::dataAvailable(pa_stream * p, size_t nbytes)
  {
  }

  void ModulePulseAudio::process(float **, float ** out, int n)
  {
    if(!m_ready) {
      memset(out[0], 0, n*4);
      return;
    }

    int pos = Nimble::Math::Min<int>(n, m_bufferSize);
    if(pos)
      memcpy(out[0], m_buffer, pos*4);

    m_bufferSize -= pos;
    m_buffer += pos;
    n -= pos;

    if(n) {
      const void * data;
      pa_threaded_mainloop_lock(m_mainloop);
      pa_stream_peek(m_stream, &data, &m_bufferSize);
      if(m_bufferData.size() < m_bufferSize)
        m_bufferData.resize(m_bufferSize);
      m_buffer = &m_bufferData[0];

      memcpy(m_buffer, data, m_bufferSize);
      pa_stream_drop(m_stream);
      pa_threaded_mainloop_unlock(m_mainloop);

      m_bufferSize /= 4;

      if(m_bufferSize) {
        int tmp = Nimble::Math::Min<int>(n, m_bufferSize);
        memcpy(out[0] + pos, m_buffer, tmp*4);
        m_bufferSize -= tmp;
        m_buffer += tmp;
        n -= tmp;
        pos += tmp;
      }
    }

    if(n) {
      memset(out[0] + pos, 0, n*4);
    }
  }
}
