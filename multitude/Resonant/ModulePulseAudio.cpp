#include "ModulePulseAudio.hpp"

namespace
{
  using namespace Resonant;
  void s_streamStateCb(pa_stream * p, void * self)
  {
    /// @todo check the state blahblah
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
  ModulePulseAudio::ModulePulseAudio(uint32_t sinkInput)
    : Module(0), m_ready(false), m_sinkInput(sinkInput)
  {
  }

  void ModulePulseAudio::contextChange(pa_context_state_t state)
  {
    static const pa_sample_spec ss = {PA_SAMPLE_FLOAT32LE, 44100, 2};

    if(state == PA_CONTEXT_READY && !m_stream) {
      if(!(m_stream = pa_stream_new(m_context, "Cornerstone capture", &ss, NULL))) {
        m_mainloopApi->quit(m_mainloopApi, 0);
        return;
      }

      pa_stream_set_state_callback(m_stream, s_streamStateCb, this);
      pa_stream_set_read_callback(m_stream, s_streamRequestCb, this);

      if(pa_stream_set_monitor_stream(m_stream, m_sinkInput) != 0) {
        m_mainloopApi->quit(m_mainloopApi, 0);
      }

      if(pa_stream_connect_record(m_stream, NULL, NULL,
                                  (pa_stream_flags_t)
                                  (PA_STREAM_INTERPOLATE_TIMING |
                                  PA_STREAM_ADJUST_LATENCY |
                                  PA_STREAM_AUTO_TIMING_UPDATE)) < 0) {
        m_mainloopApi->quit(m_mainloopApi, 0);
      }
    } else {
      PulseAudioCore::contextChange(state);
    }
  }

  bool ModulePulseAudio::prepare(int & channelsIn, int & channelsOut)
  {
    channelsIn = 0;
    channelsOut = 1;
    return true;
  }

  void ModulePulseAudio::dataAvailable(pa_stream * p, size_t nbytes)
  {
    m_ready = true;
  }

  void ModulePulseAudio::process(float **, float ** out, int n)
  {
    if(m_ready) {
      const float * data;
      size_t * bytes = 0;
      pa_stream_peek(m_stream, (const void**)&data, bytes);
      if(bytes) {
        memcpy(out, data, *bytes < n*4 ? *bytes : n*4);
        /// @todo could drop more than we want
        //pa_stream_drop(m_stream);
      }

    }
/*    const int channels = 2;
    for(int i = 0; i < channels; ++i) {
      float * dest = out[i];
      float * sentinel = dest + n;

      for(; dest < sentinel; ++dest) {
        //*dest += stuff;
      }
    }*/
  }
}
