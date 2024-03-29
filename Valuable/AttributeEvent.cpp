/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "AttributeEvent.hpp"

#include <set>

namespace Valuable
{
  AttributeEventListenerList::ListenerId AttributeEventListenerList::addListener(AttributeEvent::Types types, EventListenerFunc listener)
  {
    auto id = m_nextListenerId++;
    m_eventListeners[id] = EventListener{types, listener};
    return id;
  }

  bool AttributeEventListenerList::removeListener(ListenerId listener)
  {
    if (m_eventListeners.erase(listener)) {
      ++m_nextListenerId;
      return true;
    }
    return false;
  }

  void AttributeEventListenerList::send(AttributeEvent::Type type, std::size_t index)
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
        if (it->first < listenerGenerationLimit && (it->second.types & type) && sent.insert(it->first).second) {
          // Need to take copy of the function, otherwise calling
          // Event::removeListener inside the lambda will crash the application
          auto func = it->second.func;
          func(AttributeEvent(this, it->first, type, index));

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
