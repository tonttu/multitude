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

#ifndef NIMBLE_RECTANGLE_HPP
#define NIMBLE_RECTANGLE_HPP

#include <Nimble/Export.hpp>
#include <Nimble/Vector2.hpp>
#include <Nimble/Matrix3.hpp>

#include <vector>

namespace Nimble {

  /** A rectangle is defined by origin (O), two unit-length axis vectors
  (U, V), and two non-negative extents (e1, e2). Point P=O+x*U+y*V
  is inside or on the rectangle whenever |x|<=e1 and |y|<=e2.*/
  class NIMBLE_API Rectangle
  {
    public:
    /// Constructs a new rectangle. Does not initialize values.
    Rectangle();

      /// Constructs a new rectangle.
      /// @param origin center of the box
      /// @param a0 unit-length axis vector
      /// @param e0 non-negative extent along a0
      /// @param a1 unit-length axis vector
      /// @param e1 non-negative extent along a1
      Rectangle(Nimble::Vector2f origin, Nimble::Vector2f a0, float e0, Nimble::Vector2f a1, float e1);

      /// Constructs a new rectangle
      /// @param size size (width & height) of the rectangle
      /// @param m transformation matrix defining the center of the rectangle
      Rectangle(Nimble::Vector2f size, const Nimble::Matrix3 & m);

      /// Test if a point is inside the rectangle
      /// @param p point to test
      /// @return true if p is inside or on the rectangle
      bool inside(Nimble::Vector2f p) const;

      /// Test if two rectangles intersect
      /// @param r rectangle to test
      /// @return true if the rectangles intersect
      bool intersects(const Rectangle & r) const;
      
      /// Return the center point of the rectangle
      Nimble::Vector2f center() const { return m_origin; }
      /// Returns the first axis of the rectangle
      Nimble::Vector2f axis0() const { return m_axis0; }
      /// Returns the second axis of the rectangle
      Nimble::Vector2f axis1() const { return m_axis1; }
      /// Returns the extent along axis0
      float extent0() const { return m_extent0; }
      /// Returns the extent along axis1
      float extent1() const { return m_extent1; }

      /// Return the size of the rectangle
      Nimble::Vector2 size() const;

      /// Computes the corner vertices of the rectangle and appends the to the given vector
      /// @param corners vector of points where the four corners are appended
      void computeCorners(std::vector<Nimble::Vector2f> & corners) const;

      /// Returns a rectangle that contains the two given rectangles. The
      /// result is not guaranteed to be the smallest rectangle containing the
      /// input rectangles.
      /// @param a rectangle to merge
      /// @param b rectangle to merge
      static Nimble::Rectangle merge(const Nimble::Rectangle & a, const Nimble::Rectangle & b);

      /// Transforms the rectangle with the given matrix. If the matrix is not
      /// orthogonal, the results are undefined.
      /// @param m transformation matrix
      void transform(const Nimble::Matrix3 & m);

    private:
      Nimble::Vector2f m_origin;
      Nimble::Vector2f m_axis0;
      Nimble::Vector2f m_axis1;
      float m_extent0;
      float m_extent1;
  };

}

#endif
