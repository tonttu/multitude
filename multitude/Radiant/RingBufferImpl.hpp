/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef NIMBLE_RINGBUFFER_IMPL_HPP
#define NIMBLE_RINGBUFFER_IMPL_HPP

#include <Radiant/RingBuffer.hpp>

#include <string.h>

namespace Radiant {

  template <class TElem>
  bool RingBuffer<TElem>::resize(unsigned nBufSize)
  {
    unsigned j = 1;
    while (j < nBufSize)
      j = j << 1;
  
    if(j == m_size) return true;

    if(m_line) delete []m_line;

    if(!nBufSize) {
      m_line = 0;
      m_size = 0;
      m_mask = 0;
      return true;
    }

    m_size = j;
    m_mask = m_size - 1;
    m_line = new TElem[m_size];
    if(!m_line) {
      m_size = 0;
      m_mask = 0;
      return false;
    }
    return true;
  }

  template <class TElem>
  void RingBuffer<TElem>::setAll(TElem xVal) 
  { 
    TElem *xp1=m_line, *xp2=&m_line[m_size];

    if(m_size < 4)
      while(xp1 < xp2) *xp1++ = xVal; 
    else {
      // Manual loop unrolling. 
      // At any rate the memory will bottleneck us...
      for(;xp1 < xp2; xp1 += 4) {
	*xp1   = xVal;
	xp1[1] = xVal;
	xp1[2] = xVal;
	xp1[3] = xVal;
      }
    }
  }

  template <class TElem>
  RingBuffer<TElem> &
  RingBuffer<TElem>::operator = (const RingBuffer &xrBuffer)
  {
    if(resize(xrBuffer.m_size))
      for(unsigned i = 0; i < m_size; i++)
	m_line[i] = xrBuffer.m_line[i];
    // memcpy(m_line, xrBuffer.m_line, m_size * sizeof(TElem));
    return *this;
  }

}

#endif
