#ifndef RADIANT_MEMCHECK_HPP
#define RADIANT_MEMCHECK_HPP

namespace Radiant {
  /** Base class for memory-checked objects.

      This class is useful for testing if there are objects that are not deallocated in due order.
      To use this class you simply inherit from the MemCheck class, and recompile Cornerstone with:

      <pre>
      make clean;
      qmake MEMCHECK=yes -r && make -j5
      </pre>

      This class works correctly only on Linux.

      When you quit the application it will print out information on any non-deleted pointers.

      When using the memory checking option, the application will run markedly slower than with standard compilation.
      If Cornerstone is compiled without the memory checking flags (default), then this class does nothing,
      and it won't impact application performance in any way.
  */
  class MemCheck
  {
  public:
#ifdef MULTI_MEMCHECK
    MemCheck();
    MemCheck(const MemCheck & s);
    virtual ~MemCheck();
    MemCheck & operator=(const MemCheck & s);
#endif
  };
}

#endif // RADIANT_MEMCHECK_HPP
