
#ifndef MULTITUDE_STRINGS_H
#define MULTITUDE_STRINGS_H

#include <string.h>

inline void bzero(void * ptr, size_t n)
{
  memset(ptr, 0, n);
}

inline long long atoll(const char * str)
{
  return _atoi64(str);
}

#endif
