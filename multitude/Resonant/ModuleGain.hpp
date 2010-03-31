/* COPYRIGHT
 *
 * This file is part of Resonant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Resonant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RESONANT_MODULE_GAIN_HPP
#define RESONANT_MODULE_GAIN_HPP

#include <Nimble/Ramp.hpp>

#include <Resonant/Module.hpp>

namespace Resonant {

  /** Gain control audio module. */
  class ModuleGain : public Module
  {
  public:
    ModuleGain(Application *);
    virtual ~ModuleGain();

    virtual bool prepare(int & channelsIn, int & channelsOut);
    virtual void process(float ** in, float ** out, int n);
    
  private:

    int m_channels;

    Nimble::RampT<float> m_gain;
  };

}

#endif
