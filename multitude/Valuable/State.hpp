/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_STATE_HPP
#define VALUABLE_STATE_HPP

#include "Export.hpp"
#include "Node.hpp"

#include <memory>

namespace Valuable
{
  /// Content-loading states
  enum LoadingEnum {
    /// Default object state when loading has not yet been initiated
    STATE_NEW             = 1 << 0,
    /// Loading has been initiated
    STATE_LOADING         = 1 << 1,
    /// Content meta-data, including size has been loaded
    STATE_HEADER_READY    = 1 << 2,
    /// Enough content has been loaded to render
    STATE_READY           = 1 << 3,
    /// An error occured during loading
    STATE_ERROR           = 1 << 4
  };

  /// @cond

  /// Implementation class for Valuable::State.
  class VALUABLE_API StateInt
  {
  public:
    typedef std::function<void(int, int)> CallbackType;

  public:
    StateInt(int initialState);
    ~StateInt();

    void setWeak(std::weak_ptr<StateInt> weak);

    int state() const;
    void setState(int state);

    long onChange(CallbackType callback, bool direct, bool initialInvoke);
    long onStateMask(int state, CallbackType callback, bool once, bool direct);

    bool removeListener(long id);

    int generation() const;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };

  /// @endcond

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /// State machine that doesn't define any transitions or triggers, but
  /// implements thread-safe access and monitoring callbacks. It's basically
  /// just an enum wrapper.
  /// It is important that the Enum values form a bitmask, values must not
  /// share any bits. See LoadingEnum as an example.
  /// This class guarantees that all "once" callbacks are called exactly once,
  /// and other callbacks called only when the state is changed to any of the
  /// monitored states. All callbacks are called in right order.
  template <typename Enum>
  class State
  {
  public:
    /// Type of the callback used in monitor functions, new state is given as a first parameter
    typedef std::function<void(Enum, int)> CallbackType;

  public:
    /// Constructs a new State object
    /// @param initialState enum value used to initialize the state
    State(Enum initialState);

    /// Move constructor, not thread-safe
    State(State && state);
    /// Move assignment operator, not thread-safe
    inline State & operator=(State && state);

    /// Reads the current state.
    /// This function is thread-safe
    inline Enum state() const;
    /// Sets the current state and triggers all necessary callbacks.
    /// This function is thread-safe
    inline void setState(Enum state);

    /// Implicit conversion to the wrapped enum value, same as State::state().
    /// This function is thread-safe
    inline operator Enum() const;
    /// Same as State::setState(Enum), thread-safe
    inline State<Enum> & operator=(Enum value);

    /// Adds a monitor to the state.
    /// This function is thread-safe
    /// @param callback called when ever the state changes
    /// @param type Listener type, allowed values: DIRECT or AFTER_UPDATE
    /// @param initialInvoke call the callback once in the beginning with the current state
    /// @returns a listener id
    inline long onChange(CallbackType callback, Node::ListenerType type = Node::AFTER_UPDATE,
                         bool initialInvoke = false);

    /// Adds a monitor to the state with given stateMask. If the current state
    /// already matches to the monitored state, the callback is triggered
    /// immediately, still honoring the given listener type.
    /// This function is thread-safe
    /// @param stateMask callback is triggered only when the new state is
    ///        included in stateMask. you can monitor multiple values by using
    ///        bitwise or operation, for example HEADER | HEADER_READY.
    /// @param callback called when ever the state changes to a monitored state
    /// @param once should the callback be removed after it has been called once
    /// @param type Listener type, allowed values: DIRECT or AFTER_UPDATE
    /// @returns a listener id
    inline long onStateMask(int stateMask, CallbackType callback, bool once = true,
                            Node::ListenerType type = Node::AFTER_UPDATE);

    /// Removes a listener from the state.
    /// This function is thread-safe
    /// @param id listener id, returned from onChange or onStateMask functions
    /// @returns true if the listener was removed
    inline bool removeListener(long id);

    /// @returns how many times state has changed
    inline int generation() const;

  private:
    std::shared_ptr<StateInt> m_state;
  };

  /// Commonly used state
  typedef State<LoadingEnum> LoadingState;

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <typename Enum>
  State<Enum>::State(Enum initialState)
    : m_state(std::make_shared<StateInt>(initialState))
  {
    m_state->setWeak(m_state);
  }

  template <typename Enum>
  State<Enum>::State(State && state)
    : m_state(std::move(state.m_state))
  {}

  template <typename Enum>
  State<Enum> & State<Enum>::operator=(State && state)
  {
    std::swap(m_state, state.m_state);
    return *this;
  }

  template <typename Enum>
  Enum State<Enum>::state() const
  {
    return (Enum)m_state->state();
  }

  template <typename Enum>
  void State<Enum>::setState(Enum state)
  {
    m_state->setState(state);
  }

  template <typename Enum>
  State<Enum>::operator Enum() const
  {
    return state();
  }

  template <typename Enum>
  State<Enum> & State<Enum>::operator=(Enum value)
  {
    setState(value);
    return *this;
  }

  template <typename Enum>
  long State<Enum>::onChange(CallbackType callback, Node::ListenerType type, bool initialInvoke)
  {
    return m_state->onChange([=] (int v, int g) { callback(Enum(v), g); }, type == Node::DIRECT, initialInvoke);
  }

  template <typename Enum>
  long State<Enum>::onStateMask(int stateMask, CallbackType callback, bool once, Node::ListenerType type)
  {
    return m_state->onStateMask(stateMask, [=] (int v, int g) { callback(Enum(v), g); }, once, type == Node::DIRECT);
  }

  template <typename Enum>
  bool State<Enum>::removeListener(long id)
  {
    return m_state->removeListener(id);
  }

  template <typename Enum>
  int State<Enum>::generation() const
  {
    return m_state->generation();
  }
} // namespace Valuable

#endif // VALUABLE_STATE_HPP
