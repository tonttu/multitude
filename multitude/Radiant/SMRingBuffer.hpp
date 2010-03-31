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

#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include <Radiant/BinaryData.hpp>
#include <Radiant/Export.hpp>

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

  /**
   * @class SMRingBuffer
   *
   * This class implements a ring buffer in shared memory to
   * facilitate the passing of binary data between processes.
   *
   * Notes
   *
   * 1. The write position is never allowed to catch up with the read
   *    position. This makes it possible to distinguish an empty
   *    buffer (read pos == write pos) from a full buffer (read pos
   *    == write pos + 1), at the cost of a redundant byte.
   *
   * Usage
   *
   * 1. Decide a unique key for the shared memory area (SMA). It can
   *    be any positive integer, e.g. 1234. Make the key available to
   *    all processes by defining it in a common header file.  
   *
   * 2. To create the ring buffer, construct a SMRingBuffer object
   *    with the unique key and required size as parameters. The
   *    constructor handles allocation of the shared memory.  
   *
   * 3. To refer to the ring buffer from the same or other process,
   *    construct other SMRingBuffer object(s) using the same key.
   *    
   * 4. Call write() to add data to the ring buffer, and read() to
   *    remove it. 
   *
   * 5. Processes themselves are responsible for formatting data to
   *    be written, e.g.  prepending size or type information.
   *    
   * 6. The SMRingBuffer destructor handles deallocation of the
   *    shared memory.
   */
  class RADIANT_API SMRingBuffer
  {


  public:

    /// Class types.

    /// The read/write state tells whether the buffer is currently being read from or written
    /// to and is used to prevent simultanous reads or simultaneous writes by sharing processes.
    enum ReadWriteState
    {
      RWS_NONE    = 0x00,     ///< neither reading nor writing
      RWS_READING = 0x01,     ///< currently reading
      RWS_WRITING = 0x02,     ///< currently writing
    };


    /// Static data.

    /// Default permissions for shared memory.
    static uint32_t   smDefaultPermissions;

    /// Size of header information preceding ring buffer in shared
    /// memory.  The header contains 4 (x 32-bit integer) items that
    /// must be available to all sharing processes: buffer size,
    /// write position, read position and read/write state, in that
    /// order.
    static uint32_t   smHeaderSize;

    /// Maximum size in bytes of buffer.
    static uint32_t   maxSize;


    /// Construction / destruction.

    /// Constructor.
#ifdef WIN32
    /// @param smName User-defined name for shared memory.
    /// @param size Size in bytes of the ring buffer: if size > 0,
    /// creates a new ring buffer of that size; if size == 0,
    /// references the existing buffer identified by smKey.
    SMRingBuffer::SMRingBuffer(const std::string smName, const uint32_t size);
#else
    /// @param smKey User-defined key to shared memory.
    /// @param size Size in bytes of the ring buffer: if size > 0, creates a new ring buffer
    /// of that size; if size == 0, references the existing buffer identified by smKey.
    SMRingBuffer(const key_t smKey, const uint32_t size);
#endif

    /// Destructor.
    virtual ~SMRingBuffer();


    /// Access functions.

    /// Return a pointer to the start of the ring buffer.
    unsigned char * startPtr() const { return m_startPtr; }

    /// Return the size in bytes of the ring buffer.
    uint32_t size() const
    {
      return * ((uint32_t *)(m_startPtr - smHeaderSize));
    }

    /// Return the write position.
    uint32_t writePos() const
    {
      return * ((uint32_t *)((m_startPtr - smHeaderSize) + sizeof(uint32_t)));
    }

    /// Return the read position.
    uint32_t readPos() const
    {
      return * ((uint32_t *)((m_startPtr - smHeaderSize) + sizeof(uint32_t) * 2));
    }

    /// Return the current read/write state.
    uint32_t readWriteState() const
    {
      return * ((uint32_t *)((m_startPtr - smHeaderSize) + sizeof(uint32_t) * 3));
    }

    /// Set the write position.
    void setWritePos(const uint32_t writePos)
    {
      * ((uint32_t *)((m_startPtr - smHeaderSize) + sizeof(uint32_t))) = writePos;
    }

    /// Set the read position.
    void setReadPos(const uint32_t readPos)
    {
      * ((uint32_t *)((m_startPtr - smHeaderSize) + sizeof(uint32_t) * 2)) = readPos;
    }

    /// Set the read/write state.
    void setReadWriteState(const uint32_t readWriteState)
    {
      * ((uint32_t *)((m_startPtr - smHeaderSize) + sizeof(uint32_t) * 3)) = readWriteState;
    }


    /// Properties.

    /// Return the size in bytes of the used part of the buffer.
    /// @param first Optional pointer to int32_t to receive the size of the first
    /// used block, in case of wrap.
    uint32_t used(uint32_t * const first = 0) const;

    /// Return the size in bytes of the available (unused) part of the buffer.
    /// @param first Optional pointer to int32_t to receive the size of the first
    /// available block, in case of wrap.
    uint32_t available(uint32_t * const first = 0) const;

    /// Return true if the ring buffer is empty.
    bool isEmpty() const { return (used() == 0); }

    /// Return true if the ring buffer is full.
    bool isFull() const { return (available() == 0); }


    /// Reading and writing.

    /// Write data to the ring buffer and (if successful) advance the write position.
    /// @param src Pointer to data source.
    /// @param numBytes Size of the data.
    /// @return Number of bytes written.
    uint32_t write(const void * src, const uint32_t numBytes);

    /// Write multiple blocks of data and (if successful) advance the write position.
    /// @param numBlocks The number of blocks of data.
    /// @param srcArray Array of pointers to data sources.
    /// @param numBytesArray Array of data sizes.
    /// @return Number of bytes written.
    uint32_t write(const uint32_t numBlocks, const void ** const srcArray,
		   const uint32_t * const numBytesArray);

    uint32_t write(const BinaryData & data);

    /// Peek data in the ring buffer, i.e. read but do not advance the read position.
    /// @param dst Pointer to data destination.
    /// @param numBytes Number of bytes of data to peek.
    /// @return Number of bytes peeked.
    uint32_t peek(void * const dst, const uint32_t numBytes);

    /// Peek multiple blocks of data, i.e. read but do not advance the read position.
    /// @param numBlocks The number of blocks of data.
    /// @param dstArray Array of pointers to data destinations.
    /// @param numBytesArray Array of data sizes.
    /// @return Number of bytes peeked.
    uint32_t peek(const uint32_t numBlocks, void * const * const dstArray,
		  const uint32_t * const numBytesArray);

    /// Read data from the ring buffer and (if successful) advance the read position.
    /// @param dst Pointer to data destination.
    /// @param numBytes Number of bytes of data to read.
    /// @return Number of bytes read.
    uint32_t read(void * const dst, const uint32_t numBytes);

    /// Read multiple blocks of data and (if successful) advance the read position.
    /// @param numBlocks The number of blocks of data.
    /// @param dstArray Array of pointers to data destinations.
    /// @param numBytesArray Array of data sizes.
    /// @return Number of bytes read.
    uint32_t read(const uint32_t numBlocks, void * const * const dstArray,
		  const uint32_t * const numBytesArray);

    uint32_t read(BinaryData & data);

    bool readString(std::string & str);
    
    /// Discard data from the ring buffer by advancing the read position.
    /// @param numBytes Number of bytes of data to discard.
    /// @return Number of bytes discarded.
    uint32_t discard(const uint32_t numBytes);

    /// Clear and reset the ring buffer.
    void clear() { setWritePos(0); setReadPos(0); setReadWriteState(RWS_NONE); }


    /// Diagnostics.

#ifndef WIN32
    /// Return error message for the most recent shared memory function error.
    static std::string shmError();
#endif

    /// Return true if the ring buffer is valid, false otherwise.
    bool isValid() const;

    /// Output attributes and properties.
    void dump() const;


    /// Shared memory access functions.

    /// Return a pointer to the write position.
    unsigned char * writePtr() { return (m_startPtr + writePos()); }

    /// Return a pointer to the read position.
    const unsigned char * readPtr() const { return (m_startPtr + readPos()); }


  private:


    /// Advance the write position.
    void advanceWritePos(const uint32_t advance) { setWritePos((writePos() + advance) % size()); }

    /// Advance the read position.
    void advanceReadPos(const uint32_t advance) { setReadPos((readPos() + advance) % size()); }


    /// Attributes.

    /// true if this is the creator object, false if it is a reference object.
    bool    m_isCreator;

#ifdef WIN32
    /// User-defined name for the shared memory area.
    std::string   m_smName;

    /// Handle to shared memory area.
    HANDLE  m_hMapFile;
#else
    /// User-defined key to the shared memory area.
    key_t   m_smKey;

    /// ID of the shared memory area.
    int     m_id;
#endif

    /// Pointer to the start of the ring buffer.
      
    /// @note The first smHeaderSize bytes of the shared memory area
    /// (SMA) are used to store the header information. Consequently
    /// the ring buffer start = SMA start + header size.
    unsigned char *   m_startPtr;

  };

}

#endif
