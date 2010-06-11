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

#include "GarbageCollector.hpp"

namespace Luminous
{

  GarbageCollector::container GarbageCollector::m_items1;
  GarbageCollector::container GarbageCollector::m_items2;
  GarbageCollector::container * GarbageCollector::m_current = &GarbageCollector::m_items1;

  static Radiant::MutexStatic __garbmutex;

  GarbageCollector::GarbageCollector()
  {}

  GarbageCollector::~GarbageCollector()
  {}

  void GarbageCollector::clear()
  {
    Radiant::GuardStatic g(__garbmutex);
    // swap and clear current
    m_current = m_current == &m_items1 ? &m_items2 : &m_items1;
    m_current->clear();
  }

  void GarbageCollector::objectDeleted(Collectable * obj)
  {
    Radiant::GuardStatic g(__garbmutex);
    m_current->insert(obj);
  }

  const GarbageCollector::container & GarbageCollector::previousObjects()
  {
    return m_current == &m_items1 ? m_items2 : m_items1;
  }
}
