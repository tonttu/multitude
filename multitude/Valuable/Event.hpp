#pragma once

#include "Export.hpp"

#include <Radiant/Flags.hpp>

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
   * // Call this listener only once, even if the event was raised from
   * // multiple threads
   * onChange.addListener(EventFlag::SINGLE_SHOT, [] { code; });
   *
   * // Binding the listener lifetime to any std::shared_ptr. The listener is
   * // not called if the shared pointer is deleted. If the shared pointer is
   * // alive, then it's also kept alive during the callback call, so this is
   * // thread-safe, regardless of where the receiver is deleted.
   * onChange.addListener(weak_from_this(), [this] { this->code; });
   *
   * // Binding the listener lifetime to Valuable::Node lifetime. Since
   * // Valuable::Node lifetime is not managed through this internal sharedPtr,
   * // this is not thread-safe if Node is deleted in a different thread
   * // from where the callback is called.
   * Valuable::Node node;
   * onChange.addListener(node.sharedPtr(), [] { code; });
   *
   * // Binding the listener lifetime to a specific Reference object. If your
   * // event receiver is not a Valuable::Node or shared_ptr, you can use this
   * // for explicit listener lifetime management. The listeners are valid as
   * // long as the Reference object is alive.
   * Valuable::Reference listeners;
   * onChange.addListener(listeners.weak(), [] { code; });
   *
   * // Listeners with receivers can also be explicitly removed
   * onChange.removeListeners(weak_from_this());
   * onChange.removeListeners(listeners.weak());
   *
   * // Custom executor
   * onChange.addListener(Punctual::beforeInput(), [] { code; });
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

    /// Add a listener, return a non-negative listener id that can be used with
    /// removeListener.
    int addListener(Callback callback);

    /// @param executor if not null, the listener is called through this executor.
    int addListener(folly::Executor * executor, Callback callback);

    /// @param receiver the listener is not called if the receiver is deleted.
    /// See the class documentation for examples and how to use this with
    /// Valuable::Node and Valuable::Reference. Note that if receiver is nullptr
    /// or points to a object that has already been deleted, the callback is
    /// never called.
    ///
    /// If the receiver object gets deleted, it doesn't automatically delete
    /// the listener, it just won't get called anymore. The listener, including
    /// the callback function with everything it has captured, will get deleted
    /// properly next time raise() is called, or when removeListener(s) is called.
    int addListener(std::weak_ptr<void> receiver, Callback callback);
    int addListener(std::weak_ptr<void> receiver, folly::Executor * executor, Callback callback);

    /// Add a listener with flags.
    int addListener(EventFlags flags, Callback callback);
    int addListener(EventFlags flags, folly::Executor * executor, Callback callback);
    int addListener(EventFlags flags, std::weak_ptr<void> receiver, Callback callback);
    int addListener(EventFlags flags, std::weak_ptr<void> receiver, folly::Executor * executor, Callback callback);

    /// Remove a previously added listener. Can be called inside a listener
    /// callback, even from the listener that is going to be removed.
    /// @returns false if listener with the given id was not found.
    bool removeListener(int id);

    /// Remove all listeners with the given receiver. Can be called inside
    /// a listener callback, even from the listener that is going to be removed.
    /// This is a template function so that there's no need to copy/convert the
    /// receiver to std::weak_ptr<void>.
    /// @returns the number of listeners removed
    template <typename T>
    int removeListeners(const std::weak_ptr<T> & receiver);
    template <typename T>
    int removeListeners(const std::shared_ptr<T> & receiver);

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
