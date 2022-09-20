/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef VALUABLE_LISTENERHOLDER_HPP
#define VALUABLE_LISTENERHOLDER_HPP

#include "Export.hpp"
#include "Node.hpp"
#include "AttributeEvent.hpp"
#include "AttributeVectorContainer.hpp"

#include <unordered_map>
#include <Radiant/Mutex.hpp>

namespace Valuable
{
  /// Provides lifetime management for listeners. This is useful when a short lived class subscribes to events.
  /// If the listeners are not removed when the short lived class dies, the callbacks will access freed memory.
  /// If the listeners are added through the ListenerHolder, their lifetime is properly managed no matter who
  /// dies first: the sender or the receiver.
  ///
  /// If the ListenerHolder dies, all the listeners added to it get unregistered.
  ///
  /// If the target Node or Attribute dies, ListenerHolder does not try to unregister events for those objects.
  ///
  /// Thread safe but could use a bit more granular locks.
  ///
  /// Example:
  /// @code
  /// void MyClass::addWidget(WidgetPtr widget)
  /// {
  ///   // Add a listener to rotation attribute. The listener is automatically
  ///   // removed when 'this' is deleted, since m_listenerHolder is a member
  ///   // variable.
  ///   m_listenerHolder.add(widget->attribute("rotation"), [] {
  ///     Radiant::info("Rotation changed!");
  ///   });
  /// }
  /// @endcode
  class VALUABLE_API ListenerHolder
  {
  public:
    ListenerHolder();
    ListenerHolder(ListenerHolder && other) noexcept;
    ListenerHolder& operator=(ListenerHolder && other) noexcept;

    ~ListenerHolder();

    /// Adds an attribute listener. Calls attr->addListener(func, role).
    /// @sa Valuable::Attribute::addListener
    /// @param attr Attribute where the listener is added, i.e. sender
    /// @param func Listener callback function
    /// @param role Listened events
    /// @returns event id that Attribute::addListener returns
    /// @todo should this be renamed to addListener, and the others to eventAddListener?
    long add(Attribute * attr,
             const Attribute::ListenerFunc & func,
             int role = Attribute::CHANGE_ROLE);

    /// Adds a node event listener. Calls node->eventAddListener(name, func, listenerType).
    /// @sa Valuable::Node::eventAddListener
    /// @param node Node where the listener is added, i.e. sender
    /// @param name Listened event name
    /// @param func Listener callback function
    /// @param listenerType How the listener callback function is triggered
    /// @returns event id that Node::eventAddListener returns
    long add(Node * node,
             const QByteArray & name,
             const Node::ListenerFuncVoid & func,
             Node::ListenerType listenerType = Node::ListenerType::DIRECT);

    /// Adds a BinaryData node event listener. Calls node->eventAddListener(name, func, listenerType).
    /// This is similar to the previous function version, but uses different
    /// callback type.
    /// @sa Valuable::Node::eventAddListener
    /// @param node Node where the listener is added, i.e. sender
    /// @param name Listened event name
    /// @param func Listener callback function
    /// @param listenerType How the listener callback function is triggered
    /// @returns event id that Node::eventAddListener returns
    long addBd(Node * node,
               const QByteArray & name,
               const Node::ListenerFuncBd & func,
               Node::ListenerType listenerType = Node::ListenerType::DIRECT);

    void removeListeners(Attribute * attr);

    /// Add a new listeners to EventListenerList
    /// @param list Object that we are listening to
    /// See EventListenerList::addListener for more information about other parameters
    AttributeEvent::ListenerId addListener(AttributeEventListenerList & list, AttributeEvent::Types types,
                                  AttributeEventListenerList::EventListenerFunc listener);

    /// Add a new listeners to AttributeContainer
    /// @param container Container that we are listening to
    /// See EventListenerList::addListener for more information about other parameters
    inline AttributeEvent::ListenerId addListener(AttributeContainerBase & container, AttributeEvent::Types types,
                                         AttributeEventListenerList::EventListenerFunc listener)
    {
      return addListener(container.eventListenerList(), types, std::move(listener));
    }

    /// Remove a listener from EventListenerList
    bool removeListener(AttributeEventListenerList & list, AttributeEvent::ListenerId listener);

    /// Remove a listener from AttributeContainer
    inline bool removeListener(AttributeContainerBase & container, AttributeEvent::ListenerId listener)
    {
      return removeListener(container.eventListenerList(), listener);
    }

  private:
    ListenerHolder(const ListenerHolder&);
    void operator=(const ListenerHolder&);

    void setupRemoveListener(Attribute * attr);
    void attributeGotDeleted(Attribute * attr);

    Radiant::Mutex m_mutex;
    std::unordered_map<Attribute*, long> m_deleteListeners;
    std::unordered_multimap<Attribute*, long> m_attributeListeners;
    std::unordered_multimap<Node*, long> m_eventListeners;

    /// All Listeners for one EventListenerList
    struct ListenerInfo
    {
      AttributeEvent::ListenerId deleteListener = 0;
      std::set<AttributeEvent::ListenerId> listeners;
    };

    std::unordered_map<AttributeEventListenerList*, ListenerInfo> m_listeners;
  };
}

#endif // VALUABLE_LISTENERHOLDER_HPP
