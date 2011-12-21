#ifndef RADIANT_TIMER_HPP
#define RADIANT_TIMER_HPP

#include "Export.hpp"

#if defined(RADIANT_WINDOWS)
#  include <Radiant/TimerW32.hpp>
#else
#  include <Radiant/TimerPosix.hpp>
#endif

#endif // RADIANT_TIMER_HPP
