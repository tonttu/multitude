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

#ifndef RADIANT_SM_PIPE_HPP
#define RADIANT_SM_PIPE_HPP

/// @cond

#include "BinaryData.hpp"
#include "Export.hpp"
#include "RefPtr.hpp"

#ifdef WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <WinPort.h>
#else
#	include <sys/ipc.h> // key_t on OSX
#endif

#include <string>

namespace Radiant
{

  /** One-directional shared-memory data pipe. This type of a pipe is
      used to transfer (binary) data between two separate
      processes.

      <B>Hint:</B> Often it is easiest to format the data using one
      #Radiant::BinaryData as the data container, as #Radiant::BinaryData allows easy
      storage of integers, floats, strings etc, without too much
      overhead.

      <B>Internally</B> the pipe works as a ring buffer, that has a read- and
      write position. The producer process writes bytes into the
      buffer, and once the write is complete the write head is
      updated. The writer observes the read head location so that it
      does not overwrite data that has not been read yet.

      @see SHMDuplexPipe
  */
  class SHMPipe
  {
  public:

    /// Constructor.
#ifdef WIN32
    /// @param smName User-defined name for shared memory.
    /// @param size Size in bytes of the ring buffer: if size > 0,
    /// creates a new ring buffer of that size; if size == 0,
    /// references the existing buffer identified by smKey.
    RADIANT_API SHMPipe::SHMPipe(const std::string smName, uint32_t size);
#else
    /// @param smKey User-defined key to shared memory.
    /// @param size Size in bytes of the ring buffer: if size > 0, creates a new ring buffer
    /// of that size; if size == 0, references the existing buffer identified by smKey.
    RADIANT_API SHMPipe(key_t smKey, uint32_t size);
    RADIANT_API SHMPipe(int id);
    RADIANT_API static SHMPipe * create(uint32_t size);
#endif

    /// Reads data from the buffer.
    /** @return This function returns the number of bytes read from the buffer. */
    RADIANT_API int read(void * ptr, int n, bool block = false, bool peek = false);
    RADIANT_API int read(BinaryData &);
    RADIANT_API int peek(void * ptr, int n, bool block = false)
    { return read(ptr, n, block, true); }
    /// The number of bytes available for reading immediately
    RADIANT_API uint32_t readAvailable();
    RADIANT_API uint32_t readAvailable(uint32_t require);

    RADIANT_API void consume(int n);

    /// Stores data into the buffer, without flushing it.
    /** 
	@return the number of bytes actually written. This is either n
	or zero. */
    RADIANT_API int write(const void * ptr, int n);
    RADIANT_API int write(const BinaryData &);
    /// The number of bytes available for writing immediately
    RADIANT_API uint32_t writeAvailable(uint32_t require);
    RADIANT_API uint32_t writeAvailable();
    /// Flush the written data to the buffer
    RADIANT_API void flush();

    /// Returns the size of the shared memory area
    RADIANT_API uint32_t size() const { return m_data.size; }

    /// Clears the transfer counters
    RADIANT_API void clear();

    RADIANT_API int id() const;

    RADIANT_API static void deleteShm(int id);

  private:
    /// Access functions.

    /// Return the read position.
    inline uint32_t readPos() const { return m_data.readPos; }

    /// Return the read position.
    inline uint32_t writePos() const { return m_data.writePos; }
    
    /// Output attributes and properties.
    void dump() const;

#ifdef WIN32
    /// User-defined name for the shared memory area.
    std::string   m_smName;

    /// Handle to shared memory area.
    HANDLE  m_hMapFile;
#endif

    class SHMHolder;
    std::shared_ptr<SHMHolder> m_holder;

    struct Data {
      uint32_t size;
      /// Flushed write position
      uint32_t writePos;
      /// Write position (might be unfinished writing)
      uint32_t written;
      uint32_t readPos;
      int sem;
      uint8_t pipe[];
    } & m_data;
  };

}

/// @endcond

#endif
