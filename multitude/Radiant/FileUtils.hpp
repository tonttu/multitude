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

#include "Export.hpp"

#include <fstream>

namespace Radiant
{

  /// FileUtils contains functions for platform independent file-handing
  class RADIANT_API FileUtils
  {
  public:

    /// Get the size of a file.
    static unsigned long getFileLen(std::ifstream& file);
    /// @copydoc getFileLen
    static unsigned long getFileLen(const std::string & filename);

    /// Load a text file.
    /** The contents of the file are returned as a zero-terminated
    string. The caller is responsible for freeing the memory,
    using a call to <b>delete []</b>.*/
    /// @todo rename to loadTextFileCStr, loadTextFileWStr
    static char * loadTextFile(const char* filename);
    /// Reads the contents of a text file and returns them
    static std::wstring readTextFile(const std::string & file);

    /// Writes a string to a text file.
    static bool writeTextFile(const char * filename,
                                   const char * contents);

    /// Check if a given file is readable.
    static bool fileReadable(const char* filename);
    /// @copydoc fileReadable
    static bool fileReadable(const std::string & filename);

    /// Check if the user can append to a given file.
    /// This function is useful if you want to overwrite a file,
    /// and want to check beforehand that it is possible.
    /// @return true if the file exists and can be written.
    /// to. Otherwise false.
    static bool fileAppendable(const char* filename);

    /// Rename a file.
    static bool renameFile(const char * from, const char * to);

    /// Remove a file
    static bool removeFile(const char * filename);

    /// Extract path.
    static std::string path(const std::string & filepath);
    /// Extract filename.
    static std::string filename(const std::string & filepath);
    /// Extract filename without suffix or path
    static std::string baseFilename(const std::string & filepath);
    /// Extract filename without suffix, but with the full path
    static std::string baseFilenameWithPath(const std::string & filepath);
    /// Extract full path (including filename) without suffix.
    static std::string withoutSuffix(const std::string & filepath);
    /// Extract suffix.
    static std::string suffix(const std::string & filepath);
    /// Extract suffix, and return it in lower-case
    static std::string suffixLowerCase(const std::string & filepath);

    /// Check if a suffix matches
    static bool suffixMatch(const std::string & filename,
                 const std::string & suffix);

    /// Find a file given a list of paths to search. The directory names are
    /// separated by colon or semicolon in typical Windows or UNIX fashion
    /// (/usr/foo:/home/user/foo etc.).
    /// If the file is not found, returns an empty string.
    static std::string findFile(const std::string & filename, const std::string & paths);

    /// Try to find a file that could be over-written. If such
    /// cannot be found, then return filename.
    static std::string findOverWritable(const std::string & filename,
                                             const std::string & paths);
    /// Opens the given file for writing and creates the directories in the path if they don't exist
    static FILE * createFilePath(const std::string & filePath);

    /// Does the given filename look like an image (checks the extension)
    static bool looksLikeImage(const std::string & filePath);
    /// Does the given filename look like a video (checks the extension)
    static bool looksLikeVideo(const std::string & filePath);

    /// Returns seconds from epoch, 0 in case of error
    static unsigned long int lastModified(const std::string & filePath);

    /// Adds indentation space to the given stream
    /** This function is typically used when writing object hierarchies for
        human-readable output. */
    static void indent(FILE * f, int levels);
  };
}

#endif
