/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_DIRECTORY_HPP
#define RADIANT_DIRECTORY_HPP

#include "Export.hpp"

#include <Patterns/NotCopyable.hpp>

#include <stdexcept>
#include <QString>
#include <vector>

#include <QStringList>

#ifdef WIN32
#pragma warning(disable : 4290)
#endif

struct dirent;

namespace Radiant
{

  /** Class for reading filenames in a directory.
	
      This class exists because there exists no portable way to read
      directories (which is sad).

      @author Esa Nuuros
  */
  class RADIANT_API Directory
  {
  public:
    /// Flags to filter directory contents
    enum FilterFlags
    {
      DIRS  = 0x001, ///< Accept directories
      FILES = 0x002, ///< Accept Files
      NO_DOT_AND_DOTDOT = 0x1000, ///< Do not accept "." or ".."
      HIDDEN = 0x100, ///< Accept hidden files/directories
      SYSTEM = 0x004, ///< Accept system system files
      ALL_ENTRIES = DIRS | FILES | SYSTEM ///< Accept everything
    };

    /// Flags to sort files
    enum SortFlag
    {
      /// Sort by name
      NAME = 0x00,
      /// Do not sort
      NOT_SORTED = 0x03
    };

    /// Construct a directory listing
    /// Creating a Directory object immediately scans the contents
    /// of the directory. Entries matching the given filters are
    /// included.
    ///
    /// @param pathname directory path
    /// @param filters one or more filter flags OR'ed together
    /// @param sortFlag flag indicating how the results should be sorted
    Directory(const QString & pathname,
        int filters = ALL_ENTRIES | NO_DOT_AND_DOTDOT, SortFlag sortFlag = NAME);

    /// Construct a directory listing
    /// Creating a Directory object immediately scans the contents
    /// of the directory. Entries matching the given filters are
    /// included.
    ///
    /// @param pathname directory path
    /// @param suffixlist list of accpeted suffices, for example "jpg,png,tiff"
    ///	@param filters one or more filter flags OR'ed together
    /// @param sortFlag flag indicating how the results should be sorted
    Directory(const QString & pathname, const QString & suffixlist,
        int filters = ALL_ENTRIES | NO_DOT_AND_DOTDOT, SortFlag sortFlag = NAME);

    /// Destructor
    virtual ~Directory();

    /// Returns the number of entries in the directory.
    /// @return Number of filtered entries
    int count() const;

    /// Return given entry name in the directory
    /// @param n index of item in selected sorting scheme
    /// @return filename of selected item
    QString fileName(int n) const;

    /// Get the full path name of the nth file.
    /// This method is equal to concatenation of strings returned by @ref path and
    /// @ref fileName.
    /// @param n index of the file
    /// @return full path to the requested file
    QString fileNameWithPath(int n) const;

    /// Returns the directory path
    /// @return Path to the directory
    const QString & path() const { return m_path; } 

    /// Creates a new directory.
    /// @param dirname Name of the directory to create
    /// @return True if creation succeeded, false otherwise
    static bool mkdir(const QString & dirname);

    /// Creates a new directory recursively
    /// @param path Path to new directory
    /// @return True if succeeded, false otherwise
    static bool mkdirRecursive(const QString & path);

    /// Checks if the given directory exists
    /// @param dir Directory to search
    /// @return True if exists, false otherwise
    static bool exists(const QString & dir);

    /// Create a directory listing with MIME pattern filtering
    /// @param pathname path of the directory
    /// @param mimePattern mime pattern for files to match
    /// @param filters one or more filters flags
    /// @param sortFlag flag indicating how the results are sorted
    /// @return directory object with the matching filters
    static Directory findByMimePattern(const QString & pathname,
                                       const QString & mimePattern,
                                       int filters = ALL_ENTRIES | NO_DOT_AND_DOTDOT,
                                       SortFlag sortFlag = NAME);

  private:
    // This function takes care of the low-level platform
    // specific stuff.  All it does is fill up m_entries
    // with the directory contents matching the flags.
    void populate();
		
    QString m_path;
    QStringList m_entries;
    QStringList m_suffixes;
    int m_filterFlags;
    SortFlag m_sortFlag;
  };

}

#endif
