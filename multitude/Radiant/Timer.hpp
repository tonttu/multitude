/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_TIMER_HPP
#define RADIANT_TIMER_HPP

#include "Export.hpp"

#if defined(RADIANT_WINDOWS)
#  include <Radiant/TimerW32.hpp>
#else
#  include <Radiant/TimerPosix.hpp>
#endif

#endif // RADIANT_TIMER_HPP
