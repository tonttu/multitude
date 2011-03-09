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
  class FileUtils
  {
  public:

    /// Get the size of a file.
    RADIANT_API static unsigned long getFileLen(std::ifstream& file);
    /// @copydoc getFileLen
    RADIANT_API static unsigned long getFileLen(const QString & filename);

    /// Load a text file.
    /** The contents of the file are returned as a zero-terminated
    string. The caller is responsible for freeing the memory,
    using a call to <b>delete []</b>.*/
    /// @todo rename to loadTextFileCStr, loadTextFileWStr
    RADIANT_API static char * loadTextFile(const char* filename);
    /// Reads the contents of a text file and returns them
    RADIANT_API static QString readTextFile(const QString & file);

    /// Writes a string to a text file.
    RADIANT_API static bool writeTextFile(const char * filename,
                                   const char * contents);

    /// Check if a given file is readable.
    RADIANT_API static bool fileReadable(const char* filename);
    /// @copydoc fileReadable
    RADIANT_API static bool fileReadable(const QString & filename);

    /// Check if the user can append to a given file.
    /// This function is useful if you want to overwrite a file,
    /// and want to check beforehand that it is possible.
    /// @return true if the file exists and can be written.
    /// to. Otherwise false.
    RADIANT_API static bool fileAppendable(const char* filename);

    /// Rename a file.
    RADIANT_API static bool renameFile(const char * from, const char * to);

    /// Remove a file
    RADIANT_API static bool removeFile(const char * filename);

    /// Extract path.
    RADIANT_API static QString path(const QString & filepath);
    /// Extract filename.
    RADIANT_API static QString filename(const QString & filepath);
    /// Extract filename without suffix or path
    RADIANT_API static QString baseFilename(const QString & filepath);
    /// Extract filename without suffix, but with the full path
    RADIANT_API static QString baseFilenameWithPath(const QString & filepath);
    /// Extract full path (including filename) without suffix.
    RADIANT_API static QString withoutSuffix(const QString & filepath);
    /// Extract suffix.
    RADIANT_API static QString suffix(const QString & filepath);
    /// Extract suffix, and return it in lower-case
    RADIANT_API static QString suffixLowerCase(const QString & filepath);

    /// Check if a suffix matches
    RADIANT_API static bool suffixMatch(const QString & filename,
                 const QString & suffix);

    /// Find a file given a list of paths to search. The directory names are
    /// separated by colon or semicolon in typical Windows or UNIX fashion
    /// (/usr/foo:/home/user/foo etc.).
    /// If the file is not found, returns an empty string.
    RADIANT_API static QString findFile(const QString & filename, const QString & paths);

    /// Try to find a file that could be over-written. If such
    /// cannot be found, then return filename.
    RADIANT_API static QString findOverWritable(const QString & filename,
                                             const QString & paths);
    /// Opens the given file for writing and creates the directories in the path if they don't exist
    RADIANT_API static FILE * createFilePath(const QString & filePath);

    /// Does the given filename look like an image (checks the extension)
    RADIANT_API static bool looksLikeImage(const QString & filePath);
    /// Does the given filename look like a video (checks the extension)
    RADIANT_API static bool looksLikeVideo(const QString & filePath);

    /// Returns seconds from epoch, 0 in case of error
    RADIANT_API static unsigned long int lastModified(const QString & filePath);

    /// Adds indentation space to the given stream
    /** This function is typically used when writing object hierarchies for
        human-readable output. */
    RADIANT_API static void indent(FILE * f, int levels);
  };
}

#endif
