#include "WeakNodePtr.hpp"

#include <folly/Executor.h>

namespace Valuable
{
  /**
   * Type-safe event class.
   *
   * This class is not thread-safe.
   *
   * You can optionally use folly::Executor to move the listener execution to
   * another thread, but there is no quarantee that the Event instance is alive
   * anymore when the listener is called.
   *
   * It's safe to add and remove listeners inside a listener callback, you can
   * even remove the listener that is currently being called.
   *
   * Examples:
   *
   * // Simplest case
   * Event onChange;
   * onChange.addListener([] { code; });
   * onChange.raise();
   *
   * // Typed arguments
   * Event<Nimble::Vector2f, WidgetPtr> onWidgetCreated;
   * onWidgetCreated.addListener([] (Nimble::Vector2f, WidgetPtr) { code; });
   * onWidgetCreated.raise({1, 2}, WidgetPtr());
   *
   * // Call the listener just once
   * onChange.addListener([&onChange] { code; onChange.removeCurrentListener(); });
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
    using Cb = std::function<void(Args...)>;

    /// Add a listener, return a listener id that can be used with removeListener.
    int addListener(Cb cb);

    /// @param receiver if not null, the listener is not called if the receiver is deleted.
    int addListener(Valuable::Node * receiver, Cb cb);

    /// @param executor if not null, the listener is called through this executor.
    int addListener(folly::Executor * executor, Cb cb);

    /// Add a listener with custom executor and receiver object. This is only
    /// safe if the receiver is deleted only in the executor thread or if the
    /// receiver is deleted before the event is raised.
    int addListener(Valuable::Node * receiver, folly::Executor * executor, Cb cb);

    /// Meant to be called from a event listener to delete the active listener.
    /// Doesn't work with listeners that use custom executors.
    void removeCurrentListener();

    /// Remove a previously added listener. Can be called inside a listener
    /// callback, even from the listener that is going to be removed.
    /// @returns false if listener with the given id was not found.
    bool removeListener(int id);

    /// Raise / trigger the event.
    void raise(Args... args);

  private:
    struct Listener;
    struct D;
    /// We are storing everything here so that sizeof(Event<anything>) is the
    /// size of a pointer. If you have dozens of these in a public API of
    /// MultiWidgets::Widget or similar, we don't want to make the class huge.
    std::unique_ptr<D> m_d;
  };
}

// Hide the implementation details to keep this file clean
#include "EventImpl.hpp"
