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

  Nimble::Matrix3 Transformer::transform() const
  {
    return mat3(transform4());
  }

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

  void Transformer::pushTransformLeftMul(const Nimble::Matrix4 & m)
  {
    beforeTransformChange();
    m_stack.push(m * m_stack.top());
  }
  void Transformer::pushTransformLeftMul(const Nimble::Matrix3 & m)
  {
    pushTransformLeftMul(mat4(m));
  }

  void Transformer::pushTransformRightMul(const Nimble::Matrix4 & m)
  {
    beforeTransformChange();
    m_stack.push(m_stack.top() * m);
  }

  void Transformer::pushTransformRightMul(const Nimble::Matrix3 & m)
  {
    pushTransformRightMul(mat4(m));
  }

  void Transformer::pushTransform()
  {
    m_stack.push(m_stack.top());
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
