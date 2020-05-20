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

#ifdef RADIANT_LINUX
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#endif

#include <cassert>
#include <cstdio>
#include <string>
#include <thread>

#include <QRegExp>
#include <QMutex>
#include <QWaitCondition>

#include "StringUtils.hpp"
#include "FileUtils.hpp"

namespace
{
#ifdef RADIANT_LINUX
  const char * addr2lineOpts()
  {
    static const char * opts = nullptr;
    if (!opts) {
      // check for --pretty-print
      if (Radiant::FileUtils::runInShell("addr2line -pe /bin/false 0 >/dev/null 2>&1") == 0)
        opts = "-pie";
      else
        opts = "-ie";
    }
    return opts;
  }

  /// Calling addr2line takes hundreds of milliseconds on some libraries, but
  /// after it has started, it works pretty quickly. Keep these processes alive
  /// to keep the symbols in memory for faster lookup.
  class AddressTranslator
  {
  public:
    // Using QProcess might be a little dangerous, since we might be outside main() already
    AddressTranslator(const QString & file)
    {
      // This is basically read-write popen or "popen2"
      int pipeStdin[2];
      int pipeStdout[2];

      if (pipe(pipeStdin))
        return;

      if (pipe(pipeStdout)) {
        close(pipeStdin[0]);
        close(pipeStdin[1]);
        return;
      }

      m_addr2linePid = fork();
      if (m_addr2linePid < 0) {
        close(pipeStdin[0]);
        close(pipeStdin[1]);
        close(pipeStdout[0]);
        close(pipeStdout[1]);
        return;
      }

      if (m_addr2linePid == 0) {
        close(pipeStdin[1]);
        close(pipeStdout[0]);
        dup2(pipeStdin[0], 0);
        dup2(pipeStdout[1], 1);
        execlp("addr2line", "addr2line", addr2lineOpts(), file.toUtf8().data(), nullptr);
        exit(1);
      }

      close(pipeStdin[0]);
      close(pipeStdout[1]);
      m_write = pipeStdin[1];
      m_read = pipeStdout[0];
    }

    ~AddressTranslator()
    {
      if (m_read)
        close(m_read);
      if (m_write)
        close(m_write);
      if (m_addr2linePid)
        kill(m_addr2linePid, SIGTERM);
    }

    QString fileAndLine(intptr_t ptr)
    {
      if (!m_write)
        return QString();

      m_lastUsed = Radiant::TimeStamp::currentTime();

      QByteArray tmp = QString("%1\n").arg(ptr, 0, 16).toUtf8();
      if (write(m_write, tmp.data(), tmp.size()) != tmp.size())
        return QString();

      tmp.clear();
      char buffer = 0;
      while (read(m_read, &buffer, 1) == 1) {
        if (buffer == '\n')
          break;
        tmp.append(buffer);
      }

      if (tmp == "??:0")
        return QString();
      return tmp;
    }

    bool shouldDelete() const
    {
      return m_lastUsed.sinceSecondsD() > 60*5;
    }

  private:
    Radiant::TimeStamp m_lastUsed = Radiant::TimeStamp::currentTime();
    pid_t m_addr2linePid = 0;
    int m_write = 0;
    int m_read = 0;
  };

  std::map<QString, std::unique_ptr<AddressTranslator>> s_translators;
  QMutex s_translatorsMutex;
  QWaitCondition s_cleanerCondition;
  std::thread s_cleanerThread;

  QString fileAndLine(const QString & file, intptr_t ptr)
  {
    QMutexLocker g(&s_translatorsMutex);
    bool spawnCleanerThread = s_translators.empty();
    auto & translator = s_translators[file];
    if (!translator)
      translator.reset(new AddressTranslator(file));

    if (spawnCleanerThread) {
      if (s_cleanerThread.joinable())
        s_cleanerThread.join();
      s_cleanerThread = std::thread([] {
        while (true) {
          QMutexLocker g(&s_translatorsMutex);
          s_cleanerCondition.wait(&s_translatorsMutex, 1000 * 60);
          for (auto it = s_translators.begin(); it != s_translators.end(); ) {
            if (it->second->shouldDelete()) {
              it = s_translators.erase(it);
            } else {
              ++it;
            }
          }
          if (s_translators.empty())
            break;
        }
      });

      MULTI_ONCE atexit([] {
        s_translatorsMutex.lock();
        s_translators.clear();
        s_translatorsMutex.unlock();
        s_cleanerCondition.wakeAll();
        if (s_cleanerThread.joinable())
          s_cleanerThread.join();
      });
    }

    return ptr ? translator->fileAndLine(ptr) : QString();
  }
#else
  QString fileAndLine(const QString &, intptr_t)
  {
    return QString();
  }
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

    // Preload resolvers in parallel
    for (size_t i = 0; i < size(); i++)
      if (r.exactMatch(QString::fromUtf8(strings[i])))
        fileAndLine(r.cap(1), 0);

    for(size_t i = 0; i < size(); i++) {
      if(r.exactMatch(QString::fromUtf8(strings[i]))) {
        QByteArray func = Radiant::StringUtils::demangle(r.cap(2).toUtf8().data());
        QString file;

        Dl_info info;
        info.dli_fbase = 0;
        dladdr(m_frames[i], &info);
        if(info.dli_fbase) {
          file = fileAndLine(r.cap(1), (long)m_frames[i] - (long)info.dli_fbase);
        }

        if(file.isEmpty())
          file = fileAndLine(r.cap(1), (long)m_frames[i]);
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
    free(strings);
    m_cache = tmp;
    return tmp;
  }

  void CallStack::print() const
  {
    for (auto & str: toStringList())
      Radiant::error("%s", str.toUtf8().data());
  }
}
