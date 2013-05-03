/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "State.hpp"
#include "Node.hpp"

#include <Radiant/Mutex.hpp>

namespace Valuable
{
  class Callback
  {
  public:
    Callback();
    Callback(StateInt::CallbackType callback, bool direct, int stateMask);
    Callback(Callback && c);
    Callback & operator=(Callback && c);

  public:
    StateInt::CallbackType m_callback;
    bool m_direct;
    int m_stateMask;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  Callback::Callback()
    : m_direct(false),
      m_stateMask(0)
  {}

  Callback::Callback(StateInt::CallbackType callback, bool direct, int stateMask)
    : m_callback(callback),
      m_direct(direct),
      m_stateMask(stateMask)
  {}

  Callback::Callback(Callback && c)
    : m_callback(std::move(c.m_callback)),
      m_direct(std::move(c.m_direct)),
      m_stateMask(std::move(c.m_stateMask))
  {}

  Callback & Callback::operator=(Callback && c)
  {
    m_callback = std::move(c.m_callback);
    m_direct = std::move(c.m_direct);
    m_stateMask = std::move(c.m_stateMask);
    return *this;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class StateInt::D
  {
  public:
    D(int initialState);

    void sendCallbacks(int newState, bool direct);

  public:
    Radiant::Mutex m_stateMutex;
    int m_state;
    int m_nextCallbackId;
    std::weak_ptr<StateInt> m_weak;

    std::map<long, Callback> m_onceCallbacks;
    std::map<long, Callback> m_callbacks;
    std::map<long, Callback> m_changeCallbacks;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  StateInt::D::D(int initialState)
    : m_state(initialState),
      m_nextCallbackId(1)
  {}

  void StateInt::D::sendCallbacks(int newState, bool direct)
  {
    for (auto & p: m_changeCallbacks)
      if (p.second.m_direct == direct)
        p.second.m_callback(newState);

    for (auto it = m_onceCallbacks.begin(); it != m_onceCallbacks.end();) {
      auto & c = it->second;
      if (c.m_direct == direct && ((c.m_stateMask & newState) == newState)) {
        c.m_callback(newState);
        it = m_onceCallbacks.erase(it);
      } else ++it;
    }

    for (auto & p: m_callbacks)
      if (p.second.m_direct == direct && ((p.second.m_stateMask & newState) == newState))
        p.second.m_callback(newState);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  StateInt::StateInt(int initialState)
    : m_d(new D(initialState))
  {}

  StateInt::~StateInt()
  {}

  void StateInt::setWeak(std::weak_ptr<StateInt> weak)
  {
    m_d->m_weak = weak;
  }

  int StateInt::state() const
  {
    return m_d->m_state;
  }

  void StateInt::setState(int state)
  {
    Radiant::Guard g(m_d->m_stateMutex);
    if (m_d->m_state == state)
      return;

    m_d->m_state = state;
    m_d->sendCallbacks(state, true);

    if (m_d->m_onceCallbacks.empty() && m_d->m_callbacks.empty() && m_d->m_changeCallbacks.empty())
      return;

    auto weak = m_d->m_weak;
    Node::invokeAfterUpdate([=] {
      auto self = weak.lock();
      if (!self) return;

      Radiant::Guard g(self->m_d->m_stateMutex);
      self->m_d->sendCallbacks(state, false);
    });
  }

  long StateInt::onChange(StateInt::CallbackType callback, bool direct, bool initialInvoke)
  {
    Radiant::Guard g(m_d->m_stateMutex);
    long id = m_d->m_nextCallbackId++;
    m_d->m_changeCallbacks[id] = Callback(callback, direct, 0);

    if (initialInvoke) {
      if (direct) {
        callback(m_d->m_state);
      } else {
        auto weak = m_d->m_weak;
        Node::invokeAfterUpdate([=] {
          auto self = weak.lock();
          if (!self) return;

          Radiant::Guard g(self->m_d->m_stateMutex);
          callback(m_d->m_state);
        });
      }
    }

    return id;
  }

  long StateInt::onStateMask(int stateMask, StateInt::CallbackType callback, bool once, bool direct)
  {
    Radiant::Guard g(m_d->m_stateMutex);
    int state = m_d->m_state;
    if ((state & stateMask) == state) {
      if (direct) {
        callback(state);
      } else {
        Node::invokeAfterUpdate([=] { callback(state); });
      }
      if (!once) {
        long id = m_d->m_nextCallbackId++;
        m_d->m_callbacks[id] = Callback(callback, direct, stateMask);
        return id;
      }
    } else {
      long id = m_d->m_nextCallbackId++;
      if (once) {
        m_d->m_onceCallbacks[id] = Callback(callback, direct, stateMask);
      } else {
        m_d->m_callbacks[id] = Callback(callback, direct, stateMask);
      }
      return id;
    }
    return 0;
  }

  bool StateInt::removeListener(long id)
  {
    Radiant::Guard g(m_d->m_stateMutex);
    int n = m_d->m_onceCallbacks.erase(id);
    n += m_d->m_callbacks.erase(id);
    n += m_d->m_changeCallbacks.erase(id);
    return n > 0;
  }

} // namespace Valuable
