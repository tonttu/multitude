/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#if !defined (RADIANT_LOCKFILE_HPP)
#define RADIANT_LOCKFILE_HPP

#include "Radiant/Export.hpp"

namespace Radiant
{
  /// Platform independent lockfile.
  /// This can be used as a multi-process mutex
  class RADIANT_API LockFile
  {
  public:
    /** Tries to acquire an exclusive lock on the given file.
        No other operations should be done on this file
        If the file doesn't exist it is created.
    */
    /// @param filename Filename of lockfile
    LockFile(const char * filename);

    /// Releases the lock
    ~LockFile();

    /// @returns true if the file was succesfully locked for exclusive use
    bool isLocked() const;

  private:
    class LockFile_Impl * m_impl;
  };
}
#endif // RADIANT_LOCKFILE_HPP
