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


  class GarbageData
  {
  public:
    GarbageData();
    ~GarbageData();


    // items are double-buffered to ensure free access to previous container
    GarbageCollector::container m_items1;
    GarbageCollector::container m_items2;
    GarbageCollector::container * m_current;

    Radiant::Mutex m_mutex;
  };

  static GarbageData * s_check = 0;
  static GarbageData s_gbData;

  GarbageData::GarbageData()
    : m_current( & m_items1)
  {
    if(!s_check)
      s_check = this;
  }

  GarbageData::~GarbageData()
  {
    if(s_check == this)
      s_check = 0;
  }


  GarbageCollector::GarbageCollector()
  {}

  GarbageCollector::~GarbageCollector()
  {}

  void GarbageCollector::clear()
  {
    Radiant::Guard g(s_gbData.m_mutex);
    // swap and clear current
    s_gbData.m_current = s_gbData.m_current == & s_gbData.m_items1 ?
                         &s_gbData.m_items2 : &s_gbData.m_items1;
    s_gbData.m_current->clear();
  }

  void GarbageCollector::objectDeleted(Collectable * obj)
  {
    /* This function might be called by static Collectable objects after the
       static GarbageData has been destroyed. This s_check object is used to detect
       these cases and avoid crashing the application. */

    if(!s_check)
      return;

    Radiant::Guard g(s_gbData.m_mutex);
    s_gbData.m_current->insert(obj);
  }

  int GarbageCollector::size()
  {
    return (int) s_gbData.m_current->size();
  }

  const GarbageCollector::container & GarbageCollector::previousObjects()
  {
    return s_gbData.m_current == &s_gbData.m_items1 ?
        s_gbData.m_items2 : s_gbData.m_items1;
  }
}
