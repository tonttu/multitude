#ifndef RESONANT_AUDIOLOOPPULSEAUDIO_HPP
#define RESONANT_AUDIOLOOPPULSEAUDIO_HPP

#include "AudioLoop.hpp"
#include "PulseAudioContext.hpp"

namespace Resonant
{
  class DSPNetwork;
  class ModuleOutCollect;
  class AudioLoopPulseAudio : public AudioLoop
  {
  public:
    AudioLoopPulseAudio(DSPNetwork & dsp, const std::shared_ptr<ModuleOutCollect> & collect);
    virtual ~AudioLoopPulseAudio();

    virtual bool start(int samplerate, int channels) OVERRIDE;
    virtual bool stop() OVERRIDE;
    virtual bool isRunning() const OVERRIDE;
    virtual std::size_t outChannels() const OVERRIDE;

  private:
    class D;
    std::shared_ptr<D> m_d;
  };
} // namespace Resonant

#endif // RESONANT_AUDIOLOOPPULSEAUDIO_HPP
