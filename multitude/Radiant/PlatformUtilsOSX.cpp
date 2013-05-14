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

#ifdef RADIANT_OSX

#include "PlatformUtils.hpp"

#include "Platform.hpp"
#include "Trace.hpp"

#include <dlfcn.h>

#include <mach/task.h>
#include <mach/mach_traps.h>
#include <mach/mach.h>

#include <assert.h>

#ifndef RADIANT_IOS
# include <CoreFoundation/CoreFoundation.h>
#endif

#include <QCoreApplication>
#include <QTemporaryFile>

namespace Radiant
{

  namespace PlatformUtils
  {

#ifndef RADIANT_IOS
    QString getExecutablePath()
    {
      CFURLRef url = CFBundleCopyBundleURL(CFBundleGetMainBundle());

      char buf[512];
      buf[0] = '\0';

      CFURLGetFileSystemRepresentation(url, true, (UInt8*) buf, 512);

      return buf;
    }
#endif

    /// Returns the current process identifier
    int getProcessId()
    {
      return getpid();
    }

    QString getUserHomePath()
    {
      return QString(getenv("HOME"));
    }

    QString getModuleUserDataPath(const char * module, bool isapplication)
    {
      (void) isapplication;

      assert(strlen(module) < 128);
      char buf[312];

      sprintf(buf, "%s/Library/%s", getUserHomePath().toUtf8().data(), module);

      return buf;
    }

    void * openPlugin(const char * path)
    {
      return dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    }

    uint64_t processMemoryUsage()
    {
      struct task_basic_info t_info;
      mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

      if (KERN_SUCCESS != task_info(mach_task_self(),
                                    TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count))
      {
        return -1;
      }
      return t_info.resident_size;

    }

    QString getLibraryPath(const QString& libraryName)
    {
      auto pid = QCoreApplication::applicationPid();

      QTemporaryFile file;
      if(!file.open()) {
        Radiant::error("getLibraryPath # failed to create temporary file '%s'", file.fileName().toUtf8().data());
        return QString();
      }

      const QString cmd = QString("vmmap %2 | grep %1 | awk '{print $7}' | head -n1 > %3").arg(libraryName).arg(pid).arg(file.fileName());

      system(cmd.toUtf8().data());

      return file.readAll().trimmed();
    }

  }

}

#endif
