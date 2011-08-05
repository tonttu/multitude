#include <Radiant/Platform.hpp>
#include <Radiant/CallStack.hpp>
#include <Radiant/Mutex.hpp>
#include <Radiant/Trace.hpp>

#include <stdint.h>

#include <execinfo.h>
#include <dlfcn.h>

#include <cassert>
#include <cstdio>
#include <string>

#include <QRegExp>

#include "StringUtils.hpp"

// Utility function
QString file_and_line(const QString & file, long ptr)
{
  assert(sizeof(long) == sizeof(void*));
  char buffer[512];

  // doesn't need to be thread safe
  static const char * opts = 0;
  if(!opts) {
    // check for --pretty-print
    if(system("addr2line -pe /bin/false 0 >/dev/null 2>&1") == 0)
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
}

namespace Radiant
{
  CallStack::CallStack()
  {
    m_frameCount = backtrace(m_frames, max_frames);
  }

  CallStack::~CallStack()
  {
  }

  void CallStack::print() const
  {
    char ** strings = backtrace_symbols(stack(), size());

    /// strings[i] can be something like:
    /// /home/riku/cornerstone/multitude/lib/libRadiant.so.1(_ZN7Radiant8MemCheckC2Ev+0x40) [0x7fe3fd420c64]
    /// ./LaunchStack() [0x420669]
    //        binary     (    function(opt)           +offset(opt)      )
    QRegExp r("(.*)"  "\\("       "([^+]*)"  "(?:\\+0x[0-9a-f]+)?"   "\\).*");

    for(size_t i = 0; i < size(); i++) {
      if(r.exactMatch(QString::fromUtf8(strings[i]))) {
        QString func = Radiant::StringUtils::demangle(r.cap(2).toUtf8().data());
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
          Radiant::error("#%-2d %p at %s", int(i), m_frames[i], file.toUtf8().data());
        else
          Radiant::error("#%-2d %s at %s", int(i), func.toUtf8().data(), file.toUtf8().data());
      } else {
        Radiant::error("#%-2d %s", int(i), strings[i]);
      }
    }
    free (strings);
  }
}
