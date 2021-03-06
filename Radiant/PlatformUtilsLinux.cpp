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

#ifdef RADIANT_LINUX

#include "PlatformUtils.hpp"
#include "Trace.hpp"
#include "FileUtils.hpp"

#include <dlfcn.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sched.h>

#include <sys/time.h>
#include <sys/resource.h>

#include <QFileInfo>
#include <QStringList>
#include <QProcess>
#include <QCoreApplication>
#include <QTemporaryFile>

namespace
{
#ifndef RADIANT_MOBILE
  // Utility similar to system()
  int run(QString cmd, QStringList argv = QStringList(),
          QByteArray * out = 0, QByteArray * err = 0)
  {
    return Radiant::FileUtils::run(cmd, argv, out, err);
  }
#endif

  uint64_t toKB(uint64_t value, const QByteArray & unit)
  {
    if (unit == "kb") {
      return value;
    } else if (unit == "mb") {
      return value * 1024;
    } else if (unit == "gb") {
      return value * 1024 * 1024;
    } else if (unit == "tb") {
      return value * 1024 * 1024 * 1024;
    } else if (unit.isEmpty()) {
      return value / 1024;
    } else {
      Radiant::error("toKB # Unknown unit '%s'", unit.data());
      return 0;
    }
  }
}

namespace Radiant
{

  namespace PlatformUtils
  {

    QString getExecutablePath()
    {
      char buf[512];
      int len;

      len = readlink("/proc/self/exe", buf, 512);

      if(len == -1) {
        Radiant::error("PlatformUtils::getExecutablePath # readlink() failed");
        return QString("");
      }

      return QString::fromUtf8(buf, len);
    }

    int getProcessId()
    {
      return getpid();
    }
	
    QString getUserHomePath()
    {
      return QString::fromUtf8(getenv("HOME"));
    }

    QString getUserDocumentsPath()
    {
      return getUserHomePath() + "/Documents";
    }

    QString getModuleUserDataPath(const char * module, bool isapplication)
    {
      (void) isapplication;

      return QString("%1/.%2").arg(getUserHomePath(), module);
    }

    QString localAppPath()
    {
      QString home = getUserHomePath();
      while (!home.isEmpty() && home.endsWith('/'))
        home.chop(1);
      return home;
    }

    void * openPlugin(const char * path)
    {
      return dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    }

    uint64_t processMemoryUsage()
    {
      /* Contents of the statm files (as of 2.6.8-rc3)
         size     total program size (pages)      (same as VmSize in status)
         resident size of memory portions (pages) (same as VmRSS in status)
         shared   number of pages that are shared (i.e. backed by a file)
         trs      number of pages that are 'code' (not including libs; broken,
                                                   includes data segment)
         lrs      number of pages of library      (always 0 on 2.6)
         drs      number of pages of data/stack   (including libs; broken,
                                                   includes library text)
         dt       number of dirty pages           (always 0 on 2.6) */
      static uint64_t pagesize = 0;
      if(pagesize == 0) {
        pagesize = (uint64_t)sysconf(_SC_PAGESIZE);
      }

      FILE * f = fopen("/proc/self/statm", "r");
      unsigned long vmrss = 0u;
      if(f) {
        if(fscanf(f, "%*u %lu", &vmrss) != 1) vmrss = 0u;
        fclose(f);
      }
      return uint64_t(vmrss) * pagesize;
    }

    MemInfo memInfo()
    {
      MemInfo info;

      QFile file("/proc/meminfo");
      if (file.open(QFile::ReadOnly)) {
        QRegExp m("(MemTotal|MemAvailable|MemFree|Cached):\\s*(\\d+) (.*)");
        bool foundMemAvailable = false;
        uint64_t memFree = 0;
        uint64_t cached = 0;

        // atEnd doesn't work with /proc/meminfo, it always returns true
        while (true) {
          const QString line = QString::fromUtf8(file.readLine().trimmed());
          if (line.isEmpty()) break;

          if (m.exactMatch(line)) {
            if (m.cap(1) == "MemTotal") {
              info.memTotalKB = toKB(m.cap(2).toULongLong(), m.cap(3).toLower().toUtf8());
            } else if (m.cap(1) == "MemAvailable") {
              info.memAvailableKB = toKB(m.cap(2).toULongLong(), m.cap(3).toLower().toUtf8());
              foundMemAvailable = true;
            } else if (!foundMemAvailable && m.cap(1) == "MemFree") {
              memFree = toKB(m.cap(2).toULongLong(), m.cap(3).toLower().toUtf8());
            } else if (!foundMemAvailable && m.cap(1) == "Cached") {
              cached = toKB(m.cap(2).toULongLong(), m.cap(3).toLower().toUtf8());
            }
          }
        }

        // We have too old kernel, estimate the available memory. This is not
        // correct, but comes pretty close, see
        // https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=34e431b0ae398fc54ea69ff85ec700722c9da773
        if (!foundMemAvailable) {
          info.memAvailableKB = memFree + cached;
        }
        if (info.memTotalKB == 0) {
          Radiant::warning("PlatformUtils::memInfo # Failed to find the total amount of physical RAM");
        }
      } else {
        Radiant::warning("PlatformUtils::memInfo # Failed to open /proc/meminfo: %s",
                         file.errorString().toUtf8().data());
      }

      return info;
    }

    QString libraryFilePath()
    {
      Dl_info info;
      if (dladdr((const void*)libraryFilePath, &info))
        return QFileInfo(info.dli_fname).absoluteFilePath();
      return QString();
    }

#ifndef RADIANT_MOBILE
    QString getLibraryPath(const QString& libraryName)
    {
      auto pid = QCoreApplication::applicationPid();

      QTemporaryFile file;
      if(!file.open()) {
        Radiant::error("getLibraryPath # failed to create temporary file '%s'", file.fileName().toUtf8().data());
        return QString();
      }

      const QString cmd = QString("grep %1 /proc/%2/maps | awk '{print $6}'| head -n1 > %3").arg(libraryName).arg(pid).arg(file.fileName());

      if (FileUtils::runInShell(cmd) != 0)
        Radiant::error("PlatformUtils::getLibraryPath # Failed to get library path for %s", libraryName.toUtf8().data());

      return file.readAll().trimmed();
    }
    
    void openFirewallPortTCP(int /*port*/, const QString & /*name*/)
    {
    }

    bool reboot()
    {
      if(geteuid() == 0) {
        return run("reboot") == 0;
      } else {
        return run("sudo", (QStringList() << "-n" << "--" << "reboot")) == 0;
      }
    }

    bool shutdown()
    {
      if(geteuid() == 0) {
        return run("shutdown", (QStringList() << "-h" << "-P" << "now")) == 0;
      } else {
        return run("sudo", (QStringList() << "-n" << "--" << "shutdown" << "-h" << "-P" << "now")) == 0;
      }
    }

    void terminateProcessByName(const QString& processName)
    {
      const auto cmd = QString("killall %1").arg(processName);

      int err = FileUtils::runInShell(cmd);
      if(err != 0)
        Radiant::warning("terminateProcessByName # failed to run '%s'", cmd.toUtf8().data());
    }
#endif

    void setEnv(const QString & name, const QString & value)
    {
      if (setenv(name.toUtf8().data(), value.toUtf8().data(), 1) != 0) {
        int e = errno;
        Radiant::error("PlatformUtils::setEnv # Failed to set environment variable %s: %s",
                       name.toUtf8().data(), strerror(e));
      }
    }

    bool createHardLink(const QString & from, const QString & to)
    {
      QByteArray fromCp = from.toUtf8();
      QByteArray toCp = to.toUtf8();
      int res = link(fromCp.data(), toCp.data());
      if(res < 0) {
        Radiant::error("PlatformUtils::createHardLink # Failed to create hard link from '%s' to '%s': %s",
                       fromCp.data(), toCp.data(), strerror(errno));
      }
      return res == 0;
    }

    int numberOfHardLinks(const QString & file)
    {
      struct stat result;
      if(stat(file.toUtf8().data(), &result) < 0) {
        Radiant::error("PlatformUtils::numberOfHardLinks # Failed to stat file '%s': %s",
                       file.toUtf8().data(), strerror(errno));
        return -1;
      }
      return result.st_nlink;
    }

    QStringList getCommandLine()
    {
      QFile cmdline("/proc/self/cmdline");

      if(!cmdline.open(QIODevice::ReadOnly)) {
        Radiant::error("PlatformUtils::getCommandLine # failed to read %s", cmdline.fileName().toUtf8().data());
        return QStringList();
      }

      QList<QByteArray> args = cmdline.readAll().split('\0');

      QStringList result;
      for(const auto & i : args)
        result << i;

      return result;
    }

    void setCpuAffinity(const std::vector<int> & cpuList)
    {
      cpu_set_t mask;
      CPU_ZERO(&mask);
      for (int cpu: cpuList) {
        CPU_SET(cpu, &mask);
      }
      if (sched_setaffinity(0, sizeof(mask), &mask) != 0) {
        Radiant::error("setCpuAffinity # sched_setaffinity failed: %s", strerror(errno));
      }
    }
  }
}

#endif
