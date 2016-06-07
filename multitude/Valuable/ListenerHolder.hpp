#ifndef VALUABLE_LISTENERHOLDER_HPP
#define VALUABLE_LISTENERHOLDER_HPP

#include "Export.hpp"
#include "Node.hpp"

#include <unordered_map>

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
  /// Not thread safe. Should only be used from the same thread as the target nodes and attributes.
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
    long add(Node * node,
             const QByteArray & name,
             const Node::ListenerFuncBd & func,
             Node::ListenerType listenerType = Node::ListenerType::DIRECT);

  private:
    ListenerHolder(const ListenerHolder&);
    void operator=(const ListenerHolder&);

    void setupRemoveListener(Attribute * attr);
    void attributeGotDeleted(Attribute * attr);

    std::unordered_map<Attribute*, long> m_deleteListeners;
    std::unordered_multimap<Attribute*, long> m_attributeListeners;
    std::unordered_multimap<Node*, long> m_eventListeners;
  };
}

#endif // VALUABLE_LISTENERHOLDER_HPP
