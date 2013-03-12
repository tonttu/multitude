/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RESONANT_MODULE_GAIN_HPP
#define RESONANT_MODULE_GAIN_HPP

#include "Export.hpp"
#include "Module.hpp"

#include <Nimble/Ramp.hpp>

namespace Resonant {

  /** Gain control audio module. The gain is defined by a single gain
      coefficient, which is used for linear multiplication. */
  class RESONANT_API ModuleGain : public Resonant::Module
  {
  public:
    /// Constructs a new gain controller module
    ModuleGain();
    virtual ~ModuleGain();

    virtual bool prepare(int & channelsIn, int & channelsOut);
    virtual void process(float ** in, float ** out, int n, const CallbackTime &);

    /// Set the gain instantly
    /// @param gain New gain coefficient
    void setGainInstant(float gain) { m_gain.reset(gain); }

  private:

    int m_channels;

    Nimble::Rampf m_gain;
  };

}

#endif
