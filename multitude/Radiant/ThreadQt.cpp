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
      HRESULT res = SetThreadDescription(GetCurrentThread(), objectName().toStdWString().data());
      if (FAILED(res))
        Radiant::error("SetThreadDescription: %s", StringUtils::getLastErrorMessage().toUtf8().data());
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
    return reinterpret_cast<Thread::Id>(GetCurrentThreadId());
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
    if (HANDLE handle = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, false, reinterpret_cast<DWORD>(threadId))) {
      PWSTR data = nullptr;
      HRESULT hr = GetThreadDescription(handle, &data);
      CloseHandle(handle);
      if (SUCCEEDED(hr) && *data) {
        QByteArray name = QString::fromWCharArray(data).toUtf8();
        LocalFree(data);
        return name;
      }
    }
#endif

    return "#" + QByteArray::number(reinterpret_cast<qlonglong>(threadId));
  }

  QByteArray Thread::currentThreadName()
  {
#ifdef RADIANT_WINDOWS
    PWSTR data = nullptr;
    HRESULT hr = GetThreadDescription(GetCurrentThread(), &data);
    if (SUCCEEDED(hr) && *data) {
      QByteArray name = QString::fromWCharArray(data).toUtf8();
      LocalFree(data);
      return name;
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
