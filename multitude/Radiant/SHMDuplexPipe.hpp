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

#ifndef RADIANT_SM_DUPLEX_PIPE_HPP
#define RADIANT_SM_DUPLEX_PIPE_HPP

#include <Radiant/SHMPipe.hpp>

namespace Radiant
{

  /** Ful-duplex shared memory data pipe. This utility class packs two
      #Radiant::SHMPipe objects into one object. */
  /// @todo Document
  class SHMDuplexPipe
  {
  public:
    RADIANT_API SHMDuplexPipe(const key_t writeKey, const uint32_t writeSize,
         const key_t readKey,  const uint32_t readSize);
    RADIANT_API virtual ~SHMDuplexPipe();

    RADIANT_API int read(void * ptr, int n) { return m_in.read(ptr, n); }
    RADIANT_API int read(BinaryData & bd) { return m_in.read(bd); }
    RADIANT_API uint32_t readAvailable() { return m_in.readAvailable(); }

    RADIANT_API int write(const void * ptr, int n) { return m_out.write(ptr, n); }
    inline int write(const BinaryData & bd, bool doflush)
    { int n = m_out.write(bd); if(doflush) flush(); return n ; }
    inline uint32_t writeAvailable() { return m_out.writeAvailable(); }
    /// Flush the written data to the buffer
    inline void flush() { m_out.flush(); }
    /** Zeroes out the input- and output pipes.*/
    inline void zero() { m_out.zero(); m_in.zero(); }
  private:
    SHMPipe m_out;
    SHMPipe m_in;
  };

}

#endif


