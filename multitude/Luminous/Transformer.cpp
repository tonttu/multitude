/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */
#include "Transformer.hpp"

namespace Luminous
{

  Transformer::Transformer()
  {
    resetTransform();
  }

  Transformer::~Transformer()
  {}

  Nimble::Vector2 Transformer::project(Nimble::Vector2 v) const
  {
    return transform().project(v);
  }

  /// @todo what? rename
  Nimble::Vector2 Transformer::unproject(Nimble::Vector2 v) const
  {
    Nimble::Matrix3 m = transform().inverse();
    Nimble::Vector3 p = m * v;

    return Nimble::Vector2(p.x / p.z, p.y / p.z);
  }

  float Transformer::scale() const
  {
    return transform().extractScale();
  }

  void Transformer::pushTransformLeftMul(const Nimble::Matrix3 & m)
  {
    m_stack.push(m * m_stack.top());
  }

  void Transformer::pushTransformRightMul(const Nimble::Matrix3 & m)
  {
    m_stack.push(m_stack.top() * m);
  }

  void Transformer::pushTransform()
  {
    m_stack.push(m_stack.top());
  }

  void Transformer::pushTransform(const Nimble::Matrix3 & m)
  {
    m_stack.push(m);
  }

  void Transformer::setTransform(const Nimble::Matrix3 & m)
  {
    m_stack.top() = m;
  }

  void Transformer::leftMul(const Nimble::Matrix3 &m)
  {
    Nimble::Matrix3 & top = m_stack.top();
    top = top * m;
  }

  void Transformer::rightMul(const Nimble::Matrix3 &m)
  {
    m_stack.top() *= m;
  }

  void Transformer::resetTransform()
  {
    while(!m_stack.empty())
      m_stack.pop();

    Nimble::Matrix3 m;
    m.identity();

    m_stack.push(m);
  }

}
