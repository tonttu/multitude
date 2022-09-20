/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RADIANT_TEMPORARY_DIR_HPP
#define RADIANT_TEMPORARY_DIR_HPP

#include "Export.hpp"

#include <QString>

namespace Radiant
{
  /// Temporary directory that can be used while this object is alive.
  /// This function is guaranteed to create a new unique directory,
  /// even if multiple processes or threads are using this at the same
  /// time, unlike QTemporaryDir.
  class TemporaryDir
  {
  public:
    /// Creates a new temporary dir
    RADIANT_API TemporaryDir();
    /// Deletes the directory and its contents
    RADIANT_API ~TemporaryDir();

    /// Returns the full absolute path to the dir
    const QString & path() const { return m_path; }

    TemporaryDir(const TemporaryDir &) = delete;
  private:
    const QString m_path;
  };
} // namespace Radiant

#endif // RADIANT_TEMPORARY_DIR_HPP
