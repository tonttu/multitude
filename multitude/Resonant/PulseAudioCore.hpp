#ifndef RESONANT_PULSEAUDIOCORE_HPP
#define RESONANT_PULSEAUDIOCORE_HPP

#include <Radiant/Thread.hpp>

#include <pulse/pulseaudio.h>

namespace Resonant
{
  void g_contextStateCb(pa_context * c, void * self);
  void g_eventCb(pa_context * c, pa_subscription_event_type_t t, uint32_t idx, void * self);
  void g_contextSuccessCb(pa_context *, int success, void * self);
  void g_clientInfoCb(pa_context * c, const pa_client_info * i, int eol, void * self);
  void g_moduleCb(pa_context * c, uint32_t idx, void * self);
  void g_sinkInputInfoCb(pa_context * c, const pa_sink_input_info * i, int eol, void * self);
  void g_sinkInfoCb(pa_context * c, const pa_sink_info * i, int eol, void * self);

  class PulseAudioCore : public Radiant::Thread
  {
  public:
    PulseAudioCore();
    virtual ~PulseAudioCore();

    virtual void contextSuccess(int success);
    virtual void contextChange(pa_context_state_t state);

    virtual void clientEvent(pa_subscription_event_type_t t, uint32_t idx);
    virtual void sinkInputEvent(pa_subscription_event_type_t t, uint32_t idx);
    virtual void sinkEvent(pa_subscription_event_type_t t, uint32_t idx);

    virtual void clientInfo(const pa_client_info * i, int eol);
    virtual void sinkInputInfo(const pa_sink_input_info * i, int eol);
    virtual void sinkInfo(const pa_sink_info * i, int eol);

    virtual void moduleLoaded(uint32_t idx);

    pa_context * context() { return m_context; }

  protected:
    void childLoop();
    void runClient();

    pa_context * m_context;
    pa_mainloop_api * m_mainloopApi;

    bool m_running;
  };
}

#endif // RESONANT_PULSEAUDIOCORE_HPP
