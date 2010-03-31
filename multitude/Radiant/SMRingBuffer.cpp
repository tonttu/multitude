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

#include <Radiant/StringUtils.hpp>
#include <Radiant/Trace.hpp>

#include "SMRingBuffer.hpp"

#include <Nimble/Math.hpp>

#include <cassert>
#include <cerrno>
#include <iostream>
#include <memory>
#include <sstream>

#include <string.h>

#ifndef WIN32
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#endif

namespace Radiant
{

  // Class SMRingBuffer.


  // Static data initialization.

#ifdef WIN32
  uint32_t  SMRingBuffer::smDefaultPermissions = PAGE_EXECUTE_READWRITE;
#else
  // rw-rw-rw-.
  uint32_t  SMRingBuffer::smDefaultPermissions = 0666;
#endif

  uint32_t  SMRingBuffer::smHeaderSize = sizeof(uint32_t) * 4;

  // Set maximum buffer size to the largest possible value of a 32-bit integer less (header size + 1).
  uint32_t  SMRingBuffer::maxSize = 4294967295u - (smHeaderSize + 1);


  // Construction / destruction.

#ifdef WIN32
  SMRingBuffer::SMRingBuffer(const std::string smName, const uint32_t size)
    : m_isCreator(false),
      m_smName(smName),
      m_hMapFile(0),
      m_startPtr(0)
  {
    const char * const  fnName = "SMRingBuffer::SMRingBuffer";

    if(size > 0)
    // Create new shared memory area (SMA)
    {
      // Validate requested size

      if(size > maxSize)
      {
        error("%s # Requested size %ul is greater than maximum size %ul.",
          fnName, (unsigned long)(size), (unsigned long)(maxSize));
        assert(0);
      }

      // Clear any existing SMA with this name

      const HANDLE  hMapFile = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, false, m_smName.c_str());
      if(hMapFile)
      {
        if(::CloseHandle(hMapFile))
        {
          debug("%s # Successfully removed existing shared memory area with same name.", fnName);
        }
        else
        {
          error("%s # Failed to remove existing shared memory area with same name (%s).", fnName, StringUtils::getLastErrorMessage().c_str());
          assert(0);
        }
      }

      // Create the new SMA

      m_hMapFile = ::CreateFileMappingA(INVALID_HANDLE_VALUE, 0, smDefaultPermissions, 0, size, m_smName.c_str());
      if(m_hMapFile)
      {
        m_isCreator = true;
        debug("%s # Successfully created new shared memory area (%s).", fnName);
      }
      else
      {
        error("%s # Failed to create new shared memory area (%s).", fnName, StringUtils::getLastErrorMessage().c_str());
        assert(0);
      }
    }
    else
    // Try to reference existing SMA
    {
      m_hMapFile = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, false, m_smName.c_str());
      if(m_hMapFile)
      {
        debug("%s # Successfully accessed existing shared memory area (%s).", fnName);
      }
      else
      {
        error("%s # Failed to access existing shared memory area (%s).", fnName, StringUtils::getLastErrorMessage().c_str());
        assert(0);
      } 
    }

    // Get pointer to SMA

    char * const  smPtr = (char *)(::MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, size));
    if(smPtr)
    {
      debug("%s # Successfully obtained pointer to shared memory area.", fnName);
    }
    else
    {
      error("%s # Failed to obtain pointer to shared memory area (%s).", fnName, StringUtils::getLastErrorMessage().c_str());
      assert(0);
    }

    if(m_isCreator)
    // This is the creating object
    {
      // initialize header
      memset(smPtr, 0, smHeaderSize);
      // write size to header
      memcpy(smPtr, & size, sizeof(uint32_t));
    }

    // Set ring buffer pointer - buffer starts after header information

    m_startPtr = ((unsigned char *)(smPtr)) + smHeaderSize;

    assert(isValid());
  }
#else
  SMRingBuffer::SMRingBuffer(const key_t smKey, const uint32_t size)
  : m_isCreator(false)
  , m_smKey(smKey)
  , m_id(-1)
  , m_startPtr(0)
  {
    const char * const  fnName = "SMRingBuffer::SMRingBuffer";

    if(size > 0)
    // Create new shared memory area (SMA)
    {
      // Validate requested size

      if(size > maxSize)
      {
        Radiant::error("%s # Requested size %ul is greater than maximum size %ul.",
          fnName, (unsigned long)(size), (unsigned long)(maxSize));
        assert(0);
      }

      // Clear any existing SMA with this key

      const int   id = shmget(m_smKey, 0, smDefaultPermissions);
      if(id != -1)
      {
        if(shmctl(id, IPC_RMID, 0) != -1)
        {
          Radiant::trace(Radiant::DEBUG, "%s # Successfully removed existing shared memory area with same key.", fnName);
        }
        else
        {
          Radiant::error("%s # Failed to remove existing shared memory area with same key (%s).", fnName, shmError().c_str());
          assert(0);
        }
      }

      // Create the new SMA

      // shmget() rounds up size to nearest page size, so actual size of area may be greater
      // than requested size - however, this does not affect anything, the extra will simply
      // remain unused
      m_id = shmget(m_smKey, smHeaderSize + size + 1, smDefaultPermissions | IPC_EXCL | IPC_CREAT);
      if(m_id != -1)
      {
        m_isCreator = true;
        Radiant::trace(Radiant::DEBUG, "%s # Successfully created new shared memory area.", fnName);
      }
      else
      {
        Radiant::error("%s # Failed to create new shared memory area (%s).", fnName, shmError().c_str());
        assert(0);
      }
    }
    else
    // Try to reference existing SMA
    {
      m_id = shmget(m_smKey, 0, smDefaultPermissions);
      if(m_id != -1)
      {
        Radiant::trace(Radiant::DEBUG, "%s # Successfully accessed existing shared memory area.", fnName);
      }
      else
      {
        Radiant::error("%s # Failed to access existing shared memory area (%s).", fnName, shmError().c_str());
        assert(0);
      }
    }

    // Get pointer to SMA

    char * const  smPtr = (char *)(shmat(m_id, 0, 0));
    if(smPtr != (char *)(-1))
    {
      Radiant::trace(Radiant::DEBUG, "%s # Successfully obtained pointer to shared memory area.", fnName);
    }
    else
    {
      Radiant::error("%s # Failed to obtain pointer to shared memory area (%s)", fnName, shmError().c_str());
      assert(0);
    }

    if(m_isCreator)
    // This is the creating object
    {
      // initialize header
      memset(smPtr, 0, smHeaderSize);
      // write size to header
      memcpy(smPtr, & size, sizeof(uint32_t));
    }

    // Set ring buffer pointer - buffer starts after header information

    m_startPtr = ((unsigned char *)(smPtr)) + smHeaderSize;

    assert(isValid());
  }
#endif

#ifdef WIN32
  SMRingBuffer::~SMRingBuffer()
  {
    const char * const  fnName = "SMRingBuffer::~SMRingBuffer";

    assert(isValid());

    // Detach the SMA

    char * const  smPtr = (char *)(m_startPtr - smHeaderSize);
    if(::UnmapViewOfFile(smPtr))
    {
      Radiant::trace(Radiant::DEBUG, "%s # Successfully detached shared memory area.", fnName);
    }
    else
    {
      Radiant::error("%s # Failed to detach shared memory area (%s).", fnName, StringUtils::getLastErrorMessage().c_str());
    }

    // Only the creating object can destroy the SMA, after the last detach, i.e. when no more
    // objects are referencing it.

    if(m_isCreator)
    {
      if(::CloseHandle(m_hMapFile))
      {
        Radiant::trace(Radiant::DEBUG, "%s # Successfully destroyed shared memory area.", fnName);
      }
      else
      {
        Radiant::error("%s # Failed to destroy shared memory area (%s).", fnName, StringUtils::getLastErrorMessage().c_str());
      }
    }
  }
#else
  SMRingBuffer::~SMRingBuffer()
  {
    const char * const  fnName = "SMRingBuffer::~SMRingBuffer";

    assert(isValid());

    // Detach the SMA

    char * const  smPtr = (char *)(m_startPtr - smHeaderSize);
    if(shmdt(smPtr) != -1)
    {
      Radiant::trace(Radiant::DEBUG, "%s # Successfully detached shared memory area.", fnName);
    }
    else
    {
      Radiant::error("%s # Failed to detach shared memory area (%s).", fnName, shmError().c_str());
    }

    // Only the creating object can destroy the SMA, after the last detach, i.e. when no more
    // objects are referencing it.

    if(m_isCreator)
    {
      if(shmctl(m_id, IPC_RMID, 0) != -1)
      {
        Radiant::trace(Radiant::DEBUG, "%s # Successfully destroyed shared memory area.", fnName);
      }
      else
      {
        Radiant::error("%s # Failed to destroy shared memory area (%s).", fnName, shmError().c_str());
      }
    }
  }
#endif


  // Properties.

  uint32_t SMRingBuffer::used(uint32_t * const first) const
  {
    // Compute total bytes used

    const uint32_t  bufSize = size();
    const uint32_t  wPos = writePos();
    const uint32_t  rPos = readPos();
    const uint32_t  total = (rPos <= wPos) ? wPos - rPos : (bufSize + wPos) - rPos;

    if(first)
    {
      // If used part of buffer wraps, compute size of first block
      * first = ((rPos + total) > bufSize) ? bufSize - rPos : total;
    }

    return total;
  }

  uint32_t SMRingBuffer::available(uint32_t * const first) const
  {
    // Compute total bytes available

    const uint32_t  bufSize = size();
    const uint32_t  wPos = writePos();
    const uint32_t  rPos = readPos();
    const uint32_t  total = (wPos < rPos) ? (rPos - wPos) - 1 : ((bufSize + rPos) - wPos) - 1;

    if(first)
    {
      // If available part of buffer wraps, compute size of first block
      * first = ((wPos + total) >= bufSize) ? bufSize - wPos : total;
    }

    return total;
  }


  // Reading and writing.

  uint32_t SMRingBuffer::write(const void * const src, const uint32_t numBytes)
  {
    if(numBytes == 0)
    {
      return 0;
    }

    assert(src);

    // Do not write if other process is writing

    const uint32_t  prevReadWriteState = readWriteState();
    if(prevReadWriteState & RWS_WRITING)
    {
      return 0;
    }

    // Set read/write state to prevent other process from writing whilst this process is writing

    setReadWriteState(prevReadWriteState | RWS_WRITING);

    // Check sufficient space available

    uint32_t  firstAvl = 0;
    const uint32_t  totalAvl = available(& firstAvl);

    if(numBytes > totalAvl)
    {
//      error("SMRingBuffer::write # Insufficient space, %ul requested, %ul available.",
//        (unsigned long)(numBytes), (unsigned long)(totalAvl));
      return 0;
    }

    // Write the data

    unsigned char * const   wPtr = writePtr();

    if(numBytes <= firstAvl)
    // in 1 chunk
    {
      memcpy(wPtr, src, numBytes);
    }
    else
    // in 2 chunks
    {
      memcpy(wPtr, src, firstAvl);
      memcpy(m_startPtr, (unsigned char *)(src) + firstAvl, numBytes - firstAvl);
    }

    // Advance the write position

    advanceWritePos(numBytes);

    // Restore previous read/write state

    setReadWriteState(prevReadWriteState);

    return numBytes;
  }

  uint32_t SMRingBuffer::write(const uint32_t numBlocks, const void ** const srcArray,
    const uint32_t * numBytesArray)
  {
    if(numBlocks == 0)
    {
      return 0;
    }

    assert(srcArray);
    assert(numBytesArray);

    // Do not write if other process is writing

    const uint32_t  prevReadWriteState = readWriteState();
    if(prevReadWriteState & RWS_WRITING)
    {
      return 0;
    }

    // Set read/write state to prevent other process from writing whilst this process is writing

    setReadWriteState(prevReadWriteState | RWS_WRITING);

    // Compute total size of blocks

    uint32_t  totalBytes = 0;
    for(uint32_t i = 0; i < numBlocks; i++)
    {
      totalBytes += numBytesArray[i];
    }

    // Check sufficient space available

    uint32_t  firstAvl = 0;
    if(totalBytes > available(& firstAvl))
    {
      return 0;
    }

    // Write the blocks

    const uint32_t    sz = size();
    uint32_t          wPos = writePos();
    unsigned char *   wPtr = 0;

    for(uint32_t i = 0; i < numBlocks; i++)
    {
      if(!srcArray[i] || !numBytesArray[i])
      {
        continue;
      }

      wPtr = m_startPtr + wPos;

      if(numBytesArray[i] <= firstAvl)
      // in 1 chunk
      {
        memcpy(wPtr, srcArray[i], numBytesArray[i]);
      }
      else
      // in 2 chunks
      {
        memcpy(wPtr, srcArray[i], firstAvl);
        memcpy(m_startPtr, (unsigned char *)(srcArray[i]) + firstAvl, numBytesArray[i] - firstAvl);
      }

      firstAvl -= numBytesArray[i];
      wPos = (wPos + numBytesArray[i]) % sz;
    }

    // Advance the write position

    advanceWritePos(totalBytes);

    // Restore previous read/write state

    setReadWriteState(prevReadWriteState);

    return totalBytes;
  }

  uint32_t SMRingBuffer::write(const BinaryData & data)
  {
    int bytes = data.pos();
    const void * ptrs[2] = { & bytes, data.data() };
    uint32_t sizes[2] = { 4, data.pos() };

    return write(2, ptrs, sizes);
  }

  uint32_t SMRingBuffer::peek(void * const dst, const uint32_t numBytes)
  {
    if(numBytes == 0)
    {
      return 0;
    }

    assert(dst);

    // Check that peek will not overlap write position

    uint32_t         firstUsd = 0;
    const uint32_t   totalUsd = used(& firstUsd);
    if(numBytes > totalUsd)
    {
//     error("SMRingBuffer::peek # Insufficient data, %ul requested, %ul in use.",
//        (unsigned long)(numBytes), (unsigned long)(totalUsd));
      return 0;
    }

    // Peek the data

    const unsigned char * const  rPtr = readPtr();

    if(numBytes <= firstUsd)
    // in 1 chunk
    {
      memcpy(dst, rPtr, numBytes);
    }
    else
    // in 2 chunks
    {
      memcpy(dst, rPtr, firstUsd);
      memcpy((unsigned char *)(dst) + firstUsd, m_startPtr, numBytes - firstUsd);
    }

    return numBytes;
  }

  uint32_t SMRingBuffer::peek(const uint32_t numBlocks, void * const * const dstArray,
    const uint32_t * const numBytesArray)
  {
    if(numBlocks == 0)
    {
      return 0;
    }

    assert(dstArray);
    assert(numBytesArray);

    // Compute total size of blocks

    uint32_t  totalBytes = 0;
    for(uint32_t i = 0; i < numBlocks; i++)
    {
      totalBytes += numBytesArray[i];
    }

    // Check that peek will not overlap write position

    uint32_t  firstUsd = 0;
    if(totalBytes > used(& firstUsd))
    {
      return 0;
    }

    // Peek the blocks

    const uint32_t    sz = size();
    uint32_t          rPos = readPos();
    unsigned char *   rPtr = 0;

    for(uint32_t i = 0; i < numBlocks; i++)
    {
      if(!dstArray[i] || !numBytesArray[i])
      {
        continue;
      }

      rPtr = m_startPtr + rPos;

      if(numBytesArray[i] <= firstUsd)
      // in 1 chunk
      {
        memcpy(dstArray[i], rPtr, numBytesArray[i]);
      }
      else
      // in 2 chunks
      {
        memcpy(dstArray[i], rPtr, firstUsd);
        memcpy((unsigned char *)(dstArray[i]) + firstUsd, m_startPtr, numBytesArray[i] - firstUsd);
      }

      firstUsd -= numBytesArray[i];
      rPos = (rPos + numBytesArray[i]) % sz;
    }

    return totalBytes;
  }

  uint32_t SMRingBuffer::read(void * const dst, const uint32_t numBytes)
  {
    if(numBytes == 0)
    {
      return 0;
    }

    assert(dst);

    // Do not read if other process is reading

    const uint32_t  prevReadWriteState = readWriteState();
    if(prevReadWriteState & RWS_READING)
    {
      return 0;
    }

    // Set read/write state to prevent other process from reading whilst this process is reading

    setReadWriteState(prevReadWriteState | RWS_READING);

    // Read the data

    const uint32_t  bytesPeeked = peek(dst, numBytes);

    if(bytesPeeked)
    {
      // Advance the read position

      advanceReadPos(bytesPeeked);
    }

    // Restore previous read/write state

    setReadWriteState(prevReadWriteState);

    return bytesPeeked;
  }

  uint32_t SMRingBuffer::read(const uint32_t numBlocks, void * const * const dstArray,
    const uint32_t * const numBytesArray)
  {
    if(numBlocks == 0)
    {
      return 0;
    }

    assert(dstArray);
    assert(numBytesArray);

    // Do not read if other process is reading

    const uint32_t  prevReadWriteState = readWriteState();
    if(prevReadWriteState & RWS_READING)
    {
      return 0;
    }

    // Set read/write state to prevent other process from reading whilst this process is reading

    setReadWriteState(prevReadWriteState | RWS_READING);

    // Read the data

    const uint32_t  bytesPeeked = peek(numBlocks, dstArray, numBytesArray);

    if(bytesPeeked)
    {
      // Advance the read position

      advanceReadPos(bytesPeeked);
    }

    // Restore previous read/write state

    setReadWriteState(prevReadWriteState);

    return bytesPeeked;
  }

  uint32_t SMRingBuffer::read(BinaryData & data)
  {
    uint32_t bytes = 0;

    int n = read( & bytes, 4);
    if(n != 4)
      return n;

    data.rewind();
    data.ensure(bytes);
    n = read(data.data(), bytes);

    return n +  4;
  }

  bool SMRingBuffer::readString(std::string & str)
  {
    int32_t tmp = 0;
    int n = read( & tmp, 4);

    if(n != 4)
      return false;

    str.resize(tmp);

    if(tmp) {
      n = read( & str[0], tmp);
    }

    return tmp == n;
  }
  

  uint32_t SMRingBuffer::discard(const uint32_t numBytes)
  {
    if(numBytes == 0)
    {
      return 0;
    }

    // Do not discard if other process is reading

    const uint32_t  prevReadWriteState = readWriteState();
    if(prevReadWriteState & RWS_READING)
    {
      return 0;
    }

    // Set read/write state to prevent other process from reading whilst this process is discarding

    setReadWriteState(prevReadWriteState | RWS_READING);

    // Check that discard will not overlap write position

    const uint32_t   totalUsd = used();
    if(numBytes > totalUsd)
    {
      error("SMRingBuffer::discard # Insufficient data.");
      return 0;
    }

    // Discard the data

    advanceReadPos(numBytes);

    // Restore previous read/write state

    setReadWriteState(prevReadWriteState);

    return numBytes;
  }


  // Diagnostics.

#ifndef WIN32
  std::string SMRingBuffer::shmError()
  {
    std::string   errMsg;

    switch(errno)
    {
      case EACCES: errMsg = "EACCES"; break;

      case EEXIST: errMsg = "EEXIST"; break;

      case EFAULT: errMsg = "EFAULT"; break;

      case EIDRM:  errMsg = "EIDRM";  break;

      case EINVAL: errMsg = "EINVAL"; break;

      case EMFILE: errMsg = "EMFILE"; break;

      case ENOENT: errMsg = "ENOENT"; break;

      case ENOMEM: errMsg = "ENOMEM"; break;

      case ENOSPC: errMsg = "ENOSPC"; break;

      case EPERM:  errMsg = "EPERM";  break;

      default:
      {
        std::stringstream  ss;
        ss << errno;
        errMsg = std::string("errno = ") + ss.str();
      }
    }

    return errMsg;
  }
#endif

  bool SMRingBuffer::isValid() const
  {
#ifdef WIN32
    return (!m_smName.empty() && m_hMapFile && m_startPtr && writePos() < size()
      && readPos() < size() && readWriteState() <= RWS_WRITING);
#else
    return (m_smKey > 0 && m_startPtr && writePos() < size()
      && readPos() < size() && readWriteState() <= RWS_WRITING);
#endif
  }

  void SMRingBuffer::dump() const
  {
    debug("m_isCreator = %s", m_isCreator ? "true" : "false");
#ifdef WIN32
    debug("m_smName = %s", m_smName.c_str());
    debug("m_hMapFile = %p", m_hMapFile);
#else
    debug("m_smKey = %ul", (unsigned long)(m_smKey));
    debug("m_id = %d", m_id);
#endif
    debug("size() = %ul", (unsigned long)(size()));
    debug("m_startPtr = %p", m_startPtr);

    debug("writePos() = %ul", (unsigned long)(writePos()));
    debug("readPos() = %ul", (unsigned long)(readPos()));
    debug("readWriteState() = %ul", (unsigned long)(readWriteState()));

    debug("used = %ul", (unsigned long)(used()));
    debug("available() = %ul", (unsigned long)(available()));

    debug("isEmpty() = %s", isEmpty() ? "true" : "false");
    debug("isFull() = %s", isFull() ? "true" : "false");
  }

}
