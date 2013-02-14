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

#ifdef RADIANT_LINUX

#include "PlatformUtils.hpp"
#include "Trace.hpp"

#include <dlfcn.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/resource.h>

#include <QCoreApplication>
#include <QTemporaryFile>

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

    QString getModuleGlobalDataPath(const char * module, bool isapplication)
    {
      (void) isapplication;

      assert(strlen(module) < 128);
      char buf[312];
      sprintf(buf, "/usr/share/%s", module);

      return buf;
    }

    QString getModuleUserDataPath(const char * module, bool isapplication)
    {
      (void) isapplication;

      assert(strlen(module) < 128);
      char buf[312];

      sprintf(buf, "%s/.%s", getUserHomePath().toUtf8().data(), module);

      return buf;
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

    QString getLibraryPath(const QString& libraryName)
    {
      auto pid = QCoreApplication::applicationPid();

      QTemporaryFile file;
      if(!file.open()) {
        Radiant::error("getLibraryPath # failed to create temporary file '%s'", file.fileName().toUtf8().data());
        return QString();
      }

      const QString cmd = QString("grep %1 /proc/%2/maps | awk '{print $6}'|sort|uniq > %3").arg(libraryName).arg(pid).arg(file.fileName());

      system(cmd.toUtf8().data());

      return file.readAll().trimmed();
    }

  }

}

#endif
