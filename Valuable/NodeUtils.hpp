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

#include <vector>

namespace Valuable
{
  namespace NodeUtils
  {
    /// Find all descendant nodes recursively with the given type
    template <typename T>
    inline std::vector<T*> findDescendantNodes(const Node & node);

    /// Find all child nodes non-recursively with the given type
    template <typename T>
    inline std::vector<T*> findChildNodes(const Node & node);

    /// Returns the node hierarchy root node, or nullptr if the given node
    /// doesn't have a parent
    inline Node * root(Node & node);
    inline const Node * root(const Node & node);

    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <typename T>
    inline void findDescendantNodesImpl(std::vector<T*> & out, const Node & node)
    {
      for (auto & p: node.attributes()) {
        if (auto t = dynamic_cast<T*>(p.second))
          out.push_back(t);
        if (auto child = dynamic_cast<const Node*>(p.second))
          findDescendantNodesImpl(out, *child);
      }
    }

    template <typename T>
    inline std::vector<T*> findDescendantNodes(const Node & node)
    {
      std::vector<T*> out;
      findDescendantNodesImpl(out, node);
      return out;
    }

    template <typename T>
    inline std::vector<T*> findChildNodes(const Node & node)
    {
      std::vector<T*> out;
      for (auto & p: node.attributes())
        if (auto t = dynamic_cast<T*>(p.second))
          out.push_back(t);
      return out;
    }

    inline Node * root(Node & node)
    {
      Node * test = &node;
      Node * parent;
      while ((parent = test->host()))
        test = parent;
      return test;
    }

    inline const Node * root(const Node & node)
    {
      const Node * test = &node;
      const Node * parent;
      while ((parent = test->host()))
        test = parent;
      return test;
    }
  }
}
