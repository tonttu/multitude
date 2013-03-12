/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Luminous/DepthMode.hpp"

namespace Luminous
{
  DepthMode::DepthMode()
    : m_function(LESS_EQUAL)
    , m_range(0.f, 1.f)
  {
  }
}
