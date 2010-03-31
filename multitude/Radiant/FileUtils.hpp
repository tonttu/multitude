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

#ifndef RADIANT_FILEUTILS_HPP
#define RADIANT_FILEUTILS_HPP

#include <fstream>

#include <Radiant/Export.hpp>

namespace Radiant
{

  /// FileUtils contains functions for platform independent file-handing
  namespace FileUtils
  {
    
    /// Get the size of a file.
    RADIANT_API unsigned long getFileLen(std::ifstream& file);
    RADIANT_API unsigned long getFileLen(const std::string & filename);

    /// Load a text file.
    /** The contents of the file are returned as a zero-terminated
	string. The caller is responsible for freeing the memory,
	using a call to <b>delete []</b>.*/

    /// @todo rename to loadTextFileCStr, loadTextFileWStr

    RADIANT_API char * loadTextFile(const char* filename);
    RADIANT_API std::wstring readTextFile(const std::string & file);

    /// Writes a string to a text file.
    RADIANT_API bool writeTextFile(const char * filename, 
                                   const char * contents);

    /// Check if a given file is readable.
    RADIANT_API bool fileReadable(const char* filename);
    RADIANT_API bool fileReadable(const std::string & filename);

    /// Check if the user can append to a given file.
    /// This function is useful if you want to overwrite a file,
    /// and want to check beforehand that it is possible.
    /// @return true if the file exists and can be written.
    /// to. Otherwise false.
    RADIANT_API bool fileAppendable(const char* filename);

    /// Rename a file.
    RADIANT_API bool renameFile(const char * from, const char * to);

    /// Extract path.
    RADIANT_API std::string path(const std::string & filepath);
    /// Extract filename.
    RADIANT_API std::string filename(const std::string & filepath);
    /// Extract filename without suffix.
    RADIANT_API std::string baseFilename(const std::string & filepath);
    /// Extract full path (including filename) without suffix.
    RADIANT_API std::string withoutSuffix(const std::string & filepath);
    /// Extract suffix.
    RADIANT_API std::string suffix(const std::string & filepath);
    /// Extract suffix, and return it in lower-case
    RADIANT_API std::string suffixLowerCase(const std::string & filepath);

    /// Check if a suffix matches
    RADIANT_API bool suffixMatch(const std::string & filename,
				 const std::string & suffix);

    /// Find a file given a list of paths to search. The directory names are
    /// separated by colon or semicolon in typical Windows or UNIX fashion
    /// (/usr/foo:/home/user/foo etc.).
    /// If the file is not found, returns an empty string.
    RADIANT_API std::string findFile(const std::string & filename, const std::string & paths);

    /// Try to find a file that could be over-written. If such
    /// cannot be found, then return filename.
    RADIANT_API std::string findOverWritable(const std::string & filename,
                                             const std::string & paths);

    RADIANT_API FILE * createFilePath(const std::string & filePath);

    RADIANT_API bool looksLikeImage(const std::string & filePath);
    RADIANT_API bool looksLikeVideo(const std::string & filePath);

  }
}

#endif
