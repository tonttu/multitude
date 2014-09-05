#ifndef VALUABLE_LISTENERHOLDER_HPP
#define VALUABLE_LISTENERHOLDER_HPP

#include "Export.hpp"
#include "Node.hpp"

/// @todo document, make movable and not copyable, or alternatively, make
///       similar functionality part of node (again)

namespace Valuable
{
  class VALUABLE_API ListenerHolder
  {
  public:
    ~ListenerHolder();

    void add(Valuable::Attribute * attr, Valuable::Attribute::ListenerFunc func);
    void add(Valuable::Node * node, const QByteArray & name, Valuable::Node::ListenerFuncVoid func);
    void add(Valuable::Node* fromNode, const QByteArray& from, const QByteArray& to, Valuable::Node* toNode);

  private:
    void setupAttributeRemoveListener(Valuable::Attribute* attr);
    void setupNodeRemoveListener(Valuable::Node* node);

    std::vector<std::pair<Valuable::Attribute *, long>> m_listeners;
    std::vector<std::pair<Valuable::Node *, long>> m_eventListeners;
  };
}

#endif // VALUABLE_LISTENERHOLDER_HPP
