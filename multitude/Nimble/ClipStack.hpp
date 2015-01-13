/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_CLIPSTACK_HPP
#define NIMBLE_CLIPSTACK_HPP

#include "Export.hpp"
#include "Rectangle.hpp"

namespace Nimble
{
  /// This class provides an implementation of a clipping stack. The stack is
  /// defined by a list of rectangles. Each rectangle defines the visible area
  /// on that stack level. The ClipStack is used, for example, by the rendering
  /// algorithm to determine which widgets are visible and need to be drawn.
  class NIMBLE_API ClipStack
  {
  public:
    /// Construct a new ClipStack
    ClipStack();
    /// Construct a copy of a ClipStack
    /// @param other ClipStack to copy
    ClipStack(const ClipStack & other);
    /// Copy a ClipStack
    /// @param other ClipStack to copy
    /// @return reference to the ClipStack
    ClipStack & operator=(const ClipStack & other);
    /// Destroy a ClipStack
    ~ClipStack();

    /// Push a rectangle to the top of the ClipStack
    /// @param r rectangle to push
    /// @return reference to the ClipStack
    ClipStack & push(const Rectangle & r);
    /// Pop a rectangle from the top of the ClipStack
    /// @return reference to the ClipStack
    ClipStack & pop();

    /// Check if the given rectangle is visible. A rectangle is visible, if the
    /// intersection of the given rectangle and every rectangle in stack is
    /// non-empty.
    /// @param r rectangle to check
    /// @return true if the rectangle is visible; otherwise false
    bool isVisible(const Nimble::Rectangle & r) const;
    /// Check if the given point is visible
    /// @param p point to check
    /// @return true if the point is visible: otherwise false
    bool isVisible(const Nimble::Vector2 & p) const;

    /// Get the bounding box encompassing all the rectangles in the clipstack.
    /// @return bounding box of all rectangles
    Nimble::Rect boundingBox() const;

    /// Get the depth of the clip stack. Returns the number of rectangles in the stack.
    /// @return depth of the stack
    size_t stackDepth() const;

    /// Get a rectangle from the stack. The index ranges from zero (bottom of
    /// the stack) to stackDepth() - 1 (top of the stack).
    /// @param index index of the rectangle in the stack
    /// @return stack requested rectangle
    Nimble::Rectangle stackRectangle(size_t index) const;

  private:
    class D;
    D * m_d;
  };

}

#endif // NIMBLE_CLIPSTACK_HPP
