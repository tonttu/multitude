#include "ListenerHolder.hpp"

namespace Valuable
{

  ListenerHolder::~ListenerHolder()
  {
    for (auto p: m_listeners)
      p.first->removeListener(p.second);

    for (auto p: m_eventListeners)
      p.first->eventRemoveListener(p.second);
  }


  void ListenerHolder::add(Valuable::Attribute *attr, Valuable::Attribute::ListenerFunc func)
  {
    assert(attr);
    m_listeners.push_back(std::make_pair(attr, attr->addListener(func)));

    setupAttributeRemoveListener(attr);
  }

  void ListenerHolder::add(Valuable::Node *node, const QByteArray &name, Valuable::Node::ListenerFuncVoid func)
  {
    assert(node);
    m_eventListeners.push_back(std::make_pair(node, node->eventAddListener(name, func)));

    setupNodeRemoveListener(node);
  }

  void ListenerHolder::add(Valuable::Node *fromNode, const QByteArray &from, const QByteArray &to, Valuable::Node *toNode)
  {
    assert(fromNode);
    assert(toNode);
    m_eventListeners.push_back(std::make_pair(fromNode, fromNode->eventAddListener(from, to, toNode)));

    setupNodeRemoveListener(fromNode);
  }

  void ListenerHolder::setupAttributeRemoveListener(Valuable::Attribute *attr)
  {
    auto rmFunc = [this, attr] {

      auto it = std::remove_if(m_listeners.begin(), m_listeners.end(), [attr] (const std::pair<Valuable::Attribute*, long> & element)
      {
        return element.first == attr;
      });

      m_listeners.erase(it, m_listeners.end());
    };

    auto id = attr->addListener(rmFunc, Attribute::DELETE_ROLE);
    m_listeners.push_back(std::make_pair(attr, id));
  }

  void ListenerHolder::setupNodeRemoveListener(Valuable::Node *node)
  {
    auto rmFunc = [this, node] {

      auto it = std::remove_if(m_eventListeners.begin(), m_eventListeners.end(), [node] (const std::pair<Valuable::Node*, long>& element)
      {
        return element.first == node;
      });

      m_eventListeners.erase(it, m_eventListeners.end());
    };

    auto id = node->addListener(rmFunc, Attribute::DELETE_ROLE);
    m_listeners.push_back(std::make_pair(node, id));
  }

}
