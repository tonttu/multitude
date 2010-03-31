/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef RADIANT_FIXED_STR_HPP
#define RADIANT_FIXED_STR_HPP

#include <Nimble/Matrix3.hpp>
#include <Nimble/Vector4.hpp>

#include <Radiant/Export.hpp>

#include <stdio.h>
#include <cassert>

#include <string.h>

namespace Radiant {

  /** Template class for fixed-capacity strings. */

    /// @todo remove/move into cv
  template <int N>
  class RADIANT_API FixedStrT
  {
  public:
    FixedStrT();
    FixedStrT(float v, int digits = 1);
    FixedStrT(Nimble::Vector2 v, int digits = 1);
    FixedStrT(Nimble::Vector3 v, int digits = 1);
    FixedStrT(Nimble::Vector4 v, int digits = 1);
    FixedStrT(const Nimble::Matrix3 & v, int digits = 1);
    FixedStrT(const char * str);
    ~FixedStrT();

    void writeFloats(const float * ptr, int n, int digits);
   
    void copyn(const char * ptr, int n);

    const char * str() const;

    operator const char * () const;
    operator char * ();

    size_t length();
    /// The capacity (maximum size) of the string.
    static inline int capacity();

    FixedStrT & operator = (const char * str);

  protected:
    char m_buf[N+1];
  };

}

#endif
