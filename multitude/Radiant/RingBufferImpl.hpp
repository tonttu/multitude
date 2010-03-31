/* COPYRIGHT
 *
 * This file is part of Nimble.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Nimble.hpp" for authors and more details.
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

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <class TElem>
  TElem RingBuffer2<TElem>::getMax(unsigned nTime) const
  {
    TElem xRes = this->getNewestConst();
    for(unsigned nI=1; nI < nTime; nI++) {
      const TElem *xpTmp = &this->getNewestConst(nI);
      if(*xpTmp > xRes)
	xRes = *xpTmp;
    }
    return xRes;
  }

  template <class TElem>
  TElem RingBuffer2<TElem>::getMin(unsigned nTime) const
  {
    TElem xRes = this->getNewestConst();
    for(unsigned nI=1; nI < nTime; nI++) {
      const TElem *xpTmp = &this->getNewestConst(nI);
      if(*xpTmp < xRes)
	xRes = *xpTmp;
    }
    return xRes;
  }

  /** Calculates the autocorrelation of the signal. 

      @arg deltaTime The time difference of the correlation function.

      @arg countSamples The number of samples to be for autocorrelation
      calculus.
  */

  template <class TElem>
  TElem RingBuffer2<TElem>::autoCorrelation
  (unsigned deltaTime, unsigned countSamples) const
  {
    TElem correlation = 0;

    for(unsigned i=0; i < countSamples; i++)
      correlation += this->getNewestConst(i) * this->getNewestConst(i + deltaTime);

    return correlation;
  }

  template <class TElem>
  TElem RingBuffer2<TElem>::autoCorrelation2(unsigned deltaTime, 
					     unsigned countSamples, 
					     unsigned skipSamples) const
  {
    TElem correlation = 0;

    for(unsigned i=0; i < countSamples; i += skipSamples)
      correlation += this->getNewestConst(i) * this->getNewestConst(i + deltaTime);

    return correlation;
  }

}

#endif
