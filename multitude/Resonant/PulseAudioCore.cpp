#include "PulseAudioCore.hpp"

#include <Radiant/Trace.hpp>
#include <Radiant/Sleep.hpp>

namespace Resonant
{
  void g_contextStateCb(pa_context * c, void * self)
  {
    if(c && self) {
      PulseAudioCore * pm = static_cast<PulseAudioCore*>(self);
      if(pm->context() == c) {
        pm->contextChange(pa_context_get_state(c));
      }
    }
  }

  void g_eventCb(pa_context * c, pa_subscription_event_type_t t, uint32_t idx, void * self)
  {
    if(c && self) {
      PulseAudioCore * pm = static_cast<PulseAudioCore*>(self);
      if(pm->context() == c) {
        int type = t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK;
        pa_subscription_event_type_t e = (pa_subscription_event_type_t)(t & PA_SUBSCRIPTION_EVENT_TYPE_MASK);
        if(type == PA_SUBSCRIPTION_EVENT_CLIENT) {
          pm->clientEvent(e, idx);
        } else if(type == PA_SUBSCRIPTION_EVENT_SINK_INPUT) {
          pm->sinkInputEvent(e, idx);
        } else if(type == PA_SUBSCRIPTION_EVENT_SINK) {
          pm->sinkEvent(e, idx);
        }
      }
    }
  }

  void g_contextSuccessCb(pa_context * c, int success, void * self)
  {
    if(c && self) {
      PulseAudioCore * pm = static_cast<PulseAudioCore*>(self);
      if(pm->context() == c) {
        pm->contextSuccess(success);
      }
    }
  }

  void g_clientInfoCb(pa_context * c, const pa_client_info * i, int eol, void * self)
  {
    if(c && self && i) {
      PulseAudioCore * pm = static_cast<PulseAudioCore*>(self);
      if(pm->context() == c) {
        pm->clientInfo(i, eol);
      }
    }
  }

  void g_moduleCb(pa_context * c, uint32_t idx, void * self)
  {
    if(c && self) {
      PulseAudioCore * pm = static_cast<PulseAudioCore*>(self);
      if(pm->context() == c) {
        pm->moduleLoaded(idx);
      }
    }
  }

  void g_sinkInputInfoCb(pa_context * c, const pa_sink_input_info * i, int eol, void * self)
  {
    if(c && self && i) {
      PulseAudioCore * pm = static_cast<PulseAudioCore*>(self);
      if(pm->context() == c) {
        pm->sinkInputInfo(i, eol);
      }
    }
  }

  void g_sinkInfoCb(pa_context * c, const pa_sink_info * i, int eol, void * self)
  {
    if(c && self && i) {
      PulseAudioCore * pm = static_cast<PulseAudioCore*>(self);
      if(pm->context() == c) {
        pm->sinkInfo(i, eol);
      }
    }
  }

  PulseAudioCore::PulseAudioCore()
    : m_context(0),
    m_mainloopApi(0),
    m_running(true)
  {
  }

  PulseAudioCore::~PulseAudioCore()
  {
  }

  void PulseAudioCore::contextSuccess(int success)
  {
    if(success) {
      Radiant::info("PulseManager ready");
    } else {
      Radiant::error("PulseManager initialization failed");
    }
  }


  void PulseAudioCore::childLoop()
  {
    while(m_running) {
      runClient();
      if(!m_running) break;

      Radiant::Sleep::sleepS(2);
    }
  }

  void PulseAudioCore::runClient()
  {
    pa_mainloop * mainloop = pa_mainloop_new();
    if(!mainloop) {
      Radiant::error("pa_mainloop_new() failed");
      return;
    }

    m_mainloopApi = pa_mainloop_get_api(mainloop);

    if(!(m_context = pa_context_new(m_mainloopApi, "Cornerstone"))) {
      Radiant::error("pa_context_new() failed.");
      pa_mainloop_free(mainloop);
      return;
    }

    pa_context_set_state_callback(m_context, g_contextStateCb, this);

    pa_context_connect(m_context, 0, (pa_context_flags_t)0, 0);

    int ret;
    if(pa_mainloop_run(mainloop, &ret) < 0) {
      Radiant::error("pa_mainloop_run() failed");
    }

    pa_context_unref(m_context);
    pa_mainloop_free(mainloop);
    m_context = 0;
    m_mainloopApi = 0;
  }

  void PulseAudioCore::contextChange(pa_context_state_t state)
  {
    switch(state) {
      case PA_CONTEXT_CONNECTING:
      case PA_CONTEXT_AUTHORIZING:
      case PA_CONTEXT_SETTING_NAME:
        break;

      case PA_CONTEXT_READY:
        break;

      case PA_CONTEXT_FAILED:
        Radiant::error("PulseAudio connection failure: %s", pa_strerror(pa_context_errno(m_context)));
      case PA_CONTEXT_TERMINATED:
        m_mainloopApi->quit(m_mainloopApi, 0);
        break;

      default:
        Radiant::error("Unknown PulseAudio context state: %d", state);
    }
  }

  void PulseAudioCore::clientEvent(pa_subscription_event_type_t, uint32_t)
  {
  }

  void PulseAudioCore::sinkInputEvent(pa_subscription_event_type_t, uint32_t)
  {
  }

  void PulseAudioCore::sinkEvent(pa_subscription_event_type_t, uint32_t)
  {
  }

  void PulseAudioCore::clientInfo(const pa_client_info *, int)
  {
  }

  void PulseAudioCore::sinkInputInfo(const pa_sink_input_info *, int)
  {
  }

  void PulseAudioCore::sinkInfo(const pa_sink_info *, int)
  {
  }

  void PulseAudioCore::moduleLoaded(uint32_t)
  {
  }
}
