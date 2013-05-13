/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#if !defined (RADIANT_MEMORY_HPP)
#define RADIANT_MEMORY_HPP

#include <Radiant/Platform.hpp>
#include <memory>

#if defined (RADIANT_UNIX)
#include <stdlib.h>
#elif defined (RADIANT_WINDOWS)
#include <malloc.h>
#endif

namespace Radiant
{
// Some memory utility functions
  /// Returns a memory aligned block of memory
  /// @param size Amount of bytes to allocate
  /// @param alignment Alignment boundary size (Must be power of 2)
#if defined (RADIANT_WINDOWS)
   inline void * alignedMalloc(size_t size, unsigned int alignment)
   {
     void * ptr = _aligned_malloc(size, alignment);
     if (ptr == 0)
       throw std::bad_alloc();
     return ptr;
   }

   /// Free pointer that is allocated with @ref alignedMalloc
   /// @param ptr Pointer that is being freed
   inline void alignedFree(void * ptr) { _aligned_free(ptr); }

#elif defined (RADIANT_UNIX)
   inline void * alignedMalloc(size_t size, unsigned int alignment)
   {
      void *ptr = 0;
      int r = posix_memalign(&ptr, alignment, size);
      if (r != 0)
            throw std::bad_alloc();
      return ptr;
   }

   /// Free pointer that is allocated with @ref alignedMalloc
   /// @param ptr Pointer that is being freed
   inline void alignedFree(void * ptr) { ::free(ptr); }
#endif

   /// Returns the address of the reference
   /// @param rhs Object whose address is queried
   /// @tparam T Type of the object that is being handled
   template<typename T>
   inline T * addressOf(T & rhs) { return std::addressof(rhs); }
}

#endif // RADIANT_MEMORY_HPP
