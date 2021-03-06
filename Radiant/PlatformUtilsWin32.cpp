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

#include <QSettings>
#include <QProcess>
#include <QDir>
#include <QStandardPaths>

#include <assert.h>

#include <shlobj.h>
#include <shlwapi.h>

#include <psapi.h>
#include <process.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <Windows.h>
#include <Shellapi.h>

#include <stdlib.h>
#include <vector>

#include <QVector>
#include <QSet>

namespace {

  bool systemShutdown(bool rebootAfterShutdown)
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
    if (InitiateSystemShutdownExW(NULL, NULL,
                                  timeout, TRUE, rebootAfterShutdown, reason) != 0) {
      return true;
    } else {
      throw QString("InitiateSystemShutdownExW: %1").arg(Radiant::StringUtils::getLastErrorMessage());
    }
  }

}

namespace Radiant
{

  namespace PlatformUtils
  {

    QString getExecutablePath()
    {
      // Get the full exe path / fileneme

      QString   path;

      WCHAR buffer[_MAX_PATH] = L"";
      if(GetModuleFileNameW(0, buffer, _MAX_PATH) != 0)
      {
         // remove the filename part
         PathRemoveFileSpecW(buffer);
         path = QString::fromWCharArray(buffer);
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

      PWSTR buffer = NULL;
      if(SHGetKnownFolderPath(FOLDERID_Profile, KF_FLAG_CREATE, 0, &buffer) == S_OK)
      {
        path = QString::fromWCharArray(buffer);
      }
      else
      {
        error("PlatformUtils::getUserHomePath # SHGetKnowFolderPath() failed");
      }

      CoTaskMemFree(buffer);

      return path;
    }

    QString getUserDocumentsPath()
    {
      return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    QString getModuleUserDataPath(const char * module, bool isapplication[[maybe_unused]])
    {
      assert(strlen(module) < 128);

      // Typically this retrieves "C:\Users\(username)\Application Data"

      QString   path;

      PWSTR ppszPath = NULL;
      if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, 0, &ppszPath) == S_OK)
      {
        path = QString::fromWCharArray(ppszPath) + QString("\\") + QString(module);
      }
      else
      {
        error("PlatformUtils::getModuleUserDataPath # SHGetKnownFolderPath() failed");
      }

      CoTaskMemFree(ppszPath);

      return path;
    }

    QString localAppPath()
    {
      QString ret;
      PWSTR path = nullptr;
      if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &path) == S_OK) {
        ret = QString::fromWCharArray(path).replace('\\', '/');
      } else {
        error("PlatformUtils::localAppPath # SHGetKnownFolderPath() failed");
      }

      CoTaskMemFree(path);

      return ret;
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

    MemInfo memInfo()
    {
      MemInfo info;

      MEMORYSTATUSEX status;
      status.dwLength = sizeof(status);

      if (GlobalMemoryStatusEx(&status)) {
        info.memTotalKB = status.ullTotalPhys / 1024;
        info.memAvailableKB = status.ullAvailPhys / 1024;
      } else {
        error("PlatformUtils::memInfo # GlobalMemoryStatusEx failed: %s",
              StringUtils::getLastErrorMessage().toUtf8().data());
      }

      return info;
    }

    QString libraryFilePath()
    {
      HMODULE module = nullptr;
      if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             (LPCSTR)&libraryFilePath, &module) == 0) {
        Radiant::error("Radiant::PlatformUtils::libraryFilePath # GetModuleHandleExA failed: %s",
                       StringUtils::getLastErrorMessage().toUtf8().data());
        return QString();
      }

      wchar_t name[MAX_PATH];
      if (GetModuleFileNameW(module, name, MAX_PATH) == 0) {
        Radiant::error("Radiant::PlatformUtils::libraryFilePath # GetModuleFileNameW failed: %s",
                       StringUtils::getLastErrorMessage().toUtf8().data());
        return QString();
      }
      return QString::fromWCharArray(name);
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

      DWORD got = GetModuleFileName(handle, &buffer[0], static_cast<DWORD>(buffer.size()));

      while(got == buffer.size()) {

        buffer.resize(2 * buffer.size());

        got = GetModuleFileName(handle, &buffer[0], static_cast<DWORD>(buffer.size()));
      }

      if(got == 0) {
        Radiant::error("getLibraryPath # failed to get path for '%s'", libraryName.toUtf8().data());
        return QString();
      }

      return QString::fromStdWString(std::wstring(buffer.begin(), buffer.begin() + got));
    }

    void setEnv(const QString & name, const QString & value)
    {
      int err = _putenv_s(name.toUtf8().data(), value.toUtf8().data());
      if (err != 0) {
        Radiant::error("PlatformUtils::setEnv # Failed to set environment variable %s: error %d",
                       name.toUtf8().data(), err);
      }
    }

    bool createHardLink(const QString & from, const QString & to)
    {
      (void) from;
      (void) to;
      Radiant::error("Hard links not implemented on windows yet");
      return false;
    }

    int numberOfHardLinks(const QString & file)
    {
      (void) file;
      Radiant::error("numberOfHardLinks not implemented on windows yet");
      return -1;
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
      return systemShutdown(true);
    }

    bool shutdown()
    {
      return systemShutdown(false);
    }

    void terminateProcessByName(const QString& processName)
    {
      const auto cmd = QString("tskill %1").arg(processName);
      int err = system(cmd.toUtf8().data());
      if(err != 0)
        Radiant::warning(QString("terminateProcessByName # failed to run '%1'").arg(cmd).toUtf8().data());
    }

    QString windowsProgramDataPath()
    {
      PWSTR path;
      HRESULT res = SHGetKnownFolderPath(FOLDERID_ProgramData, KF_FLAG_CREATE, nullptr, &path);
      if(res != S_OK) {
        CoTaskMemFree(path);
        Radiant::error("Failed to get ProgramData path. ShGetKnownFolderPath failed with error code %d", res);

        return QString();
      }

      QString programData = QString::fromWCharArray(path);
      CoTaskMemFree(path);
      return programData;
    }

    static QString wantLogDir()
    {
      return QString("%1\\MultiTouch\\Logs").arg(Radiant::PlatformUtils::windowsProgramDataPath());
    }

    static QString newWindowsLogDir()
    {
      QString logPath = wantLogDir();
      // If creating the log folder fails for whatever reason, log to TEMP instead
      bool logPathOk = QDir().mkpath(logPath);
      if(!logPathOk)
        logPath = QDir::tempPath();
      return logPath;
    }

    QString newWindowsServiceLogFile(const QString & serviceName, const QString & logName, int iteration)
    {
      QString dir = newWindowsLogDir();
      bool rotate = iteration >= 0;
      if(rotate) {
        return QString("%1\\%2-%3-%4.log").arg(dir, serviceName, logName).arg(iteration % 10);
      } else {
        return QString("%1\\%2-%3.log").arg(dir, serviceName, logName);
      }
    }

    QString findWindowsServiceLogFile(const QString & serviceName, const QString & logName)
    {
      std::vector<QDir> dirs;
      dirs.push_back(QDir(wantLogDir()));
      dirs.push_back(QDir::temp());

      for(QDir & dir : dirs) {
        if(!dir.exists()) {
          continue;
        }
        dir.setNameFilters(QStringList() << QString("%1-%2*.log").arg(serviceName, logName));
        dir.setSorting(QDir::Time | QDir::Reversed);
        dir.setFilter(QDir::Files | QDir::Readable);
        QFileInfoList entries = dir.entryInfoList();
        if(!entries.empty()) {
          QFileInfo & log = entries.front();
          return log.absoluteFilePath();
        }
      }
      return QString();
    }

    ConsoleType setupConsole()
    {
      // Standard handles to check for redirection
      const QVector<DWORD> handleTypes { STD_OUTPUT_HANDLE, STD_ERROR_HANDLE, STD_INPUT_HANDLE };
      // File types that prevent us from attaching to parent process console
      const QSet<int> fileTypes { FILE_TYPE_DISK, FILE_TYPE_PIPE };

      for(const auto handleType : handleTypes) {
        const HANDLE handle = GetStdHandle(handleType);
        const auto type = GetFileType(handle);

        // If any of the standard handles have been redirected, we can't attach
        // to parent process console because it would break the redirect. The
        // downside of this is that if you only redirect, say stderr, you will
        // not see any stdout in the parent process console.
        if(fileTypes.contains(type))
          return ConsoleType::Redirected;
      }

      // No redirection in place, try to attach to parent process console
      const bool attachOk = (::AttachConsole(ATTACH_PARENT_PROCESS) != 0);
      if(attachOk) {
        // Attachment was successful, reopen the standard handles so they point to
        // the parent process
        freopen("CON", "wt", stdout);
        freopen("CON", "wt", stderr);
        freopen("CON", "rt", stdin);

        return ConsoleType::AttachedToParentProcess;
      }
      // Attachment failed. Either this process already has a console (qmake
      // CONFIG += console flag) or the parent process does not have a console
      // (e.g. started by double-clicking the executable).

      return ConsoleType::Unknown;
    }

    QStringList getCommandLine()
    {
      QStringList result;

      int argc = 0;
      auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);

      if (argv) {
        for (int i = 0; i < argc; ++i) {
          result << QString::fromWCharArray(argv[i]);
        }

        LocalFree(argv);
      } else {
        WCHAR name[256];
        GetModuleFileNameW(nullptr, name, sizeof(name));
        result << QString::fromWCharArray(name);
      }

      return result;
    }

    void setCpuAffinity(const std::vector<int> & cpuList)
    {
      DWORD_PTR mask = 0;
      for (int cpu: cpuList)
        mask |= (DWORD_PTR(1) << cpu);
      if (SetThreadAffinityMask(GetCurrentThread(), mask) == 0) {
        Radiant::error("setCpuAffinity # SetThreadAffinityMask: %s",
                       Radiant::StringUtils::getLastErrorMessage().toUtf8().data());
      }
    }
  }  // namespace PlatformUtils
}  // namespace Radiant

#endif
