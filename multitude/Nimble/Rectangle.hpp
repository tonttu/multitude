#ifndef NIMBLE_RECTANGLE_HPP
#define NIMBLE_RECTANGLE_HPP

#include <Nimble/Export.hpp>
#include <Nimble/Vector2.hpp>
#include <Nimble/Matrix3.hpp>

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
      Nimble::Vector2f center() const;

      /// Return the size of the rectangle
      Nimble::Vector2 size() const;

    private:
      Nimble::Vector2f m_origin;
      Nimble::Vector2f m_axis0;
      Nimble::Vector2f m_axis1;
      float m_extent0;
      float m_extent1;
  };

}

#endif
