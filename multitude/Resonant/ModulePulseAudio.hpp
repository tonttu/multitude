#ifndef RESONANT_MODULEPULSEAUDIO_HPP
#define RESONANT_MODULEPULSEAUDIO_HPP

#include "Module.hpp"
#include "PulseAudioCore.hpp"

namespace Resonant {
  class RESONANT_API ModulePulseAudio : public Module, public PulseAudioCore
  {
  public:
    ModulePulseAudio(uint32_t sinkInput);

    void contextChange(pa_context_state_t state);
    void dataAvailable(pa_stream * p, size_t nbytes);

    bool prepare(int & channelsIn, int & channelsOut);
    void process(float ** in, float ** out, int n);

  protected:
    bool m_ready;
    pa_stream * m_stream;
    uint32_t m_sinkInput;
  };
}

#endif // RESONANT_MODULEPULSEAUDIO_HPP
