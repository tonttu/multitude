#ifndef RESONANT_MODULEPULSEAUDIO_HPP
#define RESONANT_MODULEPULSEAUDIO_HPP

#include "Module.hpp"
#include "PulseAudioCore.hpp"

namespace Resonant {
  class RESONANT_API ModulePulseAudio : public Module, public PulseAudioCore
  {
  public:
    ModulePulseAudio(const std::string & monitorName, uint32_t sinkInput);

    void contextChange(pa_context_state_t state);
    void dataAvailable(pa_stream * p, size_t nbytes);
    void streamState(pa_stream_state_t state);

    bool prepare(int & channelsIn, int & channelsOut);
    void process(float ** in, float ** out, int n);

  protected:
    bool m_ready;
    pa_stream * m_stream;
    std::string m_monitorName;
    uint32_t m_sinkInput;
    float * m_buffer; // pointer to m_buffer
    std::vector<float> m_bufferData; // actual storage
    size_t m_bufferSize; // in samples

    void openStream();
  };
}

#endif // RESONANT_MODULEPULSEAUDIO_HPP
