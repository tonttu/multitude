/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include <Radiant/Platform.hpp>

#ifdef RADIANT_LINUX

#include "PulseAudioCore.hpp"
#include "Resonant.hpp"

#include <Radiant/Trace.hpp>
#include <Radiant/Sleep.hpp>

#include <stdio.h>

// these four for stat()
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

namespace
{
  void s_moduleInfoCb(pa_context * c, const pa_module_info * i, int eol, void * self)
  {
    if(c && self) {
      Resonant::PulseAudioCleaner * pac = static_cast<Resonant::PulseAudioCleaner*>(self);
      if(pac->context() == c) {
        pac->moduleInfo(i, eol);
      }
    }
  }

  void s_unloadSuccessCb(pa_context * c, int /*success*/, void * self)
  {
    if(c && self) {
      Resonant::PulseAudioCleaner * pac = static_cast<Resonant::PulseAudioCleaner*>(self);
      if(pac->context() == c) {
        pac->ready();
      }
    }
  }
}

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
    m_mainloop(0),
    m_mainloopApi(0),
    m_running(true),
    m_restart(false),
    m_retry(true)
  {
  }

  PulseAudioCore::~PulseAudioCore()
  {
  }

  void PulseAudioCore::contextSuccess(int success)
  {
    if(success) {
      Radiant::info("PulseAudio ready");
    } else {
      Radiant::error("PulseAudio initialization failed");
    }
  }


  void PulseAudioCore::childLoop()
  {
    while(m_running) {
      runClient();
      if(!m_running || !m_retry) break;

      Radiant::Sleep::sleepS(2);
    }
  }

  void PulseAudioCore::runClient()
  {
    debugResonant("%p runClient", this);
    m_restart = false;
    m_mainloop = pa_threaded_mainloop_new();
    if(!m_mainloop) {
      Radiant::error("pa_mainloop_new() failed");
      return;
    }

    m_mainloopApi = pa_threaded_mainloop_get_api(m_mainloop);

    debugResonant("%p pa_context_new", this);
    if(!(m_context = pa_context_new(m_mainloopApi, "Cornerstone"))) {
      Radiant::error("pa_context_new() failed.");
      pa_threaded_mainloop_free(m_mainloop);
      return;
    }

    pa_context_set_state_callback(m_context, g_contextStateCb, this);

    pa_context_connect(m_context, 0, (pa_context_flags_t)0, 0);

    debugResonant("%p pa_mainloop_run", this);
    if(pa_threaded_mainloop_start(m_mainloop) != 0) {
      Radiant::error("pa_threaded_mainloop_run() failed");
    }

    pa_threaded_mainloop_lock(m_mainloop);
    while(m_running && !m_restart) {
      pa_threaded_mainloop_wait(m_mainloop);
    }
    pa_threaded_mainloop_unlock(m_mainloop);

    beforeShutdown();

    debugResonant("%p pa_mainloop_run exit", this);
    pa_threaded_mainloop_stop(m_mainloop);
    pa_context_unref(m_context);
    pa_threaded_mainloop_free(m_mainloop);
    m_context = 0;
    m_mainloopApi = 0;
  }

  void PulseAudioCore::restart()
  {
    m_restart = true;
    pa_threaded_mainloop_signal(m_mainloop, 0);
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
        m_restart = true;
        pa_threaded_mainloop_signal(m_mainloop, 0);
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

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  PulseAudioCleaner::PulseAudioCleaner() : m_counter(0)
  {
    m_retry = false;
  }

  PulseAudioCleaner::~PulseAudioCleaner()
  {
  }

  void PulseAudioCleaner::clean(bool force)
  {
    static bool once = false;
    if(once && !force)
      return;
    once = true;

    PulseAudioCleaner * pac = new PulseAudioCleaner;
    pac->run();
    pac->waitEnd();
    delete pac;
  }


  void PulseAudioCleaner::contextChange(pa_context_state_t state)
  {
    if(state == PA_CONTEXT_READY) {
      ++m_counter;
      pa_operation * op = pa_context_get_module_info_list(m_context, s_moduleInfoCb, this);
      pa_operation_unref(op);
    } else {
      PulseAudioCore::contextChange(state);
    }
  }

  void PulseAudioCleaner::ready()
  {
    if(--m_counter <= 0) {
      m_running = false;
      pa_threaded_mainloop_signal(m_mainloop, 0);
    }
  }

  void PulseAudioCleaner::moduleInfo(const pa_module_info * i, int eol)
  {
    if(eol) {
      ready();
    } else if(strcmp(i->name, "module-null-sink") == 0) {
      int pid = 0;
      if(i->argument && sscanf(i->argument, "sink_name=\"Cornerstone.%d\"", &pid) == 1) {
        struct stat buf;
        char path[64];
        sprintf(path, "/proc/%d", pid);
        if(stat(path, &buf) == -1 && errno == ENOENT) {
          Radiant::info("PulseAudioCleaner: Unloading old module %s", i->argument);
          ++m_counter;
          pa_operation * op = pa_context_unload_module(m_context, i->index,
                                                       s_unloadSuccessCb, this);
          pa_operation_unref(op);
        }
      }
    }
  }
}

#endif
