/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "ThreadPool.hpp"

#include <QMap>

namespace Radiant {

  enum ThreadState {
    STARTING,
    RUNNING,
    STOPPING,
    STOPPED
  };

  class ThreadPool::Private
  {
    class T;
    typedef QMap<QThread*, ThreadState> Threads;

    class T : public QThread
    {
      Private & m_private;

    public:
      T(Private & p) : m_private(p)
      {
        setObjectName("ThreadPool");
      }
      virtual ~T() {}

      void run()
      {
        {
          std::lock_guard<std::mutex> g(m_private.m_mutex);
          if(m_private.m_threads[this] == STOPPING) {
            m_private.m_threads[this] = STOPPED;
            return;
          } else {
            m_private.m_threads[this] = RUNNING;
          }
        }

        m_private.m_threadPool.childLoop();

        std::lock_guard<std::mutex> g(m_private.m_mutex);
        m_private.m_threads[this] = STOPPED;
      }

    };

  public:

    Private(ThreadPool & tp) : m_threadPool(tp) {}
    ~Private()
    {
      // But there really shouldn't be anybody else doing anything anymore!
      std::lock_guard<std::mutex> g(m_mutex);
      Q_FOREACH(QThread * thread, m_threads.keys())
        delete thread;
    }

    bool setThreads(int number)
    {
      bool signal = false;
      std::lock_guard<std::mutex> g(m_mutex);

      /// Delete old threads
      QMutableMapIterator<QThread*, ThreadState> it(m_threads);
      while(it.hasNext()) {
        it.next();
        if(it.value() == STOPPED) {
          delete it.key();
          it.remove();
        }
      }

      int current = threadCount();

      // start new threads
      for(int i = current; i < number; ++i) {
        T * t = new T(*this);
        m_threads[t] = STARTING;
        t->start();
      }

      // close old threads
      for(int i = current; i > number; --i) {
        it = m_threads;
        while(it.hasNext()) {
          it.next();
          if(it.value() == RUNNING || it.value() == STARTING) {
            it.value() = STOPPING;
            signal = true;
            break;
          }
        }
      }
      return signal;
    }

    bool waitEnd()
    {
      bool ok = true;
      Q_FOREACH(QThread * t, m_threads.keys())
        if(!t->wait())
          ok = false;
      return ok;
    }

    bool isRunning() const
    {
      Q_FOREACH(const QThread * t, m_threads.keys())
        if(t->isRunning())
          return true;
      return false;
    }

    int threadCount() const
    {
      int num = 0;
      Q_FOREACH(ThreadState s, m_threads)
        if(s == STARTING || s == RUNNING)
          ++num;
      return num;
    }

    std::mutex m_mutex;
    ThreadPool & m_threadPool;
    Threads m_threads;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ThreadPool::ThreadPool() : m_p(new Private(*this))
  {
  }

  ThreadPool::~ThreadPool()
  {
    delete m_p;
  }

  void ThreadPool::run(int number)
  {
    if(m_p->setThreads(number))
      wakeAll();
  }

  bool ThreadPool::stop()
  {
    run(0);
    return m_p->waitEnd();
  }

  bool ThreadPool::waitEnd()
  {
    return m_p->waitEnd();
  }

  bool ThreadPool::isRunning() const
  {
    return m_p->isRunning();
  }

  int ThreadPool::threads() const
  {
    std::lock_guard<std::mutex> g(m_p->m_mutex);
    return m_p->threadCount();
  }

  bool ThreadPool::contains(const QThread* thread) const
  {
    std::lock_guard<std::mutex> g(m_p->m_mutex);
    for(const auto & poolThread : m_p->m_threads.keys()) {

      if(poolThread == thread)
        return true;
    }

    return false;
  }

  bool ThreadPool::running() const
  {
    std::lock_guard<std::mutex> g(m_p->m_mutex);
    return m_p->m_threads[QThread::currentThread()] == RUNNING;
  }

  void ThreadPool::wakeAll()
  {
    m_wait.notify_all();
  }
}
