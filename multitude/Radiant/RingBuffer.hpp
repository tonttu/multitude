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

#ifndef RADIANT_RINGBUFFER_HPP
#define RADIANT_RINGBUFFER_HPP

#include <Radiant/Export.hpp>

namespace Radiant {

  /** 
      Simple ring buffer template. 

      The operations are often optimized and thus there are few safety 
      checks.

      Buffer size is always 2^n.
    
      The elements of this class need to be numeric:
      float, double, int, long etc.

      @author Tommi Ilmonen, taken from the Mustajuuri project
      (Copyright, Tommi Ilmonen).

  */

  template <class TElem>
  class RADIANT_API RingBuffer
  {
  public:
    /**@name Constructors */

    //@{

    /// Creates an empty ring buffer
    RingBuffer() : m_line(0), m_mask(0), m_size(0) {}

    /// Creates a buffer with given (or larger) size
    RingBuffer(unsigned nSize) 
      : m_line(0), m_size(0) { resize(nSize);}

    /// Copies given buffer making a deep copy
    RingBuffer(const RingBuffer &xrBuffer)
      : m_line(0), m_size(0) { *this = xrBuffer; }

    /// Deletes ring buffer and frees all memory
    virtual ~RingBuffer() { if(m_line) delete []m_line; }

    /** Resize the buffer. Old buffer elements are lost when the buffer is
    resized. If new size equals the old size the buffer is not reallocated. The
    new buffer will be able to hold at least nBufSize elements.

    The new buffer elements are by default initialized to zero.

    @param nBufSize new buffer size
    @return true on success
    @author Tommi Ilmonen*/
    bool resize(unsigned nBufSize);

    /// Get a sample from the buffer.
    inline TElem &getIndex(unsigned nIndex)
    { return m_line[nIndex & m_mask]; }

    inline const TElem &getIndex(unsigned nIndex) const
    { return m_line[nIndex & m_mask]; }

    /// Get a sample from the buffer.
    inline const TElem &getIndexConst(unsigned nIndex) const
    { return m_line[nIndex & m_mask]; }
  
    /// Set a sample of the buffer.
    inline void setIndex(unsigned nIndex, const TElem &xVal)
    { m_line[nIndex & m_mask] = xVal; }


    inline TElem &operator[](unsigned nIndex) { return getIndex(nIndex); }
    inline const TElem &operator[](unsigned nIndex) const 
    { return getIndexConst(nIndex); }
  
    /// Set all buffer values.
    void setAll(TElem xVal);
  
    /// Report memory footprint in bytes.
    inline unsigned sizeOf() const
    { return sizeof(TElem) * m_size + sizeof(*this); }
  
    /// Report buffer size.
    inline unsigned size() const { return m_size; }
  
    /// Report mask used for address translation.
    inline unsigned mask() const { return m_mask; }

    /// Get pointer to data
    inline TElem *data() { return m_line; }

    /// Get const pointer to data
    inline const TElem *data() const { return m_line; }

    /// Return "sizeof(TElem)"
    inline int elemSize() const { return sizeof(TElem); } 

    /// Deep copy operator
    /// @param xrBuffer buffer to copy
    /// @return reference to this
    RingBuffer<TElem> &operator = (const RingBuffer &xrBuffer);

    /** Calculate the real size of the buffer if the required number of samples
    is nBufSize. Outsiders should not need this method too often.
    @param nBufSize target buffer size
    @return required buffer size*/
    static unsigned targetSize(unsigned nBufSize) 
    {
      unsigned j = 1;
      while (j < nBufSize) j = j << 1;
      return j;
    }

  protected:

    TElem   *m_line;   ///< Data buffer
    unsigned    m_mask;   ///< Mask for determining index.
    unsigned    m_size;   ///< Real size
  };


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /** Ring buffer for delays. This sublass if RingBuffer introduces a
      sample counter that is used to calculate delays.

      \code

      RingBufferDelay<float> delay(128); // Create a new delay line

      delay.getNewest() = 0.4;    // Set the current value to 0.4
      delay.advance();            // Advance the line
      float delayedValue = delay.getNewest(1);  // delayedValue = 0.4

      \endcode

      @author Tommi Ilmonen*/

  template <class TElem>
  class RADIANT_API RingBufferDelay : public RingBuffer<TElem>
  {
  public:
    RingBufferDelay() : m_position(0) {}
    /// Constructs a new buffer and initializes it to the given size
    RingBufferDelay(unsigned size) : RingBuffer<TElem>(size), m_position(0) {}
    virtual ~RingBufferDelay() {}

    /// Advance the counter by one.
    inline void advance() { m_position++; }

    /// Advance the counter by many.
    inline void advanceN(unsigned nSteps) { m_position += nSteps; }

    /// Get a sample from the buffer.
    /// @param nDelay delay in samples
    /// @return delayed sample
    inline TElem &getNewest(unsigned nDelay)
    { return this->m_line[(m_position - nDelay) & this->m_mask]; }

    /// @copydoc getNewest
    inline const TElem &getNewest(unsigned nDelay) const
    { return this->m_line[(m_position - nDelay) & this->m_mask]; }

    /// Get a sample from the buffer.
    inline const TElem &getNewestConst(unsigned nDelay) const
    { return this->m_line[(m_position - nDelay) & this->m_mask]; }

    /// Get the newest sample from the buffer.
    inline TElem &getNewest()
    { return this->m_line[m_position & this->m_mask]; }

    /// Get the newest sample from the buffer.
    inline const TElem &getNewestConst() const
    { return this->m_line[m_position & this->m_mask]; }

    /// Advance the delay line and set the "newest" sample.
    inline void put(const TElem &v) { advance(); getNewest() = v; }

    /// Returns the number of samples processed
    inline unsigned getSampleCount() const { return m_position; }

    /// Sets the number of samples processed
    inline void setSampleCount(const unsigned &nCount) 
    { m_position = nCount; }

    /// Report memory footprint in bytes.
    inline unsigned sizeOf() const
    { return sizeof(TElem) * this->m_size + sizeof(*this); }

    /// Reset the buffer.
    inline void reset() { m_position = 0; /* setAll(0); */}

  protected:

    /// The current position in the buffer
    unsigned   m_position;
  };

} // namespace

#endif
