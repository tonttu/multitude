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

  GarbageCollector::container GarbageCollector::m_items;

  static Radiant::MutexStatic __garbmutex;

  GarbageCollector::GarbageCollector()
  {}

  GarbageCollector::~GarbageCollector()
  {}

  void GarbageCollector::clear()
  {
    Radiant::GuardStatic g(__garbmutex);
    m_items.clear();
  }

  void GarbageCollector::objectDeleted(Collectable * obj)
  {
    Radiant::GuardStatic g(__garbmutex);
    m_items.insert(obj);
  }

  Radiant::MutexStatic & GarbageCollector::mutex()
  {
    return __garbmutex;
  }
}
