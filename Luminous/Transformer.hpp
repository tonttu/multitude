/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#ifndef LUMINOUS_TRANSFORMER_HPP
#define LUMINOUS_TRANSFORMER_HPP

#include <Luminous/Export.hpp>

#include <Nimble/Matrix4.hpp>
#include <Patterns/NotCopyable.hpp>

#include <stack>
#include <vector>

namespace Luminous
{
  /// Convert 2D homogeneous transformation matrix to 3D
  inline Nimble::Matrix4f mat4(const Nimble::Matrix3f & m)
  {
    return Nimble::Matrix4f(m[0][0], m[0][1], 0, m[0][2],
                            m[1][0], m[1][1], 0, m[1][2],
                                  0,       0, 1,       0,
                            m[2][0], m[2][1], 0, m[2][2]);
  }

  /// Convert 3D homogeneous transformation matrix to 2D by dropping Z coordinates
  inline Nimble::Matrix3f mat3(const Nimble::Matrix4f & m)
  {
    return Nimble::Matrix3f(m[0][0], m[0][1], m[0][3],
                            m[1][0], m[1][1], m[1][3],
                            m[3][0], m[3][1], m[3][3]);
  }

  /** Geometrical 3D transformation stack. This class encapsulates 3D
      transformation stack. The transformations are stored as 4x4
      matrices. */
  class LUMINOUS_API Transformer
  {
  public:
    /// Creates an empty transformation stack
    Transformer();
    /// Deletes the transformation stack
    virtual ~Transformer();

    /// Get the top matrix of the stack
    const Nimble::Matrix4 & transform() const { return m_stack.top(); }
    /// Get the top matrix of the stack as 3x3-matrix
    Nimble::Matrix3 transform3() const;

    /// Apply the current transformation matrix on a 2D vector.
    Nimble::Vector2 project(const Nimble::Vector2&) const;
    /// Apply inverse of the current transformation matrix on a 2D vector.
    Nimble::Vector2 unproject(const Nimble::Vector2&) const;

    /// Pops the top matrix from the stack
    void popTransform() { beforeTransformChange(); m_stack.pop(); }

    /// Multiply the top matrix from the left with the given matrix and push the
    /// result into the stack
    void pushTransformLeftMul(const Nimble::Matrix4 & m);
    /// @deprecated Transformation stack uses 4x4-matrices
    void pushTransformLeftMul(const Nimble::Matrix3 & m);
    /// Multiply the top matrix from the right with the given matrix and push
    /// the result into the stack
    void pushTransformRightMul(const Nimble::Matrix4 & m);
    /// @deprecated Transformation stack uses 4x4-matrices
    void pushTransformRightMul(const Nimble::Matrix3 & m);
    /// Push a new matrix to the stack, just copying the current top
    void pushTransform();
    /// Push the given matrix to the stack
    /// pushTransform(m) has the same effect as:
    /// @code
    ///  renderContext.pushTransform();
    ///  renderContext.setTransform(m);
    /// @endcode
    /// @param m matrix to push
    void pushTransform(const Nimble::Matrix4 & m);
    /// @deprecated Transformation stack uses 4x4-matrices
    void pushTransform(const Nimble::Matrix3 & m);
    /// Replaces the top matrix with the given matrix
    /// @param m matrix to set
    void setTransform(const Nimble::Matrix4 & m);

    /// Multiply the top matrix from the left with the given matrix
    /// The end result is equivalent to:
    ///  @code renderContext.setTransform(m * renderContext.transform()); @endcode
    /// @param m matrix to multiply with
    void leftMul(const Nimble::Matrix4 & m);

    /// Multiply the top matrix from the right with the given matrix
    /// The end result is equivalent to:
    /// @code renderContext.setTransform(renderContext.transform() * m); @endcode
    /// @param m matrix to multiply with
    void rightMul(const Nimble::Matrix4 & m);

    /// Clears the stack so it only contains an identity matrix
    void resetTransform();

    /// Get the size of the transform stack
    /// @return size of the stack
    size_t stackSize() const { return m_stack.size(); }

  protected:

    /// This function gets called just before the transformation matrix is
    /// changed.
    virtual void beforeTransformChange();

    /// The transformation stack
    std::stack<Nimble::Matrix4, std::vector<Nimble::Matrix4>> m_stack;
  };
}

#endif

