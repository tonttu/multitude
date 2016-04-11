#ifndef RESONANT_MODULEINPUTPLAYER_HPP
#define RESONANT_MODULEINPUTPLAYER_HPP

#include "Module.hpp"

namespace Resonant
{
  /// Forwards input source (microphone, line-input or other capture source)
  /// to the DSPNetwork. Uses portaudio to read the input, so it will most
  /// likely spawn a new thread.
  class RESONANT_API ModuleInputPlayer : public Resonant::Module
  {
  public:
    enum class OpenResult
    {
      SUCCESS,               ///< Device was opened successfully
      PA_INIT_ERROR,         ///< Pa_Initialize failed
      PA_DEVICE_NOT_FOUND,   ///< Failed to find PortAudio device with the given name
      NO_INPUT_CHANNELS,     ///< There are no input channels on the device
      PA_OPEN_ERROR,         ///< Failed to open PA stream
      PA_START_ERROR         ///< Failed to start PA stream
    };

  public:
    /// Constructs an inactive module
    ModuleInputPlayer();
    /// Calls close if the player wasn't closed already
    virtual ~ModuleInputPlayer();

    /// Synchronously opens an input source
    /// @param deviceName full name matching the PortAudio device name (use
    ///                   ListPortAudioDevices to list them all), or just ALSA
    ///                   name like "hw:2,0" in the same format how PortAudio
    ///                   prints it.
    /// @param errorMessage[out] Error message
    OpenResult open(const QString & deviceName, QString * errorMessage);

    /// Synchronously closes the input source
    void close();

    float gain() const;
    void setGain(float gain);

    /// Target maximum latency in seconds, the lower the latency, the more
    /// expected buffer underruns.
    float maxLatency() const;
    void setMaxLatency(float secs);

    virtual bool prepare(int & channelsIn, int & channelsOut) OVERRIDE;
    virtual void process(float ** in, float ** out, int n, const Resonant::CallbackTime & time) OVERRIDE;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
  typedef std::shared_ptr<ModuleInputPlayer> ModuleInputPlayerPtr;

} // namespace Resonant

#endif // RESONANT_MODULEINPUTPLAYER_HPP
