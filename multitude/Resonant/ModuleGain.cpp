/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ModuleGain.hpp"

namespace Resonant {

  ModuleGain::ModuleGain()
    : m_channels(1)
  {
    m_gain.reset(1.0f);
  }

  ModuleGain::~ModuleGain()
  {}

  bool ModuleGain::prepare(int & channelsIn, int & channelsOut)
  {
    if(channelsIn != channelsOut || channelsIn <= 0)
      return false;

    m_gain.toTarget();

    m_channels = channelsIn;

    return true;
  }

  void ModuleGain::process(float ** in, float ** out, int n, const CallbackTime &)
  {
    for(int i = 0; i < m_channels; i++) {

      Nimble::Rampf g = m_gain;

      const float * source = in[i];
      float * target = out[i];
      float * sentinel = target + n;

      if(g.left()) {

        while(target < sentinel) {
          *target = *source * g.value();
          g.update();
          target++;
          source++;
        }

        if(i + 1 == m_channels)
          m_gain = g;
      }
      else {

        float gn = g.value();

        while(target < sentinel) {
          *target = *source * gn;
          target++;
          source++;
        }
      }
    }

  }

}
