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

#include "IntrusivePtr.hpp"

#ifdef INTRUSIVE_PTR_DEBUG

#include "Mutex.hpp"

namespace
{
  typedef std::map<const void *, Radiant::IntrusivePtrDebug::CallMap> Map;
  Map s_map;
  Radiant::Mutex s_mapMutex;
}

namespace Radiant
{
  IntrusivePtrDebug::CallMap IntrusivePtrDebug::fetch(const void * ptr)
  {
    Guard g(s_mapMutex);
    Map::const_iterator it = s_map.find(ptr);
    if(it == s_map.end()) return CallMap();
    return it->second;
  }

  void IntrusivePtrDebug::add(const void * ptr, const void * intrusivePtr)
  {
    Guard g(s_mapMutex);
    s_map[ptr][intrusivePtr];
  }

  void IntrusivePtrDebug::remove(const void * ptr, const void * intrusivePtr)
  {
    Guard g(s_mapMutex);
    CallMap & map = s_map[ptr];
    map.erase(intrusivePtr);
    if(map.empty()) s_map.erase(ptr);
  }
}
#endif