/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#include "Transformer.hpp"
#include "RenderContext.hpp"

namespace Luminous
{
  Transformer::Transformer()
  {
    resetTransform();
  }

  Transformer::~Transformer()
  {}

  Nimble::Matrix3 Transformer::transform3() const
  {
    return mat3(transform());
  }

  Nimble::Vector2 Transformer::project(const Nimble::Vector2 & v) const
  {
    return transform().project(v);
  }

  /// @todo what? rename
  Nimble::Vector2 Transformer::unproject(const Nimble::Vector2 & v) const
  {
    Nimble::Matrix3 m = transform3().inverse();
    Nimble::Vector3 p = m * Nimble::Vector3(v, 1);

    return Nimble::Vector2(p.x / p.z, p.y / p.z);
  }

  void Transformer::pushTransformLeftMul(const Nimble::Matrix4 & m)
  {
    beforeTransformChange();
    m_stack.push(m * m_stack.top());
#if defined(RADIANT_DEBUG)
    if(m_stack.size() > 200)
      Radiant::warning("Transformer::pushTransformLeftMul # stack is very deep (%d)",
                       (int) m_stack.size());
#endif
  }
  void Transformer::pushTransformLeftMul(const Nimble::Matrix3 & m)
  {
    pushTransformLeftMul(mat4(m));
  }

  void Transformer::pushTransformRightMul(const Nimble::Matrix4 & m)
  {
    beforeTransformChange();
    m_stack.push(m_stack.top() * m);
#if defined(RADIANT_DEBUG)
    if(m_stack.size() > 200)
      Radiant::warning("Transformer::pushTransformLeftMul # stack is very deep (%d)",
                       (int) m_stack.size());
#endif
  }

  void Transformer::pushTransformRightMul(const Nimble::Matrix3 & m)
  {
    pushTransformRightMul(mat4(m));
  }

  void Transformer::pushTransform()
  {
    m_stack.push(m_stack.top());
#if defined(RADIANT_DEBUG)
    if(m_stack.size() > 200)
      Radiant::warning("Transformer::pushTransformLeftMul # stack is very deep (%d)", (int) m_stack.size());
#endif
  }

  void Transformer::pushTransform(const Nimble::Matrix4 & m)
  {
    beforeTransformChange();
    m_stack.push(m);
  }

  void Transformer::pushTransform(const Nimble::Matrix3 & m)
  {
    pushTransform(mat4(m));
  }

  void Transformer::setTransform(const Nimble::Matrix4 & m)
  {
    beforeTransformChange();
    m_stack.top() = m;
  }

  void Transformer::leftMul(const Nimble::Matrix4 &m)
  {
    beforeTransformChange();
    Nimble::Matrix4 & top = m_stack.top();
    top = top * m;
  }

  void Transformer::rightMul(const Nimble::Matrix4 &m)
  {
    beforeTransformChange();
    m_stack.top() *= m;
  }

  void Transformer::resetTransform()
  {
    beforeTransformChange();
    while(!m_stack.empty())
      m_stack.pop();

    m_stack.push(Nimble::Matrix4::IDENTITY);
  }

  void Transformer::beforeTransformChange()
  {}
}
