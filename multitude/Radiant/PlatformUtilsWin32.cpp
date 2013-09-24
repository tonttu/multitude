/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
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

#include <vector>

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
      // Typically this retrieves "C:\Users\(username)"

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

    QString getUserDocumentsPath()
    {
      return getUserHomePath() + "\\My Documents";
    }

    QString getModuleUserDataPath(const char * module, bool isapplication)
    {
      (void) isapplication;

      assert(strlen(module) < 128);

      // Typically this retrieves "C:\Users\(username)\Application Data"

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

    QString getLibraryPath(const QString& libraryName)
    {
      auto wLibraryName = libraryName.toStdWString();

      HMODULE handle = GetModuleHandle(wLibraryName.data());
      if(handle == NULL) {
        Radiant::error("getLibraryPath # failed for '%s'", libraryName.toUtf8().data());
        return QString();
      }

      std::vector<wchar_t> buffer(MAX_PATH);

      DWORD got = GetModuleFileName(handle, &buffer[0], buffer.size());

      while(got == buffer.size()) {

        buffer.resize(2 * buffer.size());

        got = GetModuleFileName(handle, &buffer[0], buffer.size());
      }

      if(got == 0) {
        Radiant::error("getLibraryPath # failed to get path for '%s'", libraryName.toUtf8().data());
        return QString();
      }

      return QString::fromStdWString(std::wstring(buffer.begin(), buffer.begin() + got));
    }

  }

}

#endif
