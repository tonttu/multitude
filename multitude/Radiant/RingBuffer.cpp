/* COPYRIGHT
 *
 * This file is part of Nimble.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Nimble.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#include "RingBuffer.hpp"
#include "RingBufferImpl.hpp"

#include <Nimble/Vector2.hpp>

namespace Nimble {

  float lag3IntCoeffs[LAG3_INTCOEFFS * 4];

  /// Initializes the lag3IntCoeffs structure.
  bool lag3IntCoeffsCalculate()
  {
    for(unsigned i=0; i < LAG3_INTCOEFFS; i++) {
      double delay = (double) i / (double) LAG3_INTCOEFFS;

      /*const ulong ndelay = (ulong) delay;
    const double d = delay - (double) ndelay;*/
      const double d = delay + 1.0;
      const double dm1 = d-1;
      const double dm2 = d-2;
      const double dm3 = d-3;
      const double dm12p6 = dm1 * dm2 * (1.0 / 6.0);
      const double dm03p2 = d * dm3 * 0.5;

      float * target = lag3IntCoeffs + i * 4;

      *target++ = (float) (-dm12p6 * dm3);
      *target++ = (float) ( dm03p2 * dm2);
      *target++ = (float) (-dm03p2 * dm1);
      *target   = (float) ( dm12p6 * d);
    }
    return true;
  }

}

template class Radiant::RingBuffer<float>;
template class Radiant::RingBuffer<int>;
template class Radiant::RingBuffer<Nimble::Vector2>;
