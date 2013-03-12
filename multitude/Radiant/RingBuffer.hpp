/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RADIANT_RINGBUFFER_HPP
#define RADIANT_RINGBUFFER_HPP

#include <Radiant/Export.hpp>

namespace Radiant
{

  /**
      Simple ring-buffer template. The operations are optimized and thus there
      are few safety checks.

      Buffer size must always be a power-of-two.

      The elements of this class need to be numeric: float, double, int, long
      etc.

      @author Tommi Ilmonen, taken from the Mustajuuri project
      (Copyright, Tommi Ilmonen).
  */
  template <class TElem>
  class RingBuffer
  {
  public:
    /// Create an empty ring buffer
    RingBuffer() : m_line(0), m_mask(0), m_size(0) {}

    /// Create a ring-buffer with given minimum size. The actual buffer might
    /// be larger than the given value.
    /// @param nSize minimum buffer size
    RingBuffer(unsigned nSize) : m_line(0), m_size(0) { resize(nSize);}

    /// Create a deep copy
    /// @param xrBuffer buffer to copy
    RingBuffer(const RingBuffer &xrBuffer) : m_line(0), m_size(0) { *this = xrBuffer; }

    /// Destroy the ring-buffer and free memory
    virtual ~RingBuffer() { if(m_line) delete []m_line; }

    /// Resize the buffer. Old buffer elements are lost when the buffer is
    /// resized. If new size equals the old size the buffer is not reallocated.
    /// The new buffer will be able to hold at least nBufSize elements. The new
    /// buffer elements are by default initialized to zero.
    /// @param nBufSize new buffer size
    /// @return true on success
    bool resize(unsigned nBufSize)
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

    /// Get a sample from the buffer
    /// @param nIndex index of the sample
    /// @return sample with the given index
    TElem &getIndex(unsigned nIndex)
    { return m_line[nIndex & m_mask]; }

    /// @copydoc getIndex
    const TElem &getIndex(unsigned nIndex) const
    { return m_line[nIndex & m_mask]; }

    /// @copydoc getIndex
    const TElem &getIndexConst(unsigned nIndex) const
    { return m_line[nIndex & m_mask]; }

    /// Set a sample of the buffer.
    /// @param nIndex sample index
    /// @param xVal sample value
    inline void setIndex(unsigned nIndex, const TElem &xVal)
    { m_line[nIndex & m_mask] = xVal; }

    /// Get the sample with the given index
    /// @param nIndex index to get
    /// @return sample with the given index
    inline TElem &operator[](unsigned nIndex) { return getIndex(nIndex); }
    /// @copydoc operator[](unsigned nIndex)
    inline const TElem &operator[](unsigned nIndex) const { return getIndexConst(nIndex); }

    /// Set all buffer values. Fills the buffer with the given value.
    /// @param xVal value to set
    void setAll(TElem xVal)
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

    /// Get buffer size in bytes
    /// @return size of the buffer in bytes
    inline unsigned sizeOf() const
    { return sizeof(TElem) * m_size + sizeof(*this); }

    /// Get buffer size
    /// @return buffer size in number of samples
    inline unsigned size() const { return m_size; }

    /// Report mask used for address translation
    /// @return buffer mask
    inline unsigned mask() const { return m_mask; }

    /// Get a raw pointer to data
    /// @return raw pointer to ring-buffer data
    inline TElem *data() { return m_line; }

    /// @copydoc data
    inline const TElem *data() const { return m_line; }

    /// Get the size of a sample in bytes
    /// @return sample size in bytes
    inline int elemSize() const { return sizeof(TElem); }

    /// Deep copy operator
    /// @param xrBuffer buffer to copy
    /// @return reference to this
    RingBuffer<TElem> &operator = (const RingBuffer &xrBuffer)
    {
      if(resize(xrBuffer.m_size))
        for(unsigned i = 0; i < m_size; i++)
          m_line[i] = xrBuffer.m_line[i];
      // memcpy(m_line, xrBuffer.m_line, m_size * sizeof(TElem));
      return *this;
    }

    /// Calculate the real size of the buffer if the required number of samples
    /// is nBufSize. Outsiders should not need this method too often.
    /// @param nBufSize target buffer size
    /// @return required buffer size
    static unsigned targetSize(unsigned nBufSize)
    {
      unsigned j = 1;
      while (j < nBufSize) j = j << 1;
      return j;
    }

  protected:

    TElem   *m_line;   ///< Data buffer
    unsigned m_mask;   ///< Mask for determining index.
    unsigned m_size;   ///< Real size
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /// Ring buffer for delays. This sublass if RingBuffer introduces a sample
  /// counter that is used to calculate delays.
  /// \code
  /// RingBufferDelay<float> delay(128);      // Create a new delay line
  /// delay.getNewest() = 0.4;                // Set the current value to 0.4
  /// delay.advance();                        // Advance the line
  /// float delayedValue = delay.getNewest(1);// delayedValue = 0.4
  /// \endcode
  template <class TElem>
  class RingBufferDelay : public RingBuffer<TElem>
  {
  public:
    /// Construct an empty buffer
    RingBufferDelay() : m_position(0) {}

    /// Construct a new buffer and initializes it to the given size
    /// @param size buffer size in samples
    RingBufferDelay(unsigned size) : RingBuffer<TElem>(size), m_position(0) {}
    /// Destructor
    virtual ~RingBufferDelay() {}

    /// Advance the counter by one.
    inline void advance() { m_position++; }

    /// Advance the counter by given number of steps
    /// @param nSteps number of steps to advance
    inline void advanceN(unsigned nSteps) { m_position += nSteps; }

    /// Get a sample from the buffer.
    /// @param nDelay delay in samples
    /// @return delayed sample
    inline TElem &getNewest(unsigned nDelay)
    { return this->m_line[(m_position - nDelay) & this->m_mask]; }

    /// @copydoc getNewest
    inline const TElem &getNewest(unsigned nDelay) const
    { return this->m_line[(m_position - nDelay) & this->m_mask]; }

    /// @copydoc getNewest
    inline const TElem &getNewestConst(unsigned nDelay) const
    { return this->m_line[(m_position - nDelay) & this->m_mask]; }

    /// Get the newest sample from the buffer
    inline TElem &getNewest()
    { return this->m_line[m_position & this->m_mask]; }

    /// @copydoc getNewest
    inline const TElem &getNewestConst() const
    { return this->m_line[m_position & this->m_mask]; }

    /// Advance the delay line and set the "newest" sample.
    /// @param v newest sample to set
    inline void put(const TElem &v) { advance(); getNewest() = v; }

    /// Returns the number of samples processed
    /// @return number of samples
    inline unsigned getSampleCount() const { return m_position; }

    /// Sets the number of samples processed
    /// @param nCount number of samples
    inline void setSampleCount(unsigned int nCount)
    { m_position = nCount; }

    /// Get memory usage in bytes
    /// @return number of bytes used by the buffer
    inline unsigned sizeOf() const
    { return sizeof(TElem) * this->m_size + sizeof(*this); }

    /// Reset the buffer
    inline void reset() { m_position = 0; /* setAll(0); */}

  protected:

    /// The current position in the buffer
    unsigned   m_position;
  };

} // namespace

#endif
