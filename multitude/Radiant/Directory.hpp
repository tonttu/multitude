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

#ifndef RADIANT_DIRECTORY_HPP
#define RADIANT_DIRECTORY_HPP

#include <Patterns/NotCopyable.hpp>

#include <Radiant/Export.hpp>

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
  class RADIANT_API Directory : public Patterns::NotCopyable
  {
  public:
    /// Flags to filter directory contents
    enum FilterFlags
    {
      Dirs  = 0x001,
      Files = 0x002,
      NoDotAndDotDot = 0x1000,
      Hidden = 0x100,
      AllEntries = Dirs | Files      
    };

    /// Flags to sort files
    enum SortFlag
    {
      /// Sort by name
      Name = 0x00,
      /// Do not sort
      NotSorted = 0x03
    };

    /// Construct a directory listing
    /** Creating a Directory object immediately scans the contents
	of the directory. Entries matching the given filters are
	included.

	@param pathname directory path
	@param filters one or more filter flags OR'ed together
	@param sortFlag flag indicating how the results should be sorted
    */
    Directory(const char * pathname,
        int filters = AllEntries | NoDotAndDotDot, SortFlag sortFlag = Name);
    /// Construct a directory listing
    /** Creating a Directory object immediately scans the contents
  of the directory. Entries matching the given filters are
  included.

  @param pathname directory path
  @param filters one or more filter flags OR'ed together
  @param sortFlag flag indicating how the results should be sorted
    */
    Directory(const QString & pathname,
        int filters = AllEntries | NoDotAndDotDot, SortFlag sortFlag = Name);
    /// Construct a directory listing
    /** Creating a Directory object immediately scans the contents
	of the directory. Entries matching the given filters are
	included.

	@param pathname directory path

	@param suffixlist list of accpeted suffices, for example
	"jpg,png,tiff"

	@param filters one or more filter flags OR'ed together
	@param sortFlag flag indicating how the results should be sorted
    */
    Directory(const char * pathname, const char * suffixlist,
        int filters = AllEntries | NoDotAndDotDot, SortFlag sortFlag = Name);

 
    /// Deallocates the list
    virtual ~Directory();

    /// Returns the number of entries in the directory.
    int count() const;

    /** Return given entry name in the directory
	@param n integer index of file   
	@return filename
    */
    QString fileName(int n) const;

    /** Return the full path name of the nth file.

	This method is equal to calling "dir.path() + dir.filename(n)".
    */
    QString fileNameWithPath(int n) const;

    /// Returns the directory path
    const QString & path() const { return m_path; } 

    /// Creates a new directory.
    static bool mkdir(const char * dirname);
    /// Creates a new directory.
    static bool mkdir(const QString & dirname);
    /// Creates a new directory recursively
    static bool mkdirRecursive(const QString & dirname);
    /// Checks if the given directory exists
    static bool exists(const QString & dir);

  private:
    // Calling a constructor from another is evil but we
    // can put all dupplicated code in the same private
    // method
    void init(const QString & pathname, const char * suffixlist,
	      const int filters, const SortFlag sortFlag) ;

    // This function takes care of the low-level platform
    // specific stuff.  All it does is fill up m_entries
    // with the directory contents matching the flags.
    void populate();
		
    QString m_path;
    QStringList m_entries;
    QStringList m_suffixes;
    int m_filterFlags;
    SortFlag m_sortFlags;
  };

}

#endif
