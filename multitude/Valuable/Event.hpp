#pragma once

#include "WeakNodePtr.hpp"

#include <folly/Executor.h>

#include <QMutex>

namespace Valuable
{
  enum EventFlag {
    NO_FLAGS     = 0,
    /// Event listener is called exactly once
    SINGLE_SHOT  = 1 << 0,
  };
  typedef Radiant::FlagsT<EventFlag> EventFlags;

  /**
   * Type-safe thread-safe event class.
   *
   * You can optionally use folly::Executor to move the listener execution to
   * another thread, but there is no quarantee that the Event instance is alive
   * anymore when the listener is called.
   *
   * It's safe to add and remove listeners inside a listener callback, you can
   * even remove the listener that is currently being called.
   *
   * It's also safe to raise() the event simultaneously from multiple threads
   * while also adding or removing listeners at the same time. Calling raise()
   * can also be called recursively from a listener callback. Calling raise
   * from multiple threads can also mean that a listener callback can be called
   * from multiple threads at the same time. SINGLE_SHOT listeners are not
   * called more than once even when multiple threads call raise() at the same
   * time.
   *
   * The event object is lazily initialized. Creating an event just initializes
   * m_d pointer to zero. Raising an event and destroying an event that doesn't
   * have any listeners does nothing else than just one atomic pointer read.
   * The object is properly initialized only when the first listener is added.
   *
   * Examples:
   *
   * // Simplest case
   * Event<> onChange;
   * onChange.addListener([] { code; });
   * onChange.raise();
   *
   * // Typed arguments
   * Event<Nimble::Vector2f, WidgetPtr> onWidgetCreated;
   * onWidgetCreated.addListener([] (Nimble::Vector2f, WidgetPtr) { code; });
   * onWidgetCreated.raise({1, 2}, WidgetPtr());
   *
   * // Remove listener dynamically inside the listener.
   * onChange.addListener([] { if (code) Event<>::removeCurrentListener(); });
   *
   * // Call this listener only once, even if the event was raised from
   * // multiple threads
   * onChange.addListener(EventFlag::SINGLE_SHOT, [] { code; });
   *
   * // Binding the listener lifetime to Valuable::Node lifetime
   * Valuable::Node node;
   * onChange.addListener(&node, [] { code; });
   *
   * // Custom executor
   * onChange.addListener(Punctual::TaskScheduler::instance()->beforeInput(), [] { code; });
   */
  template <typename... Args>
  class Event
  {
  public:
    using Callback = std::function<void(Args...)>;

    /// Creates a uninitialized event, no memory allocation takes place
    Event() = default;

    /// Delete the event, already dispatched callback calls using custom
    /// executors are not cancelled.
    ~Event();

    /// Add a listener, return a listener id that can be used with removeListener.
    int addListener(Callback callback);

    /// @param receiver if not null, the listener is not called if the receiver is deleted.
    /// It's not safe to delete the receiver node while raise() is being
    /// called in another thread.
    int addListener(Valuable::Node * receiver, Callback callback);

    /// @param executor if not null, the listener is called through this executor.
    int addListener(folly::Executor * executor, Callback callback);

    /// Add a listener with flags.
    int addListener(EventFlags flags, Callback callback);

    /// Add a listener with flags, executor and receiver object. Using both
    /// receiver and executor is only safe if the receiver is deleted only in
    /// the executor thread or if the receiver is deleted before the event is
    /// raised. Both receiver and executor can be null.
    int addListener(EventFlags flags, Valuable::Node * receiver, folly::Executor * executor, Callback callback);

    /// Meant to be called from a event listener to delete the active listener.
    /// Doesn't work with listeners that use custom executors. Works properly
    /// even if raise() is called recursively or from multiple threads at the
    /// same time.
    static void removeCurrentListener();

    /// Remove a previously added listener. Can be called inside a listener
    /// callback, even from the listener that is going to be removed.
    /// @returns false if listener with the given id was not found.
    bool removeListener(int id);

    /// Raise / trigger the event.
    void raise(Args... args);

    /// Returns the number of event listeners this event has.
    uint32_t listenerCount() const;

    Event(const Event &) = delete;
    Event & operator=(const Event &) = delete;

    /// Moving is not thread-safe
    Event(Event &&);
    Event & operator=(Event &&);

  private:
    struct Listener;
    struct D;
    /// We are storing everything here so that sizeof(Event<anything>) is the
    /// size of a pointer. If you have dozens of these in a public API of
    /// MultiWidgets::Widget or similar, we don't want to make the class huge.
    /// Since we initialize this lazily, we can't use std::unique_ptr, since
    /// it doesn't have an atomic setter.
    std::atomic<D*> m_d{nullptr};
  };
}

// Hide the implementation details to keep this file clean
#include "EventImpl.hpp"
