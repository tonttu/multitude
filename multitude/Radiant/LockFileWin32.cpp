/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Platform.hpp"

#ifdef RADIANT_WINDOWS

#include "LockFile.hpp"

#include <windows.h>

namespace Radiant
{
  class LockFile::D
  {
  public:
    D(const QString & filename)
      : m_filename(filename)
    {}

  public:
    QString m_filename;
    HANDLE m_fd = INVALID_HANDLE_VALUE;
  };

  LockFile::LockFile(const QString & filename, bool block)
    : m_d(new D(filename))
  {
    lock(block);
  }

  LockFile::~LockFile()
  {
    unlock();
  }

  bool LockFile::isLocked() const
  {
    return m_d->m_fd != INVALID_HANDLE_VALUE;
  }

  bool LockFile::lock(bool block)
  {
    if (isLocked())
      return true;

    m_d->m_fd = CreateFileA(m_d->m_filename.toUtf8().data(),
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            nullptr,
                            OPEN_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            nullptr);

    if (m_d->m_fd == INVALID_HANDLE_VALUE)
      return false;

    DWORD flags = LOCKFILE_EXCLUSIVE_LOCK;
    if (!block)
      flags |= LOCKFILE_FAIL_IMMEDIATELY;

    OVERLAPPED overlapped{};
    if (LockFileEx(m_d->m_fd, flags, 0, 1, 0, &overlapped)) {
      return true;
    } else {
      CloseHandle(m_d->m_fd);
      m_d->m_fd = INVALID_HANDLE_VALUE;
      return false;
    }
  }

  void LockFile::unlock()
  {
    if (m_d->m_fd != INVALID_HANDLE_VALUE) {
      CloseHandle(m_d->m_fd);
      m_d->m_fd = INVALID_HANDLE_VALUE;
    }
  }
}

#endif
