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

#ifdef RADIANT_UNIX

#include "LockFile.hpp"
#include <sys/file.h>
#include <unistd.h>

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
    int m_fd = -1;
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
    return m_d->m_fd >= 0;
  }

  bool LockFile::lock(bool block)
  {
    if (isLocked())
      return true;

    int op = LOCK_EX;
    if (!block)
      op |= LOCK_NB;

    m_d->m_fd = open(m_d->m_filename.toUtf8().data(), O_CREAT | O_CLOEXEC, 0644);
    if (flock(m_d->m_fd, op) == 0) {
      return true;
    } else {
      close(m_d->m_fd);
      m_d->m_fd = -1;
      return false;
    }
  }

  void LockFile::unlock()
  {
    if (m_d->m_fd >= 0) {
      flock(m_d->m_fd, LOCK_UN);
      close(m_d->m_fd);
      m_d->m_fd = -1;
    }
  }
}

#endif
