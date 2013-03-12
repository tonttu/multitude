/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_FILEUTILS_HPP
#define RADIANT_FILEUTILS_HPP

#include "Export.hpp"
#include "Platform.hpp"
#include "TimeStamp.hpp"

#include <Patterns/NotCopyable.hpp>

#include <fstream>

#include <QString>

namespace Radiant
{

  /// @cond

  /// This class is used by MultiTaction firmware to implement mount_rw
  class RADIANT_API FileWriter : public Patterns::NotCopyable
  {
  public:
    FileWriter();
    ~FileWriter();

    /// with std::function<void ()> f GCC 4.4 gives internal compiler error:
    /// /usr/include/c++/4.4/tr1_impl/functional:1535: internal compiler error: Segmentation fault
    /// so we use plain function pointers
    static void setInitFunction(void (*f)());
    static void setDeinitFunction(void (*f)());
  };

  /// @endcond

  /// FileUtils contains functions for platform independent file-handing
  class RADIANT_API FileUtils
  {
  public:
    /// Get the size of a file.
    /// @param file File to retrieve length of
    /// @returns the length of the file. Returns 0 if the file is empty or the file could not be found
    static unsigned long getFileLen(std::ifstream& file);
    /// @copybrief getFileLen
    /// @param filename Name of file to retrieve length of
    /// @returns the length of the file. Returns 0 if the file is empty or the file could not be found
    static unsigned long getFileLen(const QString & filename);

    /// Load a text file. If the reading fails, the returned QByteArray.isNull().
    /// @param filename Name of file
    /// @returns the contents of the text file
    static QByteArray loadTextFile(const QString & filename);

    /// Writes a string to a text file.
    /// @param filename Name of output file
    /// @param contents String to write to file
    /// @returns true if the contents have been succesfully written
    static bool writeTextFile(const char * filename,
                              const char * contents);

    /// Check if a given file is readable.
    /// @param filename Name of file
    /// @returns true if the specified file is readable
    static bool fileReadable(const QString & filename);

    /// Check if the user can append to a given file.
    /// This function is useful if you want to overwrite a file,
    /// and want to check beforehand that it is possible.
    /// @returns true if the file exists and can be written to. Otherwise false.
    /// @param filename Name of file
    static bool fileAppendable(const QString & filename);

    /// Rename a file.
    /// @param from Source filename
    /// @param to Destination filename
    /// @returns true if the rename was succesful
    static bool renameFile(const char * from, const char * to);

    /// Remove a file
    /// @param filename Name of file
    /// @returns true if the file was succesfully removed
    static bool removeFile(const char * filename);

    /// Extract path.
    /// @param filepath Full filename with path
    /// @returns the extracted path 
    static QString path(const QString & filepath);
    /// Extract filename.
    /// @param filepath Full filename with path
    /// @returns The extracted filename of the path
    static QString filename(const QString & filepath);
    /// Extract the base filename
    /// @param filepath Full filename with path
    /// @returns The extracted base filename without suffix or path
    static QString baseFilename(const QString & filepath);
    /// Extract the base filename
    /// @param filepath Full filename with path
    /// @returns The extracted base filename without suffix but with the full path
    static QString baseFilenameWithPath(const QString & filepath);
    /// Extract suffix.
    /// @param filepath Full filename with path
    /// @returns The extracted suffix
    static QString suffix(const QString & filepath);
    /// Extract suffix, and return it in lower-case
    /// @param filepath Full filename with path
    /// @returns The extracted lower-case suffix
    static QString suffixLowerCase(const QString & filepath);

    /// Check if a suffix matches
    /// @param filename Name of file
    /// @param suffix Suffix of file with dot (e.g. ".png")
    /// @returns true if the suffix matches
    static bool suffixMatch(const QString & filename,
                            const QString & suffix);

    /// Find a file given a list of paths to search. The directory names are
    /// separated by the platform path separator (colon on UNIX, semicolon on Windows)
    /// If the file is not found, returns an empty string.
    /// @param filename Name of file
    /// @param paths Search path
    /// @returns The full path to the found file, or an empty string if it couldn't be found
    static QString findFile(const QString & filename, const QString & paths);

    /// Try to find a file that could be over-written. If such
    /// cannot be found, then return filename.
    /// @param filename Name of file
    /// @param paths Search path
    /// @returns The full path to the found file, or an empty string if it couldn't be found
    static QString findOverWritable(const QString & filename,
                                    const QString & paths);

    /// Opens the given file for writing and creates the directories in the path if they don't exist
    /// @param filePath Full filename with path
    /// @returns A handle to the open filepath or NULL if the file couldn't be created
    static FILE * createFilePath(const QString & filePath);

    /// Does the given filename look like an image (checks the extension)
    /// @param filePath Full filename with path
    /// @returns true if the specified file looks like it's an image
    static bool looksLikeImage(const QString & filePath);
    /// Does the given filename look like a video (checks the extension)
    /// @param filePath Full filename with path
    /// @returns true if the specified file looks like it's a video
    static bool looksLikeVideo(const QString & filePath);

    /// Returns seconds from epoch, 0 in case of error
    /// @param filePath Full filename with path
    /// @returns timestamp of last modification or 0 if the time could not be retrieved
    static Radiant::TimeStamp lastModified(const QString & filePath);

    /// Adds indentation space to the given stream
    /// @param f Handle to a file stream
    /// @param levels Number of indentation levels to insert
    /** This function is typically used when writing object hierarchies for
        human-readable output. */
    static void indent(FILE * f, int levels);

		/// Returns the path separator for the current platform
    static QString pathSeparator();

		/// Returns the directory separator for the current platform
    static QString directorySeparator();
  };
}

#endif
