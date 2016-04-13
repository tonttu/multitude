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

#include "PulseAudioContext.hpp"

#include <pulse/pulseaudio.h>

namespace Resonant
{
  bool PulseAudioContext::PaOperation::isRunning() const
  {
    return m_op && pa_operation_get_state(m_op) == PA_OPERATION_RUNNING;
  }

  void PulseAudioContext::PaOperation::cancel()
  {
    if (m_op) {
      pa_operation_cancel(m_op);
      pa_operation_unref(m_op);
      m_op = nullptr;
    }
  }

  void PulseAudioContext::PaOperation::setPaOperation(pa_operation * op)
  {
    m_op = op;
  }

  bool PulseAudioContext::PaOperation::waitForFinished(double timeoutSecs)
  {
    if (auto op = m_op) {
      pa_operation_ref(op);

      if (pa_operation_get_state(op) != PA_OPERATION_RUNNING) {
        pa_operation_unref(op);
        return true;
      }

      std::pair<Radiant::Mutex, Radiant::Condition> mutexCond;

      pa_operation_set_state_callback(op, [](pa_operation *, void * userdata) {
        auto p = static_cast<std::pair<Radiant::Mutex, Radiant::Condition>*>(userdata);
        p->second.wakeAll(p->first);
      }, &mutexCond);

      unsigned int ms = std::max<unsigned int>(timeoutSecs > 0 ? 1 : 0, std::round(timeoutSecs * 1000));

      Radiant::Guard g(mutexCond.first);
      while (pa_operation_get_state(op) == PA_OPERATION_RUNNING && ms > 0) {
        mutexCond.second.wait2(mutexCond.first, ms);
      }

      pa_operation_set_state_callback(op, nullptr, nullptr);

      pa_operation_unref(op);
    }
    return !isRunning();
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
    PaOperationWithCallback(std::function<void(Args...)> callback)
      : m_callback(callback)
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

    bool m_restart = false;

    std::weak_ptr<PulseAudioContext> m_host;

    Radiant::Mutex m_operationsMutex;
    std::vector<PulseAudioContext::PaOperationPtr> m_operations;

    const QByteArray m_name;

    Radiant::Mutex m_contextReadyMutex;
    Radiant::Condition m_contextReadyCond;
    bool m_contextReady = false;
  };

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
    {
      m_contextReady = ready;
      m_contextReadyCond.wakeAll(m_contextReadyMutex);
    }
    if (auto host = m_host.lock()) {
      host->eventSend(ready ? "ready" : "not-ready");
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

  void PulseAudioContext::D::restart()
  {
    setReady(false);

    Radiant::Guard g(m_stateMutex);
    m_restart = true;

    if (state() == D::DONE) {
      setState(D::WAITING);
      schedule(Radiant::TimeStamp(0));
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
          scheduleFromNowSecs(5.0);
        }
      }
    } else {
      if (m_context) {
        closeConnection();
      }
    }

    Radiant::Guard g(m_stateMutex);
    if (m_running == !!m_context) {
      setFinished();
    }
  }

  void PulseAudioContext::D::changeState(bool running)
  {
    Radiant::Guard g(m_stateMutex);
    m_running = running;
    if (state() == D::DONE) {
      setState(D::WAITING);
      schedule(Radiant::TimeStamp(0));
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
    std::vector<PaOperationPtr> operations;
    {
      Radiant::Guard g(m_d->m_operationsMutex);
      std::swap(operations, m_d->m_operations);
    }
    operations.clear();
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
    unsigned int ms = std::max<unsigned int>(timeoutSecs > 0 ? 1 : 0, std::round(timeoutSecs * 1000));

    Radiant::Guard g(m_d->m_contextReadyMutex);
    while (!m_d->m_contextReady && ms > 0) {
      m_d->m_contextReadyCond.wait2(m_d->m_contextReadyMutex, ms);
    }

    return m_d->m_contextReady;
  }

  pa_context * PulseAudioContext::paContext() const
  {
    return m_d->m_context;
  }

  PulseAudioContext::PaOperationPtr PulseAudioContext::listSources(
      std::function<void (const pa_source_info *, bool)> cb)
  {
    if (m_d->m_context) {
      auto op = std::make_shared<PaOperationWithCallback<const pa_source_info *, bool>>(cb);
      addOperation(op);
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
}
#endif
