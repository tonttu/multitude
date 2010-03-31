/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "Task.hpp"

#include "BGThread.hpp"

#include <typeinfo>

#include <Radiant/Trace.hpp>


namespace Luminous
{

  Task::Task(Priority p)
    : m_state(WAITING),
    m_priority(p),
//    m_canDelete(false),
      m_scheduled(0),
      m_host(0)
  {}

  Task::~Task()
  {}

  Radiant::Mutex * Task::generalMutex()
  {
    if(m_host)
      return m_host->generalMutex();

    return 0;
  }

  void Task::initialize()
  {}

  void Task::finished()
  {
    // Radiant::trace("Task::finished # %s", typeid(*this).name());
  }

}
