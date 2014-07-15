#ifndef VALUABLE_LISTENERHOLDER_HPP
#define VALUABLE_LISTENERHOLDER_HPP

#include "Attribute.hpp"

/// @todo document, make movable and not copyable, or alternatively, make
///       similar functionality part of node (again)

namespace Valuable
{
  class ListenerHolder
  {
  public:
    void add(Valuable::Attribute * attr, Valuable::Attribute::ListenerFunc func)
    {
      assert(attr);
      m_listeners.push_back(std::make_pair(attr, attr->addListener(func)));
    }

    void add(Valuable::Node * node, const QByteArray & name, Valuable::Node::ListenerFuncVoid func)
    {
      assert(node);
      m_eventListeners.push_back(std::make_pair(node, node->eventAddListener(name, func)));
    }

    ~ListenerHolder()
    {
      for (auto p: m_listeners)
        p.first->removeListener(p.second);

      for (auto p: m_eventListeners)
        p.first->eventRemoveListener(p.second);
    }

  private:
    std::vector<std::pair<Valuable::Attribute *, long>> m_listeners;
    std::vector<std::pair<Valuable::Node *, long>> m_eventListeners;
  };
}

#endif // VALUABLE_LISTENERHOLDER_HPP
