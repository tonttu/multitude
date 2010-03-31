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
#include "Trace.hpp"

#include <dlfcn.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <sys/resource.h>

namespace Radiant
{

  namespace PlatformUtils
  {

    std::string getExecutablePath()
    {
      char buf[512];
      int len;

      len = readlink("/proc/self/exe", buf, 512);

      if(len == -1) {
        Radiant::error("PlatformUtils::getExecutablePath # readlink() failed");
        return std::string("");
      }

      return std::string(buf, len);
    }

    std::string getUserHomePath()
    {
      return std::string(getenv("HOME"));
    }

    std::string getModuleGlobalDataPath(const char * module, bool isapplication)
    {
      (void) isapplication;

      assert(strlen(module) < 128);
      char buf[312];
      sprintf(buf, "/usr/share/%s", module);

      return buf;
    }

    std::string getModuleUserDataPath(const char * module, bool isapplication)
    {
      (void) isapplication;

      assert(strlen(module) < 128);
      char buf[312];

      sprintf(buf, "%s/.%s", getUserHomePath().c_str(), module);

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

    void setEnv(const char * name, const char * value)
    {
      if(value)
        setenv(name, value, 1);
      else
        unsetenv(name);
    }
  }

}
