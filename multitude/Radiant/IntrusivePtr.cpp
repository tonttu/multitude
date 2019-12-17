/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "IntrusivePtr.hpp"

#ifdef INTRUSIVE_PTR_DEBUG

#include "Mutex.hpp"
#include "StringUtils.hpp"
#include "Trace.hpp"

namespace
{
  Radiant::IntrusivePtrDebug::CallMapDB s_db;
  Radiant::Mutex s_dbMutex;
}

namespace Radiant
{
  IntrusivePtrDebug::CallMap IntrusivePtrDebug::fetch(const IntrusivePtrCounter * counter)
  {
    Guard g(s_dbMutex);
    auto it = s_db.find(counter);
    if(it == s_db.end()) return CallMap();
    return it->second;
  }

  IntrusivePtrDebug::CallMapDB IntrusivePtrDebug::db()
  {
    Guard g(s_dbMutex);
    return s_db;
  }

  void IntrusivePtrDebug::add(const Radiant::IntrusivePtrCounter * counter, const void * intrusivePtr,
                              const std::type_info & type)
  {
    if (!counter) return;
    Guard g(s_dbMutex);
    auto & map = s_db[counter];
    // We can't just store type_info pointer, since when we read this data,
    // the dll that provided this type_info can already be unloaded
    if (map.name.isEmpty())
      map.name = StringUtils::demangle(type);
    map.links[intrusivePtr];
  }

  void IntrusivePtrDebug::move(const IntrusivePtrCounter * counter, const void * intrusivePtrFrom,
                               const void * intrusivePtrTo)
  {
    if (!counter) return;
    Guard g(s_dbMutex);
    auto & map = s_db[counter];

    map.links.erase(intrusivePtrFrom);
    map.links[intrusivePtrTo];
  }

  void IntrusivePtrDebug::remove(const IntrusivePtrCounter * counter, const void * intrusivePtr)
  {
    if (!counter) return;
    Guard g(s_dbMutex);
    auto & map = s_db[counter];
    map.links.erase(intrusivePtr);
    if (map.links.empty()) s_db.erase(counter);
  }
}

#endif
