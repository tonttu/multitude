#ifndef VALUABLE_EVENT_HPP
#define VALUABLE_EVENT_HPP

#include "Export.hpp"

#include <Radiant/Flags.hpp>

#include <functional>
#include <map>

namespace Valuable
{
  class EventListenerList;

  /// Event that is sent to event listeners when Attribute changes
  class Event
  {
  public:
    /// Event type that also works as a bitmask
    enum class Type : uint32_t
    {
      DELETED           = 1 << 0,   ///< Attribute is being deleted
      CHANGED           = 1 << 1,   ///< Attribute value changed or all attribute container elements were replaced
      HOST_CHANGED      = 1 << 2,   ///< Attribute host was changed

      ELEMENT_INSERTED  = 1 << 3,   ///< One element was inserted to the attribute container
      ELEMENT_ERASED    = 1 << 4,   ///< One element was erased from the attribute container
      ELEMENT_CHANGED   = 1 << 5,   ///< One element was changed in the attribute container

      ALL_EVENTS        = (uint32_t)-1
    };
    typedef Radiant::FlagsT<Event::Type> Types;
    typedef uint64_t ListenerId;

  public:
    /// Removes the event handler that received this event. Can be called inside the event handler.
    inline void removeListener();

    /// Event type
    inline Type type() const { return m_type; }

    /// Element index. This is only defined for ELEMENT_* event types
    inline std::size_t index() const { return m_index; }

    /// @cond

    Event(EventListenerList * listenerList, ListenerId listenerId, Type type, std::size_t index = 0)
      : m_listenerList(listenerList)
      , m_listenerId(listenerId)
      , m_type(type)
      , m_index(index)
    {}

    /// @endcond

  private:
    EventListenerList * m_listenerList;
    ListenerId m_listenerId;
    Type m_type;
    std::size_t m_index;
  };
  MULTI_FLAGS(Event::Type);

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /// Utility class for distributing the changes in Attributes to the listeners
  class VALUABLE_API EventListenerList
  {
  public:
    typedef Event::ListenerId ListenerId;
    typedef std::function<void(Event)> EventListenerFunc;

  public:
    EventListenerList() {}
    ~EventListenerList() {}

    /// Adds a new event listener for selected event types. Can be called
    /// from an event listener callback.
    /// @param types event types to listen
    /// @param listener listener callback which is invoked when any of the events happen
    /// @returns listener id which can be used to remove the listener with removeListener
    ListenerId addListener(Event::Types types, EventListenerFunc listener);

    /// Remove an event listener. Can be called from an event listener callback
    /// @param listener Listener id returned by addListener
    /// @returns true if listener was found and removed
    bool removeListener(ListenerId listener);

    /// Sends a new event to listeners. Can be called from an event listener callback
    /// @param type event type
    /// @param index see Event::index
    void send(Event::Type type, std::size_t index = 0);

  private:
    ListenerId m_nextListenerId = 1;

    struct EventListener
    {
      Event::Types types;
      EventListenerFunc func;
    };

    std::map<ListenerId, EventListener> m_eventListeners;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  void Event::removeListener()
  {
    m_listenerList->removeListener(m_listenerId);
  }

} // namespace Valuable

#endif // VALUABLE_EVENT_HPP
