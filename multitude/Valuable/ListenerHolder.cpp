#include "ListenerHolder.hpp"
#include <utility>

namespace Valuable
{
  ListenerHolder::ListenerHolder() {  }

  ListenerHolder::ListenerHolder(ListenerHolder && other) noexcept
    : m_deleteListeners(std::move(other.m_deleteListeners)),
      m_attributeListeners(std::move(other.m_attributeListeners)),
      m_eventListeners(std::move(other.m_eventListeners)) { }

  ListenerHolder & ListenerHolder::operator=(ListenerHolder && other) noexcept
  {
    using std::swap;
    if(&other == this) {
      return *this;
    }
    swap(m_deleteListeners, other.m_deleteListeners);
    swap(m_attributeListeners, other.m_attributeListeners);
    swap(m_eventListeners, other.m_eventListeners);
    return *this;
  }

  ListenerHolder::~ListenerHolder()
  {
    for(const auto & pair : m_deleteListeners) {
      Attribute * attr = pair.first;
      long id = pair.second;
      attr->removeListener(id);
    }
    for(const auto & pair : m_attributeListeners) {
      Attribute * attr = pair.first;
      long id = pair.second;
      attr->removeListener(id);
    }
    for(const auto & pair : m_eventListeners) {
      Node * node = pair.first;
      long id = pair.second;
      node->eventRemoveListener(id);
    }
  }

  long ListenerHolder::add(Attribute * attr, const Attribute::ListenerFunc & func, int role)
  {
    assert(attr);
    long id = attr->addListener(func, role);
    m_attributeListeners.insert(std::make_pair(attr, id));
    setupRemoveListener(attr);
    return id;
  }

  long ListenerHolder::add(Node * node, const QByteArray & name, const Node::ListenerFuncVoid & func, Node::ListenerType listenerType)
  {
    assert(node);
    long id = node->eventAddListener(name, func, listenerType);
    m_eventListeners.insert(std::make_pair(node, id));
    setupRemoveListener(node);
    return id;
  }

  long ListenerHolder::add(Node * node, const QByteArray & name, const Node::ListenerFuncBd & func, Node::ListenerType listenerType)
  {
    assert(node);
    long id = node->eventAddListenerBd(name, func, listenerType);
    m_eventListeners.insert(std::make_pair(node, id));
    setupRemoveListener(node);
    return id;
  }

  void ListenerHolder::setupRemoveListener(Attribute * attr)
  {
    if(m_deleteListeners.find(attr) == m_deleteListeners.end()) {
      long id = attr->addListener([attr, this] {
        attributeGotDeleted(attr);
      }, Attribute::DELETE_ROLE);
      m_deleteListeners[attr] = id;
    }
  }

  void ListenerHolder::attributeGotDeleted(Attribute * attr)
  {
    m_deleteListeners.erase(attr);
    m_attributeListeners.erase(attr);
    // Safe to downcast. If the attr is not a node, it will not be found in m_eventListeners and nothing
    // will happen. If it is found, we know it's a Node.
    m_eventListeners.erase(static_cast<Node*>(attr));
  }
}
