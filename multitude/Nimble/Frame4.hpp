/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#ifndef NIMBLE_FRAME4_HPP
#define NIMBLE_FRAME4_HPP

#include "Vector2.hpp"
#include "Vector4.hpp"
#include "Size.hpp"

namespace Nimble
{

  /// Frame of four floats. This class provides variable thickness frame
  /// information for a rectangle. Each side can have a frame of different
  /// thickness. The frame thickness is encoded with x being the top, y right,
  /// z bottom, and w the left.
  class Frame4f : public Vector4f
  {
  public:
    /// Construct a new frame
    /// @param v initial values
    Frame4f(const Nimble::Vector4f & v = Nimble::Vector4f(0, 0, 0, 0)) : Vector4f(v) {}
    /// Construct a new frame
    /// @param t top frame
    /// @param r right frame
    /// @param b bottom frame
    /// @param l left frame
    Frame4f(float t, float r, float b, float l) : Vector4f(t, r, b, l) {}

    /// Get the top frame
    /// @return thickness of the top frame
    float top() const { return x; }
    /// Get the right frame
    /// @return thickness of the right frame
    float right() const { return y; }
    /// Get the bottom frame
    /// @return thickness of the bottom frame
    float bottom() const { return z; }
    /// Get the left frame
    /// @return thickness of the left frame
    float left() const { return w; }

    /// Get the width of the frame (left() + right())
    /// @return width of the frame
    float width() const { return left() + right(); }
    /// Get the height of the frame (top() + bottom())
    /// @return height of the frame
    float height() const { return top() + bottom(); }

    /// Get the top-left corner of the frame
    /// @return top-left corner
    Nimble::Vector2f leftTop() const { return Vector2f(left(), top()); }
    /// Get the bottom-right corner of the frame
    /// @return bottom-right corner
    Nimble::Vector2f rightBottom() const { return Vector2f(right(), bottom()); }
    /// Get the size of the frame
    /// @return the size of the frame
    Nimble::SizeF size() const { return SizeF(width(), height()); }
  };

}

#endif // NIMBLE_FRAME4_HPP
