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

#ifndef RADIANT_FIXED_STR_IMPL_HPP
#define RADIANT_FIXED_STR_IMPL_HPP

#include "FixedStr.hpp"

namespace Radiant
{

  template<int N>
  FixedStrT<N>::FixedStrT()
  { m_buf[0] = '\0'; }

  template<int N>
  FixedStrT<N>::FixedStrT(float v, int digits)
  {
    writeFloats(&v, 1, digits);
  }
 
  template<int N>
  FixedStrT<N>::FixedStrT(Nimble::Vector2 v, int digits)
  { writeFloats(v.data(), 2, digits); }
 
  template<int N>
  FixedStrT<N>::FixedStrT(Nimble::Vector3 v, int digits)
  { writeFloats(v.data(), 3, digits); }

  template<int N> 
  FixedStrT<N>::FixedStrT(Nimble::Vector4 v, int digits)
  { writeFloats(v.data(), 4, digits); }

  template<int N>
  FixedStrT<N>::FixedStrT(const Nimble::Matrix3 & v, int digits)
  { writeFloats(v.data(), 9, digits); }

  template<int N>
  FixedStrT<N>::FixedStrT(const char * str)
  { 
    if(str) {
      assert(strlen(str) < N);
      strcpy(m_buf, str);
    } else
      m_buf[0] = '\0';
  }

  template<int N>
  FixedStrT<N>::~FixedStrT()
  {}

  template<int N>
  void FixedStrT<N>::writeFloats(const float * ptr, int n, int digits)
  {
    char tmp[6];

    sprintf(tmp, "%%.%df ", digits);

    char *tmpptr = m_buf;

    for(int i = 0; i < n; i++) {
      tmpptr += sprintf(tmpptr, tmp, ptr[i]);
    }
  }

  template<int N>
  inline void FixedStrT<N>::copyn(const char * ptr, int n)
  {
    if(n+1 >= N)
      return;

    memcpy(m_buf, ptr, n);
    m_buf[n] = 0;
  }

  template<int N>  
  const char * FixedStrT<N>::str() const 
  { return m_buf; }

  template<int N>
  FixedStrT<N>::operator const char * () const 
  { return m_buf; }

  template<int N>
  FixedStrT<N>::operator char * () 
  { return m_buf; }

  template<int N>
  size_t FixedStrT<N>::length() 
  { return strlen(m_buf); }

  template<int N>
  inline int FixedStrT<N>::capacity() 
  { return N; }

  template<int N>
  FixedStrT<N> & FixedStrT<N>::operator = (const char * str)
  { 
    if(str) {
      assert(strlen(str) < N);
      strcpy(m_buf, str);
    }
    else
      m_buf[0] = '\0';

    return * this;
  }

}

#endif
