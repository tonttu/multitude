/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
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
  inline Nimble::Matrix4f mat4(const Nimble::Matrix3f & m)
  {
    return Nimble::Matrix4f(m[0][0], m[0][1], 0, m[0][2],
                            m[1][0], m[1][1], 0, m[1][2],
                                  0,       0, 1,       0,
                            m[2][0], m[2][1], 0, m[2][2]);
  }

  inline Nimble::Matrix3f mat3(const Nimble::Matrix4f & m)
  {
    return Nimble::Matrix3f(m[0][0], m[0][1], m[0][3],
                            m[1][0], m[1][1], m[1][3],
                            m[3][0], m[3][1], m[3][3]);
  }

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
