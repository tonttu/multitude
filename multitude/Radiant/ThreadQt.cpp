/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Thread.hpp"
#include "Mutex.hpp"
#include "Sleep.hpp"
#include "StringUtils.hpp"
#include "Trace.hpp"
#include "ThreadChecks.hpp"

#include <errno.h>
#include <signal.h>
#include <string.h>
#include <iostream>
#include <cassert>

#include <QThread>

#ifdef RADIANT_WINDOWS
#include <windows.h>
#include <processthreadsapi.h>

namespace
{
  typedef HRESULT (WINAPI * SetThreadDescriptionFn)(HANDLE hThread, PCWSTR lpThreadDescription);
  typedef HRESULT (WINAPI * GetThreadDescriptionFn)(HANDLE hThread, PWSTR* ppszThreadDescription);
  SetThreadDescriptionFn setThreadDescription = nullptr;
  GetThreadDescriptionFn getThreadDescription = nullptr;
  bool s_initialized = false;

  /// Load these functions dynamically, since apparently Windows 2016 server
  /// doesn't have them [1], even though the documentation says it should [2].
  /// 1: https://redmine.multitaction.com/issues/16358
  /// 2: https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreaddescription
  void initializeThreadDescriptionFunctions()
  {
    if (s_initialized)
      return;
    setThreadDescription = reinterpret_cast<SetThreadDescriptionFn>(
          GetProcAddress(GetModuleHandleA("Kernel32"), "SetThreadDescription"));
    getThreadDescription = reinterpret_cast<GetThreadDescriptionFn>(
          GetProcAddress(GetModuleHandleA("Kernel32"), "GetThreadDescription"));
    s_initialized = true;
  }
}

#endif

namespace Radiant {

  bool Thread::m_threadDebug = false;
  bool Thread::m_threadWarnings = false;

  class Thread::D : public QThread 
  {
  public:
    D(Thread * host) : m_host(host) {}

    virtual void run() {
#ifdef RADIANT_WINDOWS
      /// Qt only sets thread names to visual studio using vs-specific exceptions.
      /// It doesn't set the thread name that is used by crash dumps and other
      /// code. Fix that here. See more information about thread names in Windows:
      /// https://docs.microsoft.com/en-us/visualstudio/debugger/how-to-set-a-thread-name-in-native-code
      initializeThreadDescriptionFunctions();
      if (setThreadDescription) {
        HRESULT res = setThreadDescription(GetCurrentThread(), objectName().toStdWString().data());
        if (FAILED(res))
          Radiant::error("SetThreadDescription: %s", StringUtils::getLastErrorMessage().toUtf8().data());
      }
#endif
      m_host->childLoop();
    }

    Thread * m_host;
  };

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

  Thread::Thread(const QString &name)
    : m_d(new D(this))
  {
    setName(name);
  }

  Thread::~Thread()
  {
    assert(isRunning() == false);

    delete m_d;
  }

  void Thread::setName(const QString & name)
  {
    assert(isRunning() == false);

    m_d->setObjectName(name);
  }

  Thread::Id Thread::currentThreadId()
  {
#ifdef RADIANT_LINUX
    return reinterpret_cast<Thread::Id>(pthread_self());
#elif defined(RADIANT_WINDOWS)
    return reinterpret_cast<Thread::Id>(static_cast<intptr_t>(GetCurrentThreadId()));
#else
    return reinterpret_cast<Thread::Id>(QThread::currentThread());
#endif
  }

  QByteArray Thread::threadName(Thread::Id threadId)
  {
    if (threadId == ThreadChecks::mainThreadId)
      return "Main thread";
#ifdef RADIANT_LINUX
    char buffer[128];
    if (pthread_getname_np(reinterpret_cast<pthread_t>(threadId), buffer, sizeof(buffer)) == 0)
      return buffer;
#elif defined(RADIANT_WINDOWS)
    initializeThreadDescriptionFunctions();
    if (getThreadDescription) {
      if (HANDLE handle = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, false,
                                     static_cast<DWORD>(reinterpret_cast<intptr_t>(threadId)))) {
        PWSTR data = nullptr;
        HRESULT hr = getThreadDescription(handle, &data);
        CloseHandle(handle);
        if (SUCCEEDED(hr) && data) {
          QByteArray name = QString::fromWCharArray(data).toUtf8();
          LocalFree(data);
          return name;
        }
      }
    }
#endif

    return "#" + QByteArray::number(reinterpret_cast<qlonglong>(threadId));
  }

  QByteArray Thread::currentThreadName()
  {
#ifdef RADIANT_WINDOWS
    initializeThreadDescriptionFunctions();
    if (getThreadDescription) {
      PWSTR data = nullptr;
      HRESULT hr = getThreadDescription(GetCurrentThread(), &data);
      if (SUCCEEDED(hr) && data) {
        QByteArray name = QString::fromWCharArray(data).toUtf8();
        LocalFree(data);
        return name;
      }
    }
#endif
    return threadName(currentThreadId());
  }

  void Thread::run()
  {
    m_d->start();
  }

  bool Thread::waitEnd(int timeoutms)
  {
    if(timeoutms)
      return m_d->wait(timeoutms);
    else
      return m_d->wait();
  }

  bool Thread::isRunning() const
  {
    return m_d->isRunning();
  }

  QThread * Thread::qthread() const
  {
    return m_d;
  }
}
