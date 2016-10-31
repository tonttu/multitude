/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include <Radiant/Platform.hpp>

#ifdef RADIANT_LINUX

#include <Radiant/BGThread.hpp>
#include <Radiant/Task.hpp>
#include <Radiant/Timer.hpp>
#include <Radiant/Condition.hpp>

#include "PulseAudioContext.hpp"

#include <pulse/pulseaudio.h>

namespace Resonant
{
  bool PulseAudioContext::PaOperation::isRunning() const
  {
    return m_op && !m_finished;
  }

  void PulseAudioContext::PaOperation::cancel()
  {
    if (m_op) {
      Lock lock(m_mainloop);
      pa_operation_cancel(m_op);
      pa_operation_unref(m_op);
      m_op = nullptr;
    }
  }

  void PulseAudioContext::PaOperation::setFinished()
  {
    m_finished = true;
    Radiant::Guard g(m_finishedCondMutex);
    m_finishedCond.wakeAll();
  }

  void PulseAudioContext::PaOperation::setPaOperation(pa_operation * op)
  {
    assert(!m_op);
    assert(op);
    m_op = op;

    Lock lock(m_mainloop);
    pa_operation_set_state_callback(op, [](pa_operation * op, void * userdata) {
      pa_operation_state_t state = pa_operation_get_state(op);
      auto self = static_cast<PaOperation*>(userdata);
      bool finished = state == PA_OPERATION_DONE || state == PA_OPERATION_CANCELLED;
      if (finished) {
        pa_operation_set_state_callback(op, nullptr, nullptr);
        self->setFinished();
      }
    }, this);
  }

  bool PulseAudioContext::PaOperation::waitForFinished(double timeoutSecs)
  {
    unsigned int ms = std::max<unsigned int>(timeoutSecs > 0 ? 1 : 0, std::round(timeoutSecs * 1000));

    Radiant::Guard g(m_finishedCondMutex);
    while (isRunning() && ms > 0) {
      m_finishedCond.wait2(m_finishedCondMutex, ms);
    }

    return !isRunning();
  }

  PulseAudioContext::PaOperation::PaOperation(pa_threaded_mainloop * loop)
    : m_mainloop(loop)
  {
  }

  PulseAudioContext::PaOperation::~PaOperation()
  {
    cancel();
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <typename... Args>
  class PaOperationWithCallback : public PulseAudioContext::PaOperation
  {
  public:
    PaOperationWithCallback(pa_threaded_mainloop * loop, std::function<void(Args...)> callback)
      : PaOperation(loop)
      , m_callback(callback)
    {}

    void call(Args... args)
    {
      m_callback(args...);
    }

  private:
    std::function<void(Args...)> m_callback;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class PulseAudioContext::D : public Radiant::Task
  {
  public:
    D(std::weak_ptr<PulseAudioContext> host, const QByteArray & name);

    ~D();

    void contextChange(pa_context_state_t state);
    void restart();
    void autoSchedule();
    bool openConnection();
    void closeConnection();

    virtual void doTask() OVERRIDE;
    void changeState(bool running);

    void setReady(bool ready);

  public:
    pa_context * m_context = nullptr;
    pa_threaded_mainloop * m_mainloop = nullptr;

    Radiant::Mutex m_stateMutex;
    bool m_running = false;

    int m_restartIteration = 0;
    bool m_restart = false;

    std::weak_ptr<PulseAudioContext> m_host;

    Radiant::Mutex m_operationsMutex;
    std::vector<PulseAudioContext::PaOperationPtr> m_operations;

    const QByteArray m_name;

    Radiant::Mutex m_contextReadyMutex;
    Radiant::Condition m_contextReadyCond;
    bool m_contextReady = false;
    long m_nextOnReadyId = 1;
    std::map<long, std::function<void()>> m_onReady;
  };

  PulseAudioContext::PaOperation::PaOperation(PulseAudioContext & context)
    : m_mainloop(context.m_d->m_mainloop)
  {
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  PulseAudioContext::D::D(std::weak_ptr<PulseAudioContext> host, const QByteArray & name)
    : m_host(host)
    , m_name(name)
  {
    setFinished();
  }

  PulseAudioContext::D::~D()
  {
    if (m_context) {
      closeConnection();
    }
  }

  void PulseAudioContext::D::setReady(bool ready)
  {
    std::map<long, std::function<void()>> onReady;
    {
      m_contextReady = ready;
      Radiant::Guard g(m_contextReadyMutex);
      if (ready) {
        onReady = m_onReady;
      }
      m_contextReadyCond.wakeAll();
    }
    for (auto & p: onReady) {
      p.second();
    }

    if (auto host = m_host.lock()) {
      host->eventSend(ready ? "ready" : "not-ready");
    }

    if (ready) {
      m_restartIteration = 0;
    }
  }

  void PulseAudioContext::D::contextChange(pa_context_state_t state)
  {
    switch (state) {
      case PA_CONTEXT_CONNECTING:
      case PA_CONTEXT_AUTHORIZING:
      case PA_CONTEXT_SETTING_NAME:
        break;

      case PA_CONTEXT_READY:
        setReady(true);
        break;

      case PA_CONTEXT_FAILED:
        Radiant::error("PulseAudioContext # PulseAudio connection failure: %s", pa_strerror(pa_context_errno(m_context)));
      case PA_CONTEXT_TERMINATED:
        restart();
        break;

      default:
        Radiant::error("PulseAudioContext # Unknown PulseAudio context state: %d", state);
    }
  }

  void PulseAudioContext::D::autoSchedule()
  {
    // Increase retry iteration time from 1 s up to 5 seconds in 30 seconds
    scheduleFromNowSecs(std::min(5, (m_restartIteration + 5) / 6));
  }

  void PulseAudioContext::D::restart()
  {
    setReady(false);

    Radiant::Guard g(m_stateMutex);
    m_restart = true;
    ++m_restartIteration;

    if (state() == D::DONE) {
      setState(D::WAITING);
      autoSchedule();
      Radiant::BGThread::instance()->addTask(shared_from_this());
    }
  }

  bool PulseAudioContext::D::openConnection()
  {
    auto mainloop = pa_threaded_mainloop_new();
    if (!mainloop) {
      Radiant::error("PulseAudioContext # pa_mainloop_new() failed");
      return false;
    }

    auto mainloopApi = pa_threaded_mainloop_get_api(mainloop);

    auto context = pa_context_new(mainloopApi, m_name.data());
    if (!context) {
      Radiant::error("PulseAudioContext # pa_context_new() failed.");
      pa_threaded_mainloop_free(mainloop);
      return false;
    }

    m_context = context;
    m_mainloop = mainloop;

    pa_context_set_state_callback(m_context, [] (pa_context * c, void * ptr) {
      if (c && ptr) {
        auto self = static_cast<D*>(ptr);
        if (self->m_context == c) {
          self->contextChange(pa_context_get_state(c));
        }
      }
    }, this);

    pa_context_connect(m_context, 0, (pa_context_flags_t)0, 0);

    if (pa_threaded_mainloop_start(m_mainloop) != 0) {
      Radiant::error("PulseAudioContext # pa_threaded_mainloop_start() failed");
      pa_context_unref(m_context);
      pa_threaded_mainloop_free(m_mainloop);
      m_context = nullptr;
      m_mainloop = nullptr;
      return false;
    }

    return true;
  }

  void PulseAudioContext::D::closeConnection()
  {
    std::vector<PaOperationPtr> operations;
    {
      Radiant::Guard g(m_operationsMutex);
      std::swap(operations, m_operations);
    }
    operations.clear();
    pa_threaded_mainloop_stop(m_mainloop);
    pa_context_unref(m_context);
    pa_threaded_mainloop_free(m_mainloop);
    m_context = nullptr;
    m_mainloop = nullptr;
  }

  void PulseAudioContext::D::doTask()
  {
    if (m_running) {
      if (m_restart && m_context) {
        closeConnection();
      }
      m_restart = false;

      if (!m_context) {
        if (!openConnection()) {
          ++m_restartIteration;
          autoSchedule();
        }
      }
    } else {
      if (m_context) {
        closeConnection();
      }
    }

    Radiant::Guard g(m_stateMutex);
    if (m_running == !!m_context) {
      if (m_running && m_restart) {
        autoSchedule();
      } else {
        setFinished();
      }
    }
  }

  void PulseAudioContext::D::changeState(bool running)
  {
    Radiant::Guard g(m_stateMutex);
    m_running = running;
    if (state() == D::DONE) {
      setState(D::WAITING);
      autoSchedule();
      Radiant::BGThread::instance()->addTask(shared_from_this());
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  PulseAudioContextPtr PulseAudioContext::create(const QByteArray & name)
  {
    PulseAudioContextPtr context(new PulseAudioContext());
    context->m_d.reset(new D(context, name));
    return context;
  }

  PulseAudioContext::PulseAudioContext()
  {
    eventAddOut("ready");
    eventAddOut("not-ready");
  }

  PulseAudioContext::~PulseAudioContext()
  {
    stop();
  }

  void PulseAudioContext::start()
  {
    m_d->changeState(true);
  }

  void PulseAudioContext::stop()
  {
    m_d->changeState(false);
  }

  void PulseAudioContext::addOperation(PulseAudioContext::PaOperationPtr op)
  {
    Radiant::Guard g(m_d->m_operationsMutex);
    for (auto it = m_d->m_operations.begin(); it != m_d->m_operations.end(); ) {
      if ((*it)->isRunning()) {
        ++it;
      } else {
        it = m_d->m_operations.erase(it);
      }
    }
    m_d->m_operations.push_back(std::move(op));
  }

  bool PulseAudioContext::waitForReady(double timeoutSecs)
  {
    Radiant::Timer timer;
    for (int i = 0; i < 3 && timer.time() < timeoutSecs && m_d->state() != Radiant::Task::DONE &&
         m_d->m_running; ++i) {
      m_d->runNow(false);
    }
    // Make sure not to dead-lock by waiting for the condition
    if (m_d->state() != Radiant::Task::DONE) {
      return m_d->m_contextReady;
    }
    timeoutSecs -= timer.time();
    unsigned int ms = std::max<unsigned int>(timeoutSecs > 0 ? 1 : 0, std::round(timeoutSecs * 1000));

    Radiant::Guard g(m_d->m_contextReadyMutex);
    while (!m_d->m_contextReady && ms > 0) {
      m_d->m_contextReadyCond.wait2(m_d->m_contextReadyMutex, ms);
    }

    return m_d->m_contextReady;
  }

  long PulseAudioContext::onReady(std::function<void()> func)
  {
    long id = 0;
    bool run = false;
    {
      Radiant::Guard g(m_d->m_contextReadyMutex);
      id = m_d->m_nextOnReadyId++;
      m_d->m_onReady[id] = std::move(func);
      run = m_d->m_contextReady;
    }
    if (run) {
      func();
    }
    return id;
  }

  void PulseAudioContext::removeOnReadyListener(long id)
  {
    Radiant::Guard g(m_d->m_contextReadyMutex);
    m_d->m_onReady.erase(id);
  }

  pa_context * PulseAudioContext::paContext() const
  {
    return m_d->m_context;
  }

  PulseAudioContext::PaOperationPtr PulseAudioContext::listSources(
      std::function<void (const pa_source_info *, bool)> cb)
  {
    if (m_d->m_context) {
      auto op = std::make_shared<PaOperationWithCallback<const pa_source_info *, bool>>(m_d->m_mainloop, cb);
      addOperation(op);
      Lock lock(*this);
      op->setPaOperation(pa_context_get_source_info_list(m_d->m_context, []
            (pa_context *, const pa_source_info * i, int eol, void * ptr) {
        auto op = static_cast<PaOperationWithCallback<const pa_source_info *, bool>*>(ptr);
        op->call(i, eol);
      }, op.get()));
      return op;
    } else {
      return nullptr;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  PulseAudioContext::Lock::Lock(PulseAudioContext & context)
    : m_mainloop(context.m_d->m_mainloop)
  {
    pa_threaded_mainloop_lock(m_mainloop);
  }

  PulseAudioContext::Lock::Lock(pa_threaded_mainloop * mainloop)
    : m_mainloop(mainloop)
  {
    pa_threaded_mainloop_lock(m_mainloop);
  }

  PulseAudioContext::Lock & PulseAudioContext::Lock::operator=(PulseAudioContext::Lock && o)
  {
    std::swap(m_mainloop, o.m_mainloop);
    return *this;
  }

  PulseAudioContext::Lock::Lock(PulseAudioContext::Lock && o)
    : m_mainloop(o.m_mainloop)
  {
    o.m_mainloop = nullptr;
  }

  PulseAudioContext::Lock::~Lock()
  {
    if (m_mainloop) {
      pa_threaded_mainloop_unlock(m_mainloop);
    }
  }

}
#endif
