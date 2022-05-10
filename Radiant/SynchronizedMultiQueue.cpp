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

