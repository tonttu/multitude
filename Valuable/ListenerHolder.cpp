/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "ListenerHolder.hpp"
#include <utility>

namespace Valuable
{
  ListenerHolder::ListenerHolder() {  }

  ListenerHolder::ListenerHolder(ListenerHolder && other) noexcept
  {
    Radiant::Guard guard(other.m_mutex);
    swap(m_deleteListeners, other.m_deleteListeners);
    swap(m_attributeListeners, other.m_attributeListeners);
    swap(m_eventListeners, other.m_eventListeners);
  }

  ListenerHolder & ListenerHolder::operator=(ListenerHolder && other) noexcept
  {
    using std::swap;
    if(&other == this) {
      return *this;
    }
    Radiant::Mutex * low = nullptr, * high = nullptr;
    if(&other < this) {
      low = &other.m_mutex;
      high = &this->m_mutex;
    } else {
      low = &this->m_mutex;
      high = &other.m_mutex;
    }
    Radiant::Guard guardLow(*low);
    Radiant::Guard guardHigh(*high);
    swap(m_deleteListeners, other.m_deleteListeners);
    swap(m_attributeListeners, other.m_attributeListeners);
    swap(m_eventListeners, other.m_eventListeners);
    return *this;
  }

  ListenerHolder::~ListenerHolder()
  {
    Radiant::Guard guard(m_mutex);
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
    for (const auto & pair: m_listeners) {
      AttributeEventListenerList & list = *pair.first;
      const ListenerInfo & info = pair.second;
      list.removeListener(info.deleteListener);
      for (auto id: info.listeners) {
        list.removeListener(id);
      }
    }
  }

  long ListenerHolder::add(Attribute * attr, const Attribute::ListenerFunc & func, int role)
  {
    assert(attr);
    long id = attr->addListener(func, role);
    Radiant::Guard guard(m_mutex);
    m_attributeListeners.insert(std::make_pair(attr, id));
    setupRemoveListener(attr);
    return id;
  }

  long ListenerHolder::add(Node * node, const QByteArray & name, const Node::ListenerFuncVoid & func, Node::ListenerType listenerType)
  {
    assert(node);
    long id = node->eventAddListener(name, func, listenerType);
    Radiant::Guard guard(m_mutex);
    m_eventListeners.insert(std::make_pair(node, id));
    setupRemoveListener(node);
    return id;
  }

  long ListenerHolder::addBd(Node * node, const QByteArray & name, const Node::ListenerFuncBd & func, Node::ListenerType listenerType)
  {
    assert(node);
    long id = node->eventAddListenerBd(name, func, listenerType);
    Radiant::Guard guard(m_mutex);
    m_eventListeners.insert(std::make_pair(node, id));
    setupRemoveListener(node);
    return id;
  }

  void ListenerHolder::removeListeners(Attribute * attr)
  {
    assert(attr);
    Radiant::Guard guard(m_mutex);
    auto itDel = m_deleteListeners.find(attr);
    if(itDel != m_deleteListeners.end()) {
      attr->removeListener(itDel->second);
      m_deleteListeners.erase(itDel);
    }
    auto attrRange = m_attributeListeners.equal_range(attr);
    for(auto it = attrRange.first; it != attrRange.second; ++it) {
      attr->removeListener(it->second);
    }
    m_attributeListeners.erase(attr);
    Node * node = static_cast<Node*>(attr);
    auto eventRange = m_eventListeners.equal_range(node);
    for(auto it = eventRange.first; it != eventRange.second; ++it) {
      node->eventRemoveListener(it->second);
    }
    m_eventListeners.erase(node);
  }

  AttributeEvent::ListenerId ListenerHolder::addListener(AttributeEventListenerList & list, AttributeEvent::Types types,
                                                AttributeEventListenerList::EventListenerFunc listener)
  {
    auto id = list.addListener(types, std::move(listener));
    auto key = &list;

    Radiant::Guard guard(m_mutex);
    auto & info = m_listeners[key];
    info.listeners.insert(id);

    if (info.deleteListener == 0) {
      info.deleteListener = list.addListener(AttributeEvent::Type::DELETED, [this, key] (AttributeEvent) {
        Radiant::Guard guard(m_mutex);
        m_listeners.erase(key);
      });
    }
    return id;
  }

  bool ListenerHolder::removeListener(AttributeEventListenerList & list, AttributeEvent::ListenerId listener)
  {
    bool ok = list.removeListener(listener);
    auto key = &list;

    Radiant::Guard guard(m_mutex);
    auto & info = m_listeners[key];
    info.listeners.erase(listener);

    if (info.listeners.empty()) {
      if (info.deleteListener != 0) {
        list.removeListener(info.deleteListener);
      }
      m_listeners.erase(key);
    }
    return ok;
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
    Radiant::Guard guard(m_mutex);
    m_deleteListeners.erase(attr);
    m_attributeListeners.erase(attr);
    // Safe to downcast. If the attr is not a node, it will not be found in m_eventListeners and nothing
    // will happen. If it is found, we know it's a Node.
    m_eventListeners.erase(static_cast<Node*>(attr));
  }
}
