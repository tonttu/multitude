/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ClipStack.hpp"

#include <Radiant/Trace.hpp>

#include <vector>

namespace Nimble
{

class ClipStack::D
{
public:
  struct StackItem
  {
    Nimble::Rectangle m_rectangle;
    Nimble::Rect m_compoundedBoundingBox;
  };

  typedef std::vector<StackItem> Stack;
  Stack m_stack;

  void push(const Rectangle & r)
  {
    StackItem si;

    si.m_rectangle = r;

    if(m_stack.empty())
      si.m_compoundedBoundingBox = r.boundingBox();
    else
      si.m_compoundedBoundingBox = m_stack.back().m_compoundedBoundingBox.intersection(r.boundingBox());

    m_stack.push_back(si);
  }

  void pop()
  {
    assert(m_stack.empty() == false);
    m_stack.pop_back();
  }
};


ClipStack::ClipStack()
  : m_d(new D())
{
}

ClipStack::ClipStack(const ClipStack & other)
  : m_d(new D())
{
  *m_d = *other.m_d;
}

ClipStack & ClipStack::operator=(const ClipStack & other)
{
  *m_d = *other.m_d;
  return *this;
}

ClipStack::~ClipStack()
{
  delete m_d;
}

ClipStack & ClipStack::push(const Rectangle &r)
{
  m_d->push(r);
  return *this;
}

ClipStack & ClipStack::pop()
{
  m_d->pop();
  return *this;
}

bool ClipStack::isVisible(const Rectangle &r) const
{
  if(m_d->m_stack.empty())
    return true;

  if (r.extent0() <= 0.f || r.extent1() <= 0.f)
    return false;

  auto bb = r.boundingBox();
  for(D::Stack::reverse_iterator it = m_d->m_stack.rbegin(); it != m_d->m_stack.rend(); ++it) {
    if(!it->m_compoundedBoundingBox.intersects(bb))
      return false;

    const D::StackItem & si = *it;

    if(!si.m_rectangle.intersects(r))
      return false;
  }

  return true;
}

bool ClipStack::isVisible(const Nimble::Vector2 & p) const
{
  if(m_d->m_stack.empty())
    return true;

  for(D::Stack::reverse_iterator it = m_d->m_stack.rbegin(); it != m_d->m_stack.rend(); ++it) {
    if(!it->m_compoundedBoundingBox.contains(p))
      return false;

    const D::StackItem & si = *it;

    if(!si.m_rectangle.contains(p))
      return false;
  }

  return true;
}

Rect ClipStack::boundingBox() const
{
  if (m_d->m_stack.empty())
    return Rect();

  return m_d->m_stack.back().m_compoundedBoundingBox;
}

size_t ClipStack::stackDepth() const
{
  return m_d->m_stack.size();
}

Rectangle ClipStack::stackRectangle(size_t index) const
{
  assert(index < m_d->m_stack.size());
  return m_d->m_stack.at(index).m_rectangle;
}

}
