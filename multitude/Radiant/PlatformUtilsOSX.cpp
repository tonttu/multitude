/* COPYRIGHT
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
#else
    QString getExecutablePath()
    {
      return QString();
    }
#endif

    QString getUserHomePath()
    {
      return QString(getenv("HOME"));
    }

    QString getModuleGlobalDataPath(const char * module, bool isapplication)
    {
      assert(strlen(module) < 128);
      char buf[312];

      if(isapplication) {
        sprintf(buf, "/Applications/%s.app/Contents/Resources", module);
      }
      else {
        sprintf(buf, "/Library/Frameworks/%s.framework", module);
      }
      return buf;
    }

    QString getModuleUserDataPath(const char * module, bool isapplication)
    {
      (void) isapplication;

      assert(strlen(module) < 128);
      char buf[312];

      sprintf(buf, "%s/Library/%s", getUserHomePath().toUtf8().data(), module);

      return buf;
    }

    bool fileReadable(const char * filename)
    {
      FILE * f = fopen(filename, "r");
      if(!f)
        return false;
      fclose(f);
      return true;
    }

    void * openPlugin(const char * path)
    {
      return dlopen(path, RTLD_NOW | RTLD_GLOBAL);
    }

    uint64_t processMemoryUsage()
    {
      // task_t task = MACH_PORT_NULL;
      struct task_basic_info t_info;
      mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

      if (KERN_SUCCESS != task_info(mach_task_self(),
                                    TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count))
      {
        return -1;
      }
      return t_info.resident_size;
      // t_info.virtual_size;

    }

    void setEnv(const char * name, const char * value)
    {
      if(value)
        setenv(name, value, 1);
      else
        unsetenv(name);
    }
  }

}

#endif
