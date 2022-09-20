/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

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
