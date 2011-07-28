#if !defined (RADIANT_CALLSTACK_HPP)
#define RADIANT_CALLSTACK_HPP

#include "Platform.hpp"

#include <iostream>
#include <cassert>
#include <stdint.h>

#ifdef RADIANT_WINDOWS
# define WIN32_MEAN_AND_LEAN
# define NOMINMAX
  typedef uint64_t stackptr_t;
#else
  typedef void * stackptr_t;
#endif

namespace Radiant
{
  /// Captures the current callstack
  class CallStack
  {
  public:

    CallStack();
    ~CallStack();

    /// Returns the raw callstack
    const stackptr_t * stack() const { return m_frames; }

    /// Returns the index-th element in the callstack
    stackptr_t operator[](size_t index) const { assert(index < m_frameCount); return m_frames[index]; }

    /// Returns the number of frames in the callstack
    size_t size() const { return m_frameCount; }

    /// Prints a human-readable version of the stack to the log
    void print() const;

  private:
    enum { max_frames = 32 };
    stackptr_t m_frames[max_frames];
    size_t m_frameCount;
  };
}

#endif
