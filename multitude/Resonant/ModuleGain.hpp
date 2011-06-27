/* COPYRIGHT
 */

#ifndef RESONANT_MODULE_GAIN_HPP
#define RESONANT_MODULE_GAIN_HPP

#include "Export.hpp"
#include "Module.hpp"

#include <Nimble/Ramp.hpp>

namespace Resonant {

  /** Gain control audio module. The gain is defined by a single gain
      coefficient, which is used for linear multiplication. */
  class RESONANT_API ModuleGain : public Module
  {
  public:
    /// Constructs a new gain controller module
    ModuleGain(Application *);
    virtual ~ModuleGain();

    virtual bool prepare(int & channelsIn, int & channelsOut);
    virtual void process(float ** in, float ** out, int n);

    void setGainInstant(float gain) { m_gain.reset(gain); }

  private:

    int m_channels;

    Nimble::RampT<float> m_gain;
  };

}

#endif
