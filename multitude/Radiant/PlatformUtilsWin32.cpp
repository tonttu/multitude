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

#include "Platform.hpp"

#ifdef RADIANT_WINDOWS

#include "PlatformUtils.hpp"
#include "StringUtils.hpp"
#include "Trace.hpp"

#include <assert.h>

#include <shlobj.h>
#include <shlwapi.h>

#include <psapi.h>
#include <process.h>

#include <sys/types.h>
#include <sys/stat.h>

namespace Radiant
{

  namespace PlatformUtils
  {

    QString getExecutablePath()
    {
      // Get the full exe path / fileneme

      QString   path;

      char  buffer[_MAX_PATH] = "";
      if(GetModuleFileNameA(0, buffer, _MAX_PATH) != 0)
      {
         // remove the filename part
         PathRemoveFileSpecA(buffer);
         path = QString(buffer);
      }
      else
      {
        error("PlatformUtils::getExecutablePath # GetModuleFileName() failed");
      }

      return path;
    }

    /// Returns the current process identifier
    int getProcessId()
    {
      return GetCurrentProcessId();
    }

    QString getUserHomePath()
    {
      // Typically this retrieves "C:\Documents and Settings\(username)"

      QString   path;

      char  buffer[_MAX_PATH] = "";
      if(SHGetFolderPathA(0, CSIDL_PROFILE | CSIDL_FLAG_CREATE, 0, 0, buffer) == S_OK)
      {
        path = QString(buffer);
      }
      else
      {
        error("PlatformUtils::getUserHomePath # SHGetFolderPath() failed");
      }

      return path;
    }

    QString getModuleGlobalDataPath(const char * module, bool isapplication)
    {
      (void) isapplication;

      assert(strlen(module) < 128);

      // Typically this retrieves "C:\Documents and Settings\All Users\Application Data"
      // which by most accounts is the safest place to store application data

      QString   path;

      char  buffer[_MAX_PATH] = "";
      if(SHGetFolderPathA(0, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, 0, 0, buffer) == S_OK)
      {
        path = QString(buffer) + QString("\\") + QString(module);
      }
      else
      {
        error("PlatformUtils::getModuleGlobalDataPath # SHGetFolderPath() failed");
      }

      return path;
    }

    QString getModuleUserDataPath(const char * module, bool isapplication)
    {
      (void) isapplication;

      assert(strlen(module) < 128);

      // Typically this retrieves "C:\Documents and Settings\(username)\Application Data"

      QString   path;

      char  buffer[_MAX_PATH] = "";
      if(SHGetFolderPathA(0, CSIDL_APPDATA | CSIDL_FLAG_CREATE, 0, 0, buffer) == S_OK)
      {
        path = QString(buffer) + QString("\\") + QString(module);
      }
      else
      {
        error("PlatformUtils::getModuleUserDataPath # SHGetFolderPath() failed");
      }

      return path;
    }

    void * openPlugin(const char * path)
    {
      return (void *)(LoadLibraryA(path));
    }

    uint64_t processMemoryUsage()
    {
      HANDLE hProcess;
      PROCESS_MEMORY_COUNTERS pmc;

      hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
                             PROCESS_VM_READ, FALSE, _getpid() );
      if(!hProcess)
        return 0;

      if (!GetProcessMemoryInfo( hProcess, &pmc, sizeof(pmc)) )
      {
        error("PlatformUtils::processMemoryUsage # GetProcessMemoryInfo failed");
	      return 0;
      }

      CloseHandle( hProcess );

      return pmc.WorkingSetSize;
    }

  }

}

#endif
