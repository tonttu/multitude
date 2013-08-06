/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_MEMCHECK_HPP
#define RADIANT_MEMCHECK_HPP

#include "Radiant/Singleton.hpp"
#include "Radiant/CallStack.hpp"

#include <memory>

#include <map>

namespace Radiant {
  /** Class for memory-checked objects.

      This class is useful for testing if there are objects that are not deallocated in due order.

      <pre>
      make clean;
      qmake MEMCHECK=yes -r && make -j5
      </pre>

      When you quit the application it will print out information on any non-deleted pointers.

      MultiWidgets::Application creates the MemChecker at construction and deletes it at the very end of the
      destructor. Anything that's allocated in between that will use the memchecked allocators

      When using the memory checking option, the application will run markedly slower than with standard compilation.
      If Cornerstone is compiled without the memory checking flags (default), then this class does nothing,
      and it won't impact application performance in any way.
  */

  class RADIANT_API MemChecker
  {
    DECLARE_SINGLETON(MemChecker);

/// @cond

  public:

    MemChecker();
    ~MemChecker();

    void * malloc(size_t s);
    void free(void * ptr);
    inline size_t allocated() const { return m_allocated; }

  private:
    struct Allocation
    {
      Allocation(size_t s) : bytes(s) {}
      Allocation() {}

      CallStack stack;
      size_t bytes;
    };

    typedef std::map<void *, Allocation> AllocationMap;
    AllocationMap m_allocations;
    Radiant::Mutex m_mutex;
    size_t m_allocated;

  };

#if MULTI_MEMCHECK
  inline void * mtmalloc(size_t s) { return MemChecker::instance()->malloc(s); }
  inline void mtfree(void * ptr) { MemChecker::instance()->free(ptr); }

// For overloading new/delete operators
#define MEMCHECKED \
  public: \
    static inline void *operator new(size_t s) { return Radiant::mtmalloc(s); } \
    static inline void *operator new[](size_t s) { return Radiant::mtmalloc(s); } \
    static inline void operator delete(void *ptr) { Radiant::mtfree(ptr); } \
    static inline void operator delete[] (void *ptr) { Radiant::mtfree(ptr); }

#define MEMCHECKED_USING(parent) \
  public: \
    using parent::operator new; \
    using parent::operator new[]; \
    using parent::operator delete; \
    using parent::operator delete[];

#else
  inline void * mtmalloc(size_t s) { return ::malloc(s); }
  inline void mtfree(void * ptr) { ::free(ptr); }

#define MEMCHECKED
#define MEMCHECKED_USING(parent)

#endif

/// @endcond

}

#endif // RADIANT_MEMCHECK_HPP
