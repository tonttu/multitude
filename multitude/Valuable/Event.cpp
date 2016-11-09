#include "Event.hpp"

#include <set>

namespace Valuable
{
  EventListenerList::ListenerId EventListenerList::addListener(Event::Flags flags, EventListenerFunc listener)
  {
    auto id = m_nextListenerId++;
    m_eventListeners[id] = EventListener{flags, listener};
    return id;
  }

  bool EventListenerList::removeListener(ListenerId listener)
  {
    if (m_eventListeners.erase(listener)) {
      ++m_nextListenerId;
      return true;
    }
    return false;
  }

  void EventListenerList::send(Event::Type type, std::size_t index)
  {
    // Only call listeners that have lower listener id than this to exclude
    // listeners that were added during this function call
    const auto listenerGenerationLimit = m_nextListenerId;

    // Use this to check if any listeners were added or removed during this function call
    auto listenerGeneration = m_nextListenerId;

    /// @todo This needs some heap allocation, similar to std::function
    ///       copying in the loop. We could use custom stack-based allocator
    ///       here to make it faster.
    std::set<ListenerId> sent;

    for (;;) {
      bool done = true;
      // It's safer to use iterators instead of range-based for loop, since
      // we might be modifying m_eventListeners inside the event handlers
      for (auto it = m_eventListeners.begin(), end = m_eventListeners.end(); it != end; ++it) {
        // Call the event listener if it matches the event type and hasn't been called yet
        if (it->first < listenerGenerationLimit && (it->second.flags & type) && sent.insert(it->first).second) {
          // Need to take copy of the function, otherwise calling
          // Event::removeListener inside the lambda will crash the application
          auto func = it->second.func;
          func(Event(this, it->first, type, index));

          // If this event handler modified the listener list, our iterator has been invalidated
          // and we need to start from the beginning
          if (listenerGeneration != m_nextListenerId) {
            listenerGeneration = m_nextListenerId;
            done = false;
            break;
          }
        }
      }

      if (done) {
        break;
      }
    }
  }
} // namespace Valuable
