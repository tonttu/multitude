/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "SynchronizedMultiQueue.hpp"

#ifdef RADIANT_MSVC
  #include <intrin.h>
#endif
namespace Radiant
{
  namespace BitUtils
  {
    // first bit set counting from LSB
    // if no bits are set, returns -1

    int firstSetBit(uint32_t bits)
    {
  #ifdef RADIANT_MSVC
      unsigned long index;
      if(_BitScanForward(&index, bits) == 0)
        return -1;
      return index;
  #elif defined(RADIANT_CLANG) || defined(RADIANT_GNUC)
      return __builtin_ffs(bits)-1;
  #else
#error "Don't know how to get firstSetBit"
  #endif
    }
  }


} // namespace Radiant

