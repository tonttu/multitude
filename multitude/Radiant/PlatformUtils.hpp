/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#ifndef RADIANT_PLATFORM_UTILS_HPP
#define RADIANT_PLATFORM_UTILS_HPP

#include "Export.hpp"
#include "Platform.hpp"

#include <cstdint>

#include <QString>

namespace Radiant
{
  /** Small utility functions to help handle platform-specific functions. */
  namespace PlatformUtils
  {
    /// Information about available and total memory
    struct MemInfo
    {
      /// Total physical memory in kilobytes
      uint64_t memTotalKb = 0;
      /// Available physical memory in kilobytes. On Linux this is the value of
      /// "MemAvailable", not "MemFree", if the kernel is new enough. See
      /// https://www.kernel.org/doc/Documentation/filesystems/proc.txt
      uint64_t memAvailableKb = 0;
    };

    /// Return absolute path to the executable that was used to launch the process.
    /// @return Absolute path to the executable used to launch the process
    RADIANT_API QString getExecutablePath();

    /// Returns the current process identifier
    /// @return Process id of this process
    RADIANT_API int getProcessId();

    /// Return absolute path to the user's home directory.
    /// @return Absolute path to user's home directory
    RADIANT_API QString getUserHomePath();

    /// Return absolute path to the user's "Documents" directory.
    /// @return Absolute path to user's Documents directory
    RADIANT_API QString getUserDocumentsPath();

    /// Return path to the user data directory of the given module.
    /// @param module Name of the module whose data directory is being queried
    /// @param isapplication True if the modul is application, false otherwise.
    /// @return Absolute path of the given module inside user's home directory
    RADIANT_API QString getModuleUserDataPath(const char * module, bool isapplication);

    /// Open a dynamic library
    /// @param path Full path to plugin
    /// @returns Handle to plugin or nullptr if failed
    RADIANT_API void * openPlugin(const char * path);

    /// Returns the memory usage of the process, in bytes
    /// This function is not implemented for all platforms.
    /// @returns Size of memory usage (in bytes)
    RADIANT_API uint64_t processMemoryUsage();

    /// Returns information about available and total memory
    /// @todo not implemented on OSX
    RADIANT_API MemInfo memInfo();

    /// This function returns the path to a library the running process is
    /// linked against.
    /// @param libraryName (part of) linked library name to search for
    /// @return absolute path to the library file
    RADIANT_API QString getLibraryPath(const QString& libraryName);

    /// Sets environment variable
    /// @param name name of the environment variable
    /// @param value value of the environment variable
    RADIANT_API void setEnv(const QString & name, const QString & value);

    /// Creates a hard link at from, pointing to to
    /// @param from path where the new link will be created
    /// @param to existing file, target of the new link
    RADIANT_API bool createHardLink(const QString & from, const QString & to);

    /// Returns the number of hard links to the file.
    /// @param file file path
    /// @returns the number of hard links or -1 on error
    RADIANT_API int numberOfHardLinks(const QString & file);

    /// Make a new TCP rule to OS firewall
    /// @param port TCP port to open
    /// @param name rule name
    RADIANT_API void openFirewallPortTCP(int port, const QString & name);

    /// Reboot the system
    /// @throw QString error message
    /// @return true on success
    RADIANT_API bool reboot();

    /// Shutdown the system immediately
    /// @return true on success
    RADIANT_API bool shutdown();

    /// Terminate all processes matching the given name
    /// @param processName name of the process to terminate
    RADIANT_API void terminateProcessByName(const QString& processName);

#ifdef RADIANT_WINDOWS
    /// Get path to folder used for application data that is not user specific (i.e. ProgramData)
    RADIANT_API QString windowsProgramDataPath();

    /// @param iteration If the iteration is not given, it will not rotate the logs.
    /// @returns The absolute file path of the log file
    RADIANT_API QString newWindowsServiceLogFile(const QString & serviceName,
                                                 const QString & logName,
                                                 int iteration = -1);

    /// Searches for the latest log file for the given service and log name.
    /// You need to call this after the service is running, else you will not get
    /// reliable results.
    /// @returns The absolute file path of the log file. Null string if nothing was
    /// found.
    RADIANT_API QString findWindowsServiceLogFile(const QString & serviceName,
                                                  const QString & logName);
#endif

    /// Get the command-line arguments for the current process
    /// @return list of command-line arguments including executable name as first argument
    RADIANT_API QStringList getCommandLine();
  }
}

#endif
