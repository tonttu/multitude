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

#include "SHMPipe.hpp"

#include <Radiant/StringUtils.hpp>
#include <Radiant/Sleep.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/Trace.hpp>

#include <Nimble/Math.hpp>

#include <cassert>
#include <cerrno>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <sys/sem.h>
#include <string.h>

#ifndef WIN32
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#endif

namespace Radiant
{
  const char * shmError()
  {
    const char * str = strerror(errno);
    errno = 0;
    return str;
  }

  class SHMPipe::SHMHolder : public Radiant::MemCheck
  {
  public:
    SHMHolder(key_t key, uint32_t size);
    SHMHolder(int id);
    ~SHMHolder();

    void attach();
    void attachToExisting();

    void * data() { return m_data; }
    int size() const { return m_size; }
    int id() const { return m_id; }
    int sem() const { return m_sem; }

  private:
    void * m_data;
    int m_id;
    int m_size;
    int m_sem;
  };

  SHMPipe::SHMHolder::SHMHolder(key_t key, uint32_t size)
    : m_size(size)
  {
    const char * const fnName = "SHMHolder::SHMHolder";

    if(size == 0) {
      m_id = shmget(key, 0, 0660);
      if(m_id == -1) {
        error("SHMPipe::attach # Failed to attach to shared memory area (%s).", shmError());
        throw std::runtime_error("shmget failed");
      }
      attachToExisting();
      return;
    }

    // Clear any existing SMA with this key
    m_id = shmget(key, 0, 0660);
    if(m_id > 0) {
      if(shmctl(m_id, IPC_RMID, 0) != -1) {
        debug("%s # Successfully removed existing shared memory area with same key..", fnName);
      } else {
        error("%s # Failed to remove existing shared memory area with same key (%s).", fnName, shmError());
      }
    }

    // Create the new SMA
    /* shmget() rounds up size to nearest page size, so actual size
       of area may be greater than requested size - however, this
       does not affect anything, the extra will simply remain
       unused */
    m_id = shmget(key, sizeof(Data) + size, 0660 | IPC_EXCL | IPC_CREAT);
    if(m_id != -1) {
      debug("%s # Successfully created new shared memory area.", fnName);
    } else {
      error("%s # Failed to create new shared memory area (%s).",
            fnName, shmError());
      throw std::runtime_error("shmget failed");
    }

    m_sem = semget(IPC_PRIVATE, 2, 0660 | IPC_CREAT | IPC_EXCL);
    if(m_sem != -1) {
      debug("%s # Successfully created new semaphore for shared memory area.", fnName);
    } else {
      error("%s # Failed to create new semaphore for shared memory area, using polling (%s).",
            fnName, shmError());
    }

    if(m_sem != -1 && semctl(m_sem, 0, SETVAL, 0) == -1) {
      error("%s # Failed to set semaphore value to 0 (%s).",
            fnName, shmError());
      throw std::runtime_error("semctl failed");
    }

    if(m_sem != -1 &&  semctl(m_sem, 1, SETVAL, 1) == -1) {
      error("%s # Failed to set semaphore value to 1 (%s).",
            fnName, shmError());
      throw std::runtime_error("semctl failed");
    }

    attach();

    // It could be handy to mark the m_id segment to be destroyed here, since
    // it will be destroyed only when the last user detaches it. Unfortunately
    // while shmat() with correct ID continues working after that, shmget with
    // the same KEY won't work.
    /// @todo Maybe we could do this if key includes IPC_PRIVATE?
    /*
    // Mark the segment to be destroyed. It will be destroyed when
    // last user detached it
    if(shmctl(m_id, IPC_RMID, 0) != -1) {
      debug("%s # Successfully destroyed shared memory area.", fnName);
    } else {
      error("%s # Failed to destroy shared memory area (%s).",
            fnName, shmError());
    }*/
  }

  SHMPipe::SHMHolder::SHMHolder(int id)
    : m_id(id),
    m_size(0),
    m_sem(-1)
  {
    attachToExisting();
  }

  SHMPipe::SHMHolder::~SHMHolder()
  {
    const char * const fnName = "SHMHolder::~SHMHolder";
    bool last = false;

    if(m_sem != -1) {
      struct sembuf sb;
      sb.sem_num = 1;
      sb.sem_op = -1;
      sb.sem_flg = 0;
      semop(m_sem, &sb, 1);

      sb.sem_num = 1;
      sb.sem_op = 0;
      sb.sem_flg = IPC_NOWAIT;
      if(semop(m_sem, &sb, 1) == 0) {
        semctl(m_sem, 0, IPC_RMID);
        last = true;
      }
    }

    if(shmdt(m_data) != -1) {
      debug("%s # Successfully detached shared memory area.", fnName);
    } else {
      error("%s # Failed to detach shared memory area (%s).",
            fnName, shmError());
    }

    if(last) {
      // Mark the segment to be destroyed. It will be destroyed when
      // last user detached it
      if(shmctl(m_id, IPC_RMID, 0) != -1) {
        info("%s # Successfully destroyed shared memory area.", fnName);
      } else {
        error("%s # Failed to destroy shared memory area (%s).",
              fnName, shmError());
      }
    }
  }

  void SHMPipe::SHMHolder::attach()
  {
    const char * const fnName = "SHMHolder::attach";

    // Get pointer to SMA
    m_data = shmat(m_id, 0, 0);
    if(m_data != (void *)(-1)) {
      debug("%s # Successfully obtained pointer %p to shared memory area.",
            fnName, m_data);
    } else {
      error("%s # Failed to obtain pointer to shared memory area (%s)",
            fnName, shmError());
      throw std::runtime_error("shmat failed");
    }
  }

  void SHMPipe::SHMHolder::attachToExisting()
  {
    attach();
    SHMPipe::Data * d = reinterpret_cast<SHMPipe::Data*>(m_data);
    m_size = d->size;
    m_sem = d->sem;

    if(m_sem != -1) {
      struct sembuf sb;
      sb.sem_num = 1;
      sb.sem_op = 1;
      sb.sem_flg = 0;
      semop(m_sem, &sb, 1);
    }
  }

  // Class SHMPipe.

  // Construction / destruction.

#ifdef WIN32
  /*
  SHMPipe::SHMPipe(const std::string smName, const uint32_t size)
    : m_isCreator(false),
      m_smName(smName),
      m_hMapFile(0),
      m_size(0),
      m_written(0),
      m_read(0),
      m_mask(0),
      m_shm(0),
      m_pipe(0)
  {
    const char * const  fnName = "SHMPipe::SHMPipe";

    if(size > 0)
    // Create new shared memory area (SMA)
    {
      // Validate requested size

      unsigned s = 1;

      while(s < size)
	s = s << 1;

      size = s;
      m_size = size;

      if(size > maxSize)
      {
        error("%s # Requested size %lu is greater than maximum size %lu.",
          fnName, (unsigned long)(size), (unsigned long)(maxSize));
        assert(0);
      }

      // Clear any existing SMA with this name

      const HANDLE  hMapFile = ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, false, m_smName.c_str());
      if(hMapFile)
      {
        if(::CloseHandle(hMapFile)) {
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
      if(m_hMapFile) {
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
        debug("%s # Successfully accessed existing shared memory area (%s).",
	      fnName);
      }
      else
      {
        error("%s # Failed to access existing shared memory area (%s).", fnName, StringUtils::getLastErrorMessage().c_str());
        assert(0);
      } 

      m_size = readHeaderValue(SHM_SIZE_LOC);
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

    m_mask = m_size - 1;
    m_shm = (uint8_t *) smPtr;
    m_pipe = m_shm + SHM_PIPE_LOC;

    assert(isValid());
  }
  */
#else
  SHMPipe::SHMPipe(key_t key, uint32_t size)
    : m_holder(new SHMHolder(key, size)),
    m_data(*reinterpret_cast<Data*>(m_holder->data()))
  {
    m_data.size = m_holder->size();
    m_data.sem = m_holder->sem();
  }

  SHMPipe::SHMPipe(int id)
    : m_holder(new SHMHolder(id)),
    m_data(*reinterpret_cast<Data*>(m_holder->data()))
  {
    info("Opened client SHMPipe with %u buffer bytes", size());
  }

  SHMPipe * SHMPipe::create(uint32_t size)
  {
    // shmget(2) BUGS section:
    // The name choice IPC_PRIVATE was perhaps unfortunate,
    // IPC_NEW would more clearly show its function.
    return new SHMPipe(IPC_PRIVATE, size);
  }

#endif

#ifdef WIN32
  /*
  SHMPipe::~SHMPipe()
  {
    const char * const  fnName = "SHMPipe::~SHMPipe";

    assert(isValid());

    // Detach the SMA

    char * const  smPtr = (char *)(m_shm - SHM_HEADER_SIZE);
    if(::UnmapViewOfFile(smPtr))
    {
     debug("%s # Successfully detached shared memory area.", fnName);
    }
    else
    {
      error("%s # Failed to detach shared memory area (%s).",
	    fnName, StringUtils::getLastErrorMessage().c_str());
    }

    // Only the creating object can destroy the SMA, after the last detach, i.e. when no more
    // objects are referencing it.

    if(m_isCreator) {
      if(::CloseHandle(m_hMapFile)) {
	debug("%s # Successfully destroyed shared memory area.", fnName);
      }
      else {
        error("%s # Failed to destroy shared memory area (%s).", fnName, StringUtils::getLastErrorMessage().c_str());
      }
    }
  }
  */

#endif

  int SHMPipe::read(void * dest, int n, bool block, bool peek)
  {
    if(block) {
      readAvailable(n);
    } else {
      uint32_t avail = readAvailable();

      if(static_cast<int>(avail) < n) {
        //debug("SHMPipe::read # Only %d available, %d needed (%u %u)",
        // (int) avail, n, (unsigned) readPos(), (unsigned) writePos());
        return 0;
      }
    }

    if(readPos() + n > size()) {
      int n1 = size() - readPos();
      memcpy(dest, m_data.pipe + readPos(), n1);
      memcpy(reinterpret_cast<char*>(dest) + n1, m_data.pipe, n - n1);
      if(!peek) m_data.readPos = n - n1;
    } else {
      memcpy(dest, m_data.pipe + readPos(), n);
      if(!peek) m_data.readPos += n;
    }

    return n;
  }

  void SHMPipe::consume(int n)
  {
    m_data.readPos = (m_data.readPos + n) % m_data.size;
  }

  int SHMPipe::read(BinaryData & data)
  {
    data.rewind();

    uint32_t bytes = 0;

    uint32_t n = read( & bytes, 4);

    if(n != 4) {
      // debug("SHMPipe::read # could not read 4 bytes");
      return n;
    }

    if(bytes > m_data.size) {
      error("SHMPipe::read # Too large object to read, stream corrupted %u",
            (int) bytes);
      return n;
    }

    data.ensure(bytes);
    n = read(data.data(), bytes);
    data.setTotal(n);

    if(n != bytes) {
      error("SHMPipe::read # could not read final %d vs %d (%u)",
            n, (int) bytes, readPos());
    }

    return n + 4;
  }

  uint32_t SHMPipe::readAvailable()
  {
    uint32_t rp = readPos();
    uint32_t wp = writePos();

    return wp >= rp ? wp - rp : wp + size() - rp;
  }

  uint32_t SHMPipe::readAvailable(uint32_t require)
  {
    uint32_t avail = readAvailable();
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    while(avail < require) {
      if(m_data.sem != -1)
        semop(m_data.sem, &sb, 1);
      else
        Sleep::sleepMs(2);
      avail = readAvailable();
    }
    return avail;
  }

  int SHMPipe::write(const void * src, int n)
  {
    uint32_t avail = writeAvailable();

    n = Nimble::Math::Min<uint32_t>(n, avail);

    if(m_data.written + n > size()) {
      int n1 = size() - m_data.written;
      memcpy(m_data.pipe + m_data.written, src, n1);
      memcpy(m_data.pipe, reinterpret_cast<const char*>(src) + n1, n - n1);
      m_data.written = n - n1;
    } else {
      memcpy(m_data.pipe + m_data.written, src, n);
      m_data.written += n;
    }

    return n;
  }

  int SHMPipe::write(const BinaryData & data)
  {
    uint32_t wavail = writeAvailable(data.pos() + 4);
    if(wavail < (uint32_t) data.pos() + 4) {
      error("SHMPipe::write # Not enough space in the pipe (%u, %u < %u)",
            (unsigned) m_data.written, (unsigned) wavail, (unsigned) data.pos() + 4 );
      return 0;
    }

    uint32_t bytes = data.pos();
    if(write(&bytes, 4) != 4)
      return 0;

    return write(data.data(), bytes) + 4;
  }

  uint32_t SHMPipe::writeAvailable()
  {
    uint32_t rp = readPos();
    uint32_t wp = m_data.written;

    return wp >= rp ? rp + size() - wp : rp - wp;
  }

  uint32_t SHMPipe::writeAvailable(uint32_t require)
  {
    int times = 0;
    uint32_t avail = writeAvailable();

    while(avail < require && times++ < 100) {
      Sleep::sleepMs(2);
      avail = writeAvailable();
    }
    return avail;
  }

  void SHMPipe::flush()
  {
    m_data.writePos = m_data.written;


    // if SEMVMX wasn't so small, we could use the semaphore value as "writeAvailable"
    /*
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = n;
    sb.sem_flg = 0;
    semop(m_data.sem, &sb, 1);*/

    if(m_data.sem != -1)
      semctl(m_data.sem, 0, SETVAL, 1);
    // info("SHMPipe::flush # Flushed out written data (%u)",(unsigned) m_written);
  }

  void SHMPipe::clear()
  {
    m_data.written = 0;
    m_data.writePos = 0;
    m_data.readPos = 0;
  }

  int SHMPipe::id() const
  {
    return m_holder->id();
  }

  void SHMPipe::deleteShm(int id)
  {
    Data * data = reinterpret_cast<Data*>(shmat(id, 0, 0));
    if(data == (void *)(-1))
      return;

    if(data->sem != -1)
      semctl(data->sem, 0, IPC_RMID);

    shmdt(data);
    shmctl(id, IPC_RMID, 0);
  }

  // Diagnostics.
  void SHMPipe::dump() const
  {
#ifdef WIN32
    debug("m_smName = %s", m_smName.c_str());
    debug("m_hMapFile = %p", m_hMapFile);
#else
#endif
    debug("size() = %lu", (unsigned long)(size()));

    debug("writePos() = %lu", (unsigned long)(writePos()));
    debug("readPos() = %lu", (unsigned long)(readPos()));
    /*
    debug("readWriteState() = %lu", (unsigned long)(readWriteState()));

    debug("used = %lu", (unsigned long)(used()));
    debug("available() = %lu", (unsigned long)(available()));

    debug("isEmpty() = %s", isEmpty() ? "true" : "false");
    debug("isFull() = %s", isFull() ? "true" : "false");
    */
  }

}
