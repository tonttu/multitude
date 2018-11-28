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
  class LockFile_Impl
  {
  public:
    LockFile_Impl(const char * filename)
    {
      m_fd = CreateFileA(filename, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }

    ~LockFile_Impl()
    {
      CloseHandle(m_fd);
    }

    bool isLocked() const
    {
      return (m_fd != INVALID_HANDLE_VALUE);
    }
  private:
    HANDLE m_fd;
  };

  LockFile::LockFile(const char * filename)
  {
    m_impl = new LockFile_Impl(filename);
  }

  LockFile::~LockFile()
  {
    delete m_impl;
  }

  bool LockFile::isLocked() const
  {
    return m_impl->isLocked();
  }
}

#endif
