#ifndef RESONANT_MODULE_BUFFER_PLAYER_HPP
#define RESONANT_MODULE_BUFFER_PLAYER_HPP

#include "Module.hpp"

#include <Radiant/BlockRingBuffer.hpp>

namespace Resonant
{
  /// Provides an audio buffer that will be played on DSPNetwork. User of this class
  /// needs to fill the buffer periodically.
  class RESONANT_API ModuleBufferPlayer : public Resonant::Module
  {
  public:
    /// Constructs an inactive module
    /// @param name prefix for the Resonant::Module id
    ModuleBufferPlayer(const QString & name);
    virtual ~ModuleBufferPlayer();

    void setChannelCount(int channelCount);
    int channelCount() const;

    /// Buffers that need to be filled periodically, if the module is added to
    /// DSPNetwork. One buffer per channel. Always fill all buffers the same
    /// amount.
    std::vector<Radiant::BlockRingBuffer<float>> & buffers();

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
  typedef std::shared_ptr<ModuleBufferPlayer> ModuleBufferPlayerPtr;

} // namespace Resonant

#endif // RESONANT_MODULE_BUFFER_PLAYER_HPP
