/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#pragma once

#include "Export.hpp"

#include <QString>

#include <memory>

namespace Radiant
{
  /// Platform independent lockfile.
  /// This can be used as a multi-process mutex
  class RADIANT_API LockFile
  {
  public:
    /// Tries to acquire an exclusive lock on the given file.
    /// No other operations should be done on this file
    /// If the file doesn't exist it is created.
    /// @param filename Filename of lockfile
    /// @block If true, the constructor will wait until the lock is acquired or an error is triggered
    LockFile(const QString & filename, bool block);

    /// Releases the lock
    ~LockFile();

    /// Check if the file is locked
    /// @returns true if the file was succesfully locked for exclusive use
    bool isLocked() const;

    /// Locks the file, returns true if locking succeeded or if the file was already locked.
    bool lock(bool block);

    /// Releases the lock
    void unlock();

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}
