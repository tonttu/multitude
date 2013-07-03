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

#include <QSettings>
#include <QProcess>

#include <assert.h>

#include <shlobj.h>
#include <shlwapi.h>

#include <psapi.h>
#include <process.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <Windows.h>

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

      QSettings settings("SOFTWARE\\MultiTouch\\MTSvc", QSettings::NativeFormat);
      QString root = settings.value("Root").toString();
      if(!root.isEmpty())
        return root + QString("\\share");

      assert(false && "PlatformUtils::GetModuleGlobalDataPath # Root in SOFTWARE\\MultiTouch\\MTSvc not set");
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

    bool fileReadable(const char * filename)
    {
      struct _stat  buf;
      memset(& buf, 0, sizeof(struct _stat));

      return ((_stat(filename, & buf) != -1) && (buf.st_mode & _S_IREAD));
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

    void setEnv(const char * name, const char * value)
    {
      SetEnvironmentVariableA((char *) name, (char *) value);
    }

    void openFirewallPortTCP(int port, const QString & name)
    {
      QByteArray argv0(512, '\0');
      GetModuleFileNameA(0, argv0.data(), argv0.size());
      QString nameRule = QString("name=%1").arg(name);
      QString progRule = QString("program=%1").arg(argv0.data());
      QString portRule = QString("localport=%1").arg(port);

      QProcess::execute("netsh", QStringList() << "advfirewall" << "firewall" <<
                        "delete" << "rule" << nameRule << "dir=in" << "profile=any" <<
                        progRule << "protocol=tcp");

      QProcess::execute("netsh", QStringList() << "advfirewall" << "firewall" <<
                        "add" << "rule" << nameRule << "dir=in" << "action=allow" <<
                        progRule << "profile=any" << portRule << "protocol=tcp" <<
                        "interfacetype=lan");
    }

    bool reboot()
    {
      HANDLE token;
      if (!OpenProcessToken(GetCurrentProcess(),
                            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
        throw QString("OpenProcessToken: %1").arg(Radiant::StringUtils::getLastErrorMessage());

      TOKEN_PRIVILEGES tkp;
      tkp.PrivilegeCount = 1;
      tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
      if (!LookupPrivilegeValue(0, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid))
        throw QString("LookupPrivilegeValue: %1").arg(Radiant::StringUtils::getLastErrorMessage());

      if (!AdjustTokenPrivileges(token, FALSE, &tkp, 0, 0, 0))
        throw QString("AdjustTokenPrivileges: %1").arg(Radiant::StringUtils::getLastErrorMessage());

      const DWORD reason = SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_FLAG_PLANNED;
      const DWORD timeout = 30;
      if (InitiateSystemShutdownExA(NULL, "Rebooting due to received AMX command",
                               timeout, TRUE, TRUE, reason) != 0) {
        return true;
      } else {
        throw QString("InitiateSystemShutdownExA: %1").arg(Radiant::StringUtils::getLastErrorMessage());
      }
    }
  }

}

#endif
