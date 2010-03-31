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

#include <Radiant/Export.hpp>

#include <stdexcept>
#include <string>
#include <vector>

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
    enum FilterFlags
    {
      Dirs  = 0x001,
      Files = 0x002,
      NoDotAndDotDot = 0x1000,
      Hidden = 0x100,
      AllEntries = Dirs | Files      
    };

    enum SortFlag
    {
      Name = 0x00,
      Unsorted = 0x03
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
	      int filters = AllEntries, SortFlag sortFlag = Name);
    Directory(const std::string & pathname,
	      int filters = AllEntries, SortFlag sortFlag = Name);
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
	      int filters = AllEntries, SortFlag sortFlag = Name);

 
    /// Deallocates the list
    virtual ~Directory();

    /// Returns the number of entries in the directory.
    int count() const;

    /** Return given entry name in the directory
	@param n integer index of file   
	@return filename
    */
    std::string fileName(int n) const;

    /** Return the full path name of the nth file.

	This method is equal to calling "dir.path() + dir.filename(n)".
    */
    std::string fileNameWithPath(int n) const;

    /// Returns the directory path
    const std::string & path() const { return m_path; } 

    /// Creates a new directory.
    static bool mkdir(const char * dirname);
    static bool mkdir(const std::string & dirname);
    static bool mkdirRecursive(const std::string & dirname);
    static bool exists(const std::string & dir);

  private:

    /// @todo make use of NotCopyable
    Directory(const Directory &) {}
    Directory & operator = (const Directory &) { return * this;}

    // Calling a constructor from another is evil but we
    // can put all dupplicated code in the same private
    // method
    void init(const std::string & pathname, const char * suffixlist,
	      const int filters, const SortFlag sortFlag) ;

    // This function takes care of the low-level platform
    // specific stuff.  All it does is fill up m_entries
    // with the directory contents matching the flags.
    void populate();
		
    std::string m_path;
    std::vector<std::string> m_entries;      
    std::vector<std::string> m_suffixes;
    int m_filterFlags;
    SortFlag m_sortFlags;
  };

}

#endif
