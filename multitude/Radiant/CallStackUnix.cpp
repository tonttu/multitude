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

#include "CallStack.hpp"
#include "Mutex.hpp"
#include "Trace.hpp"

#include <cstdint>

#include <execinfo.h>
#include <dlfcn.h>

#include <cassert>
#include <cstdio>
#include <string>

#include <QRegExp>

#include "StringUtils.hpp"
#include "FileUtils.hpp"

// Utility function
QString file_and_line(const QString & file, long ptr)
{
#ifdef RADIANT_LINUX
  assert(sizeof(long) == sizeof(void*));
  char buffer[512];

  // doesn't need to be thread safe
  static const char * opts = 0;
  if(!opts) {
    // check for --pretty-print
    if(Radiant::FileUtils::runInShell("addr2line -pe /bin/false 0 >/dev/null 2>&1") == 0)
      opts = "-pie";
    else
      opts = "-ie";
  }

  // Using QProcess might be a little dangerous, since we might be outside main() already
  FILE * f = popen(QString("addr2line %1 \"%2\" %3").arg(opts).arg(file).arg(ptr, 0, 16).toUtf8().data(), "r");
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
#else
  (void)file;
  (void)ptr;
  return QString();
#endif
}

namespace Radiant
{
  CallStack::CallStack()
  {
    m_frameCount = backtrace(m_frames, MAX_FRAMES);
  }

  CallStack::~CallStack()
  {
  }

  QStringList CallStack::toStringList() const
  {
    if (!m_cache.isEmpty())
      return m_cache;

    char ** strings = backtrace_symbols(stack(), size());

    QStringList tmp;

    /// strings[i] can be something like:
    /// /home/riku/cornerstone/multitude/lib/libRadiant.so.1(_ZN7Radiant8MemCheckC2Ev+0x40) [0x7fe3fd420c64]
    /// ./LaunchStack() [0x420669]
    //        binary     (    function(opt)           +offset(opt)      )
    QRegExp r("(.*)"  "\\("       "([^+]*)"  "(?:\\+0x[0-9a-f]+)?"   "\\).*");

    for(size_t i = 0; i < size(); i++) {
      if(r.exactMatch(QString::fromUtf8(strings[i]))) {
        QByteArray func = Radiant::StringUtils::demangle(r.cap(2).toUtf8().data());
        QString file;

        Dl_info info;
        info.dli_fbase = 0;
        dladdr(m_frames[i], &info);
        if(info.dli_fbase) {
          file = file_and_line(r.cap(1), (long)m_frames[i] - (long)info.dli_fbase);
        }

        if(file.isEmpty())
          file = file_and_line(r.cap(1), (long)m_frames[i]);
        if(file.isEmpty())
          file = r.cap(1);

        if(func.isEmpty())
          tmp << QString("#%1 0x%2 at %3").arg(i, -2).arg((intptr_t)m_frames[i], 0, 16).arg(file);
        else
          tmp << QString("#%1 %2 at %3").arg(i, -2).arg(func.data(), file);
      } else {
        tmp << QString("#%1 %3").arg(i, -2).arg(strings[i]);
      }
    }
    free (strings);
    m_cache = tmp;
    return tmp;
  }

  void CallStack::print() const
  {
    for (auto & str: toStringList())
      Radiant::error("%s", str.toUtf8().data());
  }
}
