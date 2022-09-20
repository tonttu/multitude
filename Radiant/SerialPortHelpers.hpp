/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef SERIALPORTHELPERS_HPP
#define SERIALPORTHELPERS_HPP

#include "Trace.hpp"

namespace Radiant {
  template<class T>
  void safeset(T *outp, const T &value)
  {
    if(outp != nullptr) {
      *outp = value;
    }
  }

  inline void printBuffer(const char *buffer, int len, const char *op, const char *traceName)
  {
    const char *begin = buffer;
    const char *end = buffer + len;
    while(begin < end) {
      const char* zeroPtr = (const char*)memchr(begin, 0, end - begin);
      if(zeroPtr == nullptr) {
        zeroPtr = end;
      }
      // print chars before null
      int lenToPrint = zeroPtr - begin;
      if(lenToPrint > 0) {
        Radiant::info("%s%s: %.*s", traceName, op, lenToPrint, begin);
      }
      begin = begin + lenToPrint;
      // now consume all nulls
      while(*begin == '\0' && begin < end) {
        // todo - get rid of newlines
        Radiant::info("%s%s: \\0", traceName, op);
        begin++;
      }
    }
  }
}

#endif // SERIALPORTHELPERS_HPP
