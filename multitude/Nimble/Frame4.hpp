/* COPYRIGHT
 *
 * This file is part of Nimble.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Nimble.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */
#ifndef NIMBLE_FRAME4_HPP
#define NIMBLE_FRAME4_HPP

#include "Vector4.hpp"

namespace Nimble
{

  ///             x
  ///         w       y
  ///             z
  class Frame4f : public Vector4f
  {
  public:
    Frame4f(const Nimble::Vector4f & v = Nimble::Vector4f(0, 0, 0, 0)) : Vector4f(v) {}

    float top() const { return x; }
    float right() const { return y; }
    float bottom() const { return z; }
    float left() const { return w; }

    float width() const { return left() + right(); }
    float height() const { return top() + bottom(); }

    Vector2 leftTop() const { return Vector2(left(), top()); }
    Vector2 rightBottom() const { return Vector2(right(), bottom()); }
    Vector2 size() const { return Vector2(width(), height()); }
  };

}

#endif // NIMBLE_FRAME4_HPP
