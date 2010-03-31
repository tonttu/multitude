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
#include "PlatformUtils.hpp"
#include "StringUtils.hpp"
#include "Trace.hpp"

#include <assert.h>

// #include <windows.h>
// #include <winbase.h>
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

    std::string getExecutablePath()
    {
      // Get the full exe path / fileneme

      std::string   path;

      char  buffer[_MAX_PATH] = "";
      if(GetModuleFileNameA(0, buffer, _MAX_PATH) != 0)
      {
         // remove the filename part
         PathRemoveFileSpecA(buffer);
         path = std::string(buffer);
      }
      else
      {
        error("PlatformUtils::getExecutablePath # GetModuleFileName() failed");
      }

      return path;
    }

    std::string getUserHomePath()
    {
      // Typically this retrieves "C:\Documents and Settings\(username)"

      std::string   path;

      char  buffer[_MAX_PATH] = "";
      if(SHGetFolderPathA(0, CSIDL_PROFILE | CSIDL_FLAG_CREATE, 0, 0, buffer) == S_OK)
      {
        path = std::string(buffer);
      }
      else
      {
        error("PlatformUtils::getUserHomePath # SHGetFolderPath() failed");
      }

      return path;
    }

    std::string getModuleGlobalDataPath(const char * module, bool isapplication)
    {
      (void) isapplication;

      assert(strlen(module) < 128);

      // Typically this retrieves "C:\Documents and Settings\All Users\Application Data"
      // which by most accounts is the safest place to store application data

      std::string   path;

      char  buffer[_MAX_PATH] = "";
      if(SHGetFolderPathA(0, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, 0, 0, buffer) == S_OK)
      {
        path = std::string(buffer) + std::string("\\") + std::string(module);
      }
      else
      {
        error("PlatformUtils::getModuleGlobalDataPath # SHGetFolderPath() failed");
      }

      return path;
    }

    std::string getModuleUserDataPath(const char * module, bool isapplication)
    {
      (void) isapplication;

      assert(strlen(module) < 128);

      // Typically this retrieves "C:\Documents and Settings\(username)\Application Data"

      std::string   path;

      char  buffer[_MAX_PATH] = "";
      if(SHGetFolderPathA(0, CSIDL_APPDATA | CSIDL_FLAG_CREATE, 0, 0, buffer) == S_OK)
      {
        path = std::string(buffer) + std::string("\\") + std::string(module);
      }
      else
      {
        error("PlatformUtils::getModuleUserDataPath # SHGetFolderPath() failed");
      }

      return path;
    }

    bool fileReadable(const char * filename)
    {
      struct _stat  buf;
      memset(& buf, 0, sizeof(struct _stat));

      return ((_stat(filename, & buf) != -1) && (buf.st_mode & _S_IREAD));
    }

    void * openPlugin(const char * path)
    {
      const std::wstring  wp = StringUtils::stdStringToStdWstring(std::string(path));

      return (void *)(LoadLibrary(wp.data()));
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

    void setEnv(const char * name, const char * value)
    {
      SetEnvironmentVariableA((char *) name, (char *) value);
    }

  }


}
