/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "MemCheck.hpp"
#include "Trace.hpp"

namespace Radiant {
  MemChecker::MemChecker()
    : m_allocated(0)
  {
  }

  MemChecker::~MemChecker()
  {
    printAllocations();
  }

  void * MemChecker::malloc(size_t s)
  {
    void * ptr = ::malloc(s);

    Radiant::Guard lock(m_mutex);
    m_allocations[ptr] = Allocation(s);
    m_allocated += s;
    return ptr;
  }

  void MemChecker::free(void * ptr)
  {
    // Don't do anything with NULL pointers
    if (ptr == NULL)
      return;

    Radiant::Guard lock(m_mutex);
    // Find the allocation and free it
    AllocationMap::iterator it = m_allocations.find(ptr);
    if (it != m_allocations.end()) {
      m_allocated -= it->second.bytes;

      m_allocations.erase(it);
      ::free(ptr);
    } else {

      // Free anyway to avoid leaks
      ::free(ptr);

      // Shouldn't ever happen, so assert, unless explicitly disabled
      if(m_assertOnFreeErrors) {
        Radiant::error("Tried to free invalid pointer %p! (maybe tried to mtfree a normal malloc?)", ptr);
        assert(false);
      }
    }
  }

  void MemChecker::clearAllocations()
  {
    m_allocations.clear();
    m_allocated = 0;
  }

  void MemChecker::printAllocations()
  {
#if MULTI_MEMCHECK
    Radiant::Guard lock(m_mutex);

    // Display any allocations that are still open
    if (m_allocations.empty()) {
      Radiant::info("Memcheck: No leaked allocations: Great!");
    }
    else
    {
      Radiant::error("Memcheck: Leaked %ld bytes in %ld allocation(s)", allocated(), m_allocations.size());

      AllocationMap::iterator it = m_allocations.begin();
      for (; it != m_allocations.end(); ++it) {
        Radiant::error("Allocated %ld bytes @ %p", it->second.bytes, it->first);
        it->second.stack.print();
      }
    }
#endif
  }
  }

DEFINE_SINGLETON(Radiant::MemChecker);
