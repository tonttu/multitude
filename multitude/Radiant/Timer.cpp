/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include <Radiant/Timer.hpp>

namespace Radiant
{
  Timer::Timer()
  {
    start();
  }

  void Timer::start()
  {
    m_started = TimeStamp::getTime();
  }

  float Timer::elapsed() const 
  {
    TimeStamp now = TimeStamp::getTime();
    TimeStamp t = now - m_started;
  
    return static_cast<float> (t.secondsD());
  }

}

