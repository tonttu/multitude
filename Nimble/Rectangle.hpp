/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_RECTANGLE_HPP
#define NIMBLE_RECTANGLE_HPP

#include "Export.hpp"
#include "Vector2.hpp"
#include "Matrix3.hpp"
#include "Rect.hpp"

#include <array>

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
      /// @param m the resulting rectangle is formed by transforming the axis-aligned rectangle from -size/2 to size/2 with this matrix
      Rectangle(Nimble::SizeF size, const Nimble::Matrix3 & m);

      /// Construct a copy of the given rectangle
      /// @param rect rectangle to copy
      Rectangle(const Nimble::Rectf & rect);

      /// Test if a point is inside the rectangle
      /// @param p point to test
      /// @return true if p is inside or on the rectangle
      bool contains(Nimble::Vector2f p) const;

      /// Test if an another rectangle is fully inside this rectangle
      /// @param r rectangle to test
      /// @return true if r is fully inside or on the rectangle
      bool contains(const Nimble::Rectangle & r) const;

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
      Nimble::SizeF size() const;

      /// Computes the corner vertices of the rectangle
      std::array<Nimble::Vector2f, 4> computeCorners() const;

      /// Computes the corner vertices of the rectangle and writes them to the given array
      /// @param corners vector of points where the four corners are appended
      MULTI_ATTR_DEPRECATED("computerCorners(corners) is deprecated. Use computeCorners() instead.",
      void computeCorners(std::array<Nimble::Vector2f, 4> & corners) const);

      /// Returns a rectangle that contains the two given rectangles. The
      /// result is not guaranteed to be the smallest rectangle containing the
      /// input rectangles.
      /// @param a rectangle to merge
      /// @param b rectangle to merge
      /// @return Bouding rectangle of the two input rectangles
      static Nimble::Rectangle merge(const Nimble::Rectangle & a, const Nimble::Rectangle & b);

      /// Transforms the rectangle with the given matrix. This function
      /// transforms all four corner points separately and then forms a bounding
      /// box around then.
      /// @param m transformation matrix
      void transform(const Nimble::Matrix3 & m);

      /// Get the axis-aligned bounding box of this rectangle.
      /// @return axis-aligned bounding box
      Nimble::Rect boundingBox() const;

    private:
      Nimble::Vector2f m_origin;
      Nimble::Vector2f m_axis0;
      Nimble::Vector2f m_axis1;
      float m_extent0;
      float m_extent1;
  };

}

#endif
