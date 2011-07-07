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

#include "MemCheck.hpp"

#ifdef MULTI_MEMCHECK

#include "Mutex.hpp"
#include "Trace.hpp"
#include <typeinfo>

#ifndef __GNUC__

#include <set>

#else // __GNUC__

#include "StringUtils.hpp"

#include <dlfcn.h>
#include <execinfo.h>
#include <cxxabi.h>
#include <map>
#include <strings.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <cassert>

#include <QRegExp>

#endif


namespace Radiant {
  static Mutex s_mutex;
  static long s_total = 0;

#ifndef __GNUC__

  typedef std::set<MemCheck*> MemSet;
  static MemSet s_set;

  MemCheck::MemCheck()
  {
    Guard g(s_mutex);
    s_set.insert(this);
    ++s_total;
  }

  MemCheck::MemCheck(const MemCheck &)
  {
    Guard g(s_mutex);
    s_set.insert(this);
    ++s_total;
  }

  MemCheck & MemCheck::operator=(const MemCheck &)
  {
    Guard g(s_mutex);
    s_set.insert(this);
    ++s_total;

    return *this;
  }

  MemCheck::~MemCheck()
  {
    Guard g(s_mutex);

    MemSet::iterator it = s_set.find(this);
    if(it == s_set.end()) {
      std::cerr << "~MemCheck: Couldn't find object " << this << std::endl;
    } else {
      s_set.erase(it);
    }
  }

  class Checker {
  public:
    virtual ~Checker()
    {
      Guard g(s_mutex);
      if(s_set.empty()) {
        info("All %d MemCheck objects were released", s_total);
      } else {
        info("%d of %d MemCheck objects were not released", s_map.size(), s_total);
        std::map<std::string, int> map;
        for(MemSet::iterator it = s_set.begin(); it != s_set.end(); ++it)
          ++map[typeid(**it).name()];
        for(std::map<std::string, int>::iterator it = map.begin(); it != map.end(); ++it) {
          error("%d %s objects were not released", it->second, it->first);
        }
      }
    }
  };
#else // __GNUC__

  typedef std::map<MemCheck*, std::pair<void **, size_t> > MemMap;
  static MemMap s_map;

  inline std::string file_and_line(const QString & file, long ptr)
  {
    assert(sizeof(long) == sizeof(void*));
    char buffer[512];

    // Using QProcess might be a little dangerous, since we might be outside main() already
    FILE * f = popen(QString("addr2line -pie \"%1\" %2").arg(file).arg(ptr, 0, 16).toUtf8().data(), "r");
    if(f && fgets(buffer, sizeof(buffer), f)) {
      fclose(f);
      f = 0;
      int l = strlen(buffer);
      if(l > 1) {
        // remove newline
        if(buffer[l-1] == '\n') buffer[l-1] = '\0';
        if(std::string("??:0") != buffer)
          return buffer;
      }
    }

    if(f) fclose(f);
    return "";
  }

  inline void print_bt(void ** data, size_t size)
  {
    char ** strings = backtrace_symbols(data, size);

    /// strings[i] can be something like:
    /// /home/riku/cornerstone/multitude/lib/libRadiant.so.1(_ZN7Radiant8MemCheckC2Ev+0x40) [0x7fe3fd420c64]
    /// ./LaunchStack() [0x420669]
    //        binary     (    function(opt)           +offset(opt)      )
    QRegExp r("(.*)"  "\\("       "([^+]*)"  "(?:\\+0x[0-9a-f]+)?"   "\\).*");

    int status;
    for(size_t i = 0; i < size; i++) {
      if(r.exactMatch(QString::fromUtf8(strings[i]))) {
        std::string func = Radiant::StringUtils::demangle(r.cap(2).toUtf8().data());
        std::string file;

        Dl_info info;
        info.dli_fbase = 0;
        dladdr(data[i], &info);
        if(info.dli_fbase) {
          file = file_and_line(r.cap(1), (long)data[i] - (long)info.dli_fbase);
        }

        if(file.empty())
          file = file_and_line(r.cap(1), (long)data[i]);
        if(file.empty())
          file = r.cap(1).toUtf8().data();

        if(func.empty())
          fprintf(stderr, "#%-2d %p at %s\n", int(i), data[i], file.c_str());
        else
          fprintf(stderr, "#%-2d %s at %s\n", int(i), func.c_str(), file.c_str());
      } else {
        fprintf(stderr, "#%-2d %s\n", int(i), strings[i]);
      }
    }
    free (strings);
  }

  MemCheck::MemCheck()
  {
    void ** bt = new void*[50];
    size_t size = backtrace(bt, 50);

    Guard g(s_mutex);
    s_map[this] = std::make_pair(bt, size);
    ++s_total;
  }

  MemCheck::MemCheck(const MemCheck &)
  {
    void ** bt = new void*[50];
    size_t size = backtrace(bt, 50);

    Guard g(s_mutex);
    s_map[this] = std::make_pair(bt, size);
    ++s_total;
  }

  MemCheck & MemCheck::operator=(const MemCheck &)
  {
    void ** bt = new void*[50];
    size_t size = backtrace(bt, 50);

    Guard g(s_mutex);
    s_map[this] = std::make_pair(bt, size);
    ++s_total;

    return *this;
  }

  MemCheck::~MemCheck()
  {
    Guard g(s_mutex);
    MemMap::iterator it = s_map.find(this);
    if(it == s_map.end()) {
      error("~MemCheck: Couldn't find object %p", this);
      void ** bt = new void*[50];
      size_t size = backtrace(bt, 50);
      print_bt(bt, size);
      delete[] bt;
    } else {
      delete[] it->second.first;
      s_map.erase(it);
    }
  }

  class Checker {
  public:
    virtual ~Checker()
    {
      Guard g(s_mutex);
      if(s_map.empty()) {
        info("All %ld MemCheck objects were released", s_total);
      } else {
        info("%ld of %ld MemCheck objects were not released",
             (long) s_map.size(), (long) s_total);
        std::multimap<int, MemMap::value_type> sorted;
        for(MemMap::iterator it = s_map.begin(); it != s_map.end(); ++it)
          sorted.insert(std::pair<int, MemMap::value_type>(it->second.second, *it));

        int count = 0;
        for(std::multimap<int, MemMap::value_type>::iterator it2 = sorted.begin(); it2 != sorted.end(); ++it2, ++count) {
          size_t size = it2->first;
          MemCheck * obj = it2->second.first;
          void ** data = it2->second.second.first;

          int status;
          char * tmp = abi::__cxa_demangle(typeid(*obj).name(), 0, 0, &status);
          error("MemCheck object %s was not released", tmp ? tmp : typeid(*obj).name());
          free(tmp);

          print_bt(data, size);
          if(count == 50) {
            error(".. limiting error printing to 50 errors (there are %ld errors)",
                  (long) s_map.size());
            break;
          }
        }
      }
    }
  };
#endif

  static Checker s_checker;
}
#endif // MULTI_MEMCHECK
