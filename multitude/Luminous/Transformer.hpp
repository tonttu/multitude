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
#ifndef LUMINOUS_TRANSFORMER_HPP
#define LUMINOUS_TRANSFORMER_HPP

#include <Luminous/Export.hpp>

#include <Nimble/Matrix3.hpp>

#include <stack>
#include <vector>

namespace Luminous
{
  /** Geometrical 2D transformation stack. This class encapsulates 2D
      transformation stack. The transformations are stored as 3x3
      matrices. */
  class LUMINOUS_API Transformer
  {
  public:
    /// Creates an empty transformation stack
    Transformer();
    /// Deletes the transformation stack
    virtual ~Transformer();

    /// Get the top matrix of the stack
    const Nimble::Matrix3 & transform() const { return m_stack.top(); }

    /// Apply the current transformation matrix on a 2D vector.
    Nimble::Vector2 project(Nimble::Vector2) const;
    /// Apply inverse of the current transformation matrix on a 2D vector.
    Nimble::Vector2 unproject(Nimble::Vector2) const;

    /// Extracts the scaling from the transform. Only valid for uniform scaling.
    float scale() const;

    /// Pops the top matrix from the stack
    void popTransform() { beforeTransformChange(); m_stack.pop(); }

    /// Multiply the top matrix from the left with the given matrix and push the
    /// result into the stack
    void pushTransformLeftMul(const Nimble::Matrix3 & m);
    /// Multiply the top matrix from the right with the given matrix and push
    /// the result into the stack
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
    void pushTransform(const Nimble::Matrix3 & m);
    /// Replaces the top matrix with the given matrix
    /// @param m matrix to set
    void setTransform(const Nimble::Matrix3 & m);

    /// Multiply the top matrix from the left with the given matrix
    /// The end result is equivalent to:
    ///  @code renderContext.setTransform(m * renderContext.transform()); @endcode
    /// @param m matrix to multiply with
    void leftMul(const Nimble::Matrix3 & m);

    /// Multiply the top matrix from the right with the given matrix
    /// The end result is equivalent to:
    /// @code renderContext.setTransform(renderContext.transform() * m); @endcode
    /// @param m matrix to multiply with
    void rightMul(const Nimble::Matrix3 & m);

    /// Clears the stack so it only contains an identity matrix
    void resetTransform();

  protected:
    virtual void beforeTransformChange();

    /// The transformation stack
    std::stack<Nimble::Matrix3, std::vector<Nimble::Matrix3> > m_stack;
  };
}

#endif

