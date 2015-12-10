#ifndef VALUABLE_LISTENERHOLDER_HPP
#define VALUABLE_LISTENERHOLDER_HPP

#include "Export.hpp"
#include "Node.hpp"

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
  class VALUABLE_API ListenerHolder
  {
  public:
    ListenerHolder();
    ListenerHolder(ListenerHolder && other) noexcept;
    ListenerHolder& operator=(ListenerHolder && other) noexcept;

    ~ListenerHolder();

    /// Adds an attribute listener
    long add(Attribute * attr,
             const Attribute::ListenerFunc & func,
             int role = Attribute::CHANGE_ROLE);

    /// Adds a node event listener
    long add(Node * node,
             const QByteArray & name,
             const Node::ListenerFuncVoid & func,
             Node::ListenerType listenerType = Node::ListenerType::DIRECT);

    /// Adds a BinaryData node event listener
    long add(Node * node,
             const QByteArray & name,
             const Node::ListenerFuncBd & func,
             Node::ListenerType listenerType = Node::ListenerType::DIRECT);

    void removeListeners(Attribute * attr);

  private:
    ListenerHolder(const ListenerHolder&);
    void operator=(const ListenerHolder&);

    void setupRemoveListener(Attribute * attr);
    void attributeGotDeleted(Attribute * attr);

    Radiant::Mutex m_mutex;
    std::unordered_map<Attribute*, long> m_deleteListeners;
    std::unordered_multimap<Attribute*, long> m_attributeListeners;
    std::unordered_multimap<Node*, long> m_eventListeners;
  };
}

#endif // VALUABLE_LISTENERHOLDER_HPP
