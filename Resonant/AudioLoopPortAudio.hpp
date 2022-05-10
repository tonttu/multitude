#ifndef AUDIOLOOPPORTAUDIO_HPP
#define AUDIOLOOPPORTAUDIO_HPP

#include "AudioLoop.hpp"

#include <memory>

namespace Resonant
{
  class DSPNetwork;
  class ModuleOutCollect;

  /// PortAudio backend for DSPNetwork
  class RESONANT_API AudioLoopPortAudio : public AudioLoop
  {
  public:
    /// Creates a new audio IO object
    AudioLoopPortAudio(DSPNetwork & dsp, const std::shared_ptr<ModuleOutCollect> & collect);
    /// Deletes an audio IO object
    virtual ~AudioLoopPortAudio();

    /// Start the AudioLoop.
    /// In practice this spans a new thread that is managed by the PortAudio (or rather,
    /// the operating system audio engine).
    /// @param samplerate Desired samplerate, 44100 is safe choice
    /// @param channels Number of channels to open
    /// @return False on error
    virtual bool start(int samplerate, int channels) OVERRIDE;

    /// Stop the audio processing
    virtual bool stop() OVERRIDE;

    /// Check if the audio IO is operational
    virtual bool isRunning() const OVERRIDE;

    /// Returns the number of output channels in the current setup.
    /// This number reflects the numer of channels that the current sound
    /// card has. It is quite typical for sound cards to advertise more channels
    /// they actually have. This may be caused by a sound-card manufacturer
    /// using the same chips in two sounds cards, with a one card having 10
    /// DACs, while the other might have only 4 (case with M-Audio delta 44 vs 1010).
    /// @return Number of channels
    virtual std::size_t outChannels() const OVERRIDE;

    /// Set the global resonand devices configuration XML file, look for example
    /// resonant-devices.xml for more information
    /// @param xmlFilename Filename to the configuration that will be used with
    ///                    all new AudioLoops
    static void setDevicesFile(const QString & xmlFilename);

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}

#endif // AUDIOLOOPPORTAUDIO_HPP
