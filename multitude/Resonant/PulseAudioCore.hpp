/* COPYRIGHT
 *
 * This file is part of Resonant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RESONANT_PULSEAUDIOCORE_HPP
#define RESONANT_PULSEAUDIOCORE_HPP

#include "Export.hpp"

#include <Radiant/Thread.hpp>
#include <pulse/pulseaudio.h>

/// @cond

namespace Resonant
{
  RESONANT_API void g_contextStateCb(pa_context * c, void * self);
  RESONANT_API void g_eventCb(pa_context * c, pa_subscription_event_type_t t, uint32_t idx, void * self);
  RESONANT_API void g_contextSuccessCb(pa_context *, int success, void * self);
  RESONANT_API void g_clientInfoCb(pa_context * c, const pa_client_info * i, int eol, void * self);
  RESONANT_API void g_moduleCb(pa_context * c, uint32_t idx, void * self);
  RESONANT_API void g_sinkInputInfoCb(pa_context * c, const pa_sink_input_info * i, int eol, void * self);
  RESONANT_API void g_sinkInfoCb(pa_context * c, const pa_sink_info * i, int eol, void * self);

  class PulseAudioCore : public Radiant::Thread
  {
  public:
    RESONANT_API PulseAudioCore();
    RESONANT_API virtual ~PulseAudioCore();

    RESONANT_API virtual void contextSuccess(int success);
    RESONANT_API virtual void contextChange(pa_context_state_t state);

    RESONANT_API virtual void clientEvent(pa_subscription_event_type_t t, uint32_t idx);
    RESONANT_API virtual void sinkInputEvent(pa_subscription_event_type_t t, uint32_t idx);
    RESONANT_API virtual void sinkEvent(pa_subscription_event_type_t t, uint32_t idx);

    RESONANT_API virtual void clientInfo(const pa_client_info * i, int eol);
    RESONANT_API virtual void sinkInputInfo(const pa_sink_input_info * i, int eol);
    RESONANT_API virtual void sinkInfo(const pa_sink_info * i, int eol);

    RESONANT_API virtual void moduleLoaded(uint32_t idx);

    pa_context * context() { return m_context; }

  protected:
    RESONANT_API void childLoop();
    RESONANT_API void runClient();
    RESONANT_API virtual void beforeShutdown() {}
    RESONANT_API void restart();

  protected:
    pa_context * m_context;
    pa_threaded_mainloop * m_mainloop;
    pa_mainloop_api * m_mainloopApi;

    bool m_running, m_restart, m_retry;
  };

  /// Removes all null modules that belong to dead processes
  class PulseAudioCleaner : public PulseAudioCore
  {
  public:
    RESONANT_API PulseAudioCleaner();
    RESONANT_API virtual ~PulseAudioCleaner();

    RESONANT_API static void clean(bool force = false);

    RESONANT_API void contextChange(pa_context_state_t state);
    RESONANT_API void moduleInfo(const pa_module_info * i, int eol);
    RESONANT_API void ready();

  protected:
    int m_counter;
  };
}

/// @endcond

#endif // RESONANT_PULSEAUDIOCORE_HPP
