/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef LUMINOUS_SCREENDETECTORQT_H
#define LUMINOUS_SCREENDETECTORQT_H

#include "ScreenDetector.hpp"

/// @cond

namespace Luminous
{
  namespace ScreenDetectorQt
  {
    void detect(QList<ScreenInfo> & result);
  }
} // namespace Luminous

/// @endcond

#endif // LUMINOUS_SCREENDETECTORQT_H
