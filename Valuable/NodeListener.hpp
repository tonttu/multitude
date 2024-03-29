/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#pragma once

#include "Node.hpp"
#include "Event.hpp"

#include <Punctual/Executors.hpp>

namespace Valuable
{
  /// Helper class for monitoring when a Node gets (re)created and when any of
  /// its attributes change. The template parameter must match the type of the
  /// leaf node in the search string. For instance:
  ///
  /// NodeListener<MyClass> listener;
  /// listener.onChange.addListener(...);
  /// listener.monitor(root, "names/of/nodes/before/my_class");
  template <typename N = Node>
  class NodeListener
  {
  public:
    /// Event that is raised when the target node is found. It's also raised
    /// with nullptr when the previously found node is lost due to the node
    /// or any of its parents getting deleted or reparented.
    Event<N*> onChange;

  public:
    /// Remove all listeners
    void reset()
    {
      m_listenerOwners.clear();
      nodeLost();
    }

    /// Starts monitoring a node with the given search pattern. Only one
    /// pattern can be monitored at a time. If the node can be found
    /// immediately, onChange is raised during this function call. Otherwise
    /// this uses AFTER_UPDATE events internally, since typically
    /// attribute-added -events are triggered when the added node or attribute
    /// is still being constructed in Attribute constructor.
    /// @param path Search pattern that consists of node names separated by '/'.
    void monitor(Node & root, const QByteArray & path)
    {
      reset();
      monitor(&root, path.split('/'), 0);
    }

  private:
    void nodeLost()
    {
      if (m_found) {
        m_found = false;
        onChange.raise(nullptr);
      }
    }

    void monitor(Node * node, const QByteArrayList & path, int depth)
    {
      m_listenerOwners.resize(depth);

      for (; depth < path.size(); ++depth) {
        const QByteArray name = path[depth];

        m_listenerOwners.emplace_back(new Node());
        Node * owner = m_listenerOwners.back().get();

        node->onAttributeAdded.addListener(owner->sharedPtr(), Punctual::afterUpdate(),
                                           [this, path, name, depth] (Valuable::Attribute * attr) {
          if (name != attr->name())
            return;

          Node * child = dynamic_cast<Node*>(attr);
          if (child)
            monitor(child, path, depth + 1);
        });

        node->onAttributeRemoved.addListener(owner->sharedPtr(), [this, name, depth] (Valuable::Attribute * attr) {
          if (name != attr->name())
            return;

          m_listenerOwners.resize(depth + 1);
          nodeLost();
        });

        Node * child = dynamic_cast<Node*>(node->attribute(name));
        if (child) {
          node = child;
        } else {
          nodeLost();
          return;
        }
      }

      N * result = dynamic_cast<N*>(node);
      if (!result) {
        nodeLost();
        return;
      }

      m_listenerOwners.emplace_back(new Node());
      Node * owner = m_listenerOwners.back().get();

      node->onAttributeAdded.addListener(owner->sharedPtr(), Punctual::afterUpdate(),
                                         [this, result, owner] (Valuable::Attribute * attr) {
        if (attr) {
          attr->addListener(owner, [this, result] {
            onChange.raise(result);
          });
          onChange.raise(result);
        }
      });

      node->onAttributeRemoved.addListener(owner->sharedPtr(), [this, result] (auto) {
        onChange.raise(result);
      });

      for (auto & p: node->attributes()) {
        p.second->addListener(owner, [this, result] {
          onChange.raise(result);
        });
      }

      m_found = true;
      onChange.raise(result);
    }

  private:
    /// These are used as a way to remove event listeners. If the search path
    /// is "foo/bar", then m_listenerOwners[0] monitors the root when "foo"
    /// is added, [1] monitors "foo" when "bar" is added, and [2] monitors
    /// "bar" for any attributes that are added.
    std::vector<std::unique_ptr<Node>> m_listenerOwners;
    bool m_found = false;
  };
}
