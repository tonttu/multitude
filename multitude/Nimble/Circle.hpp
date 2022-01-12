#ifndef NIMBLE_CIRCLE_HPP
#define NIMBLE_CIRCLE_HPP

#include "Export.hpp"
#include "Rect.hpp"

namespace Nimble {
  /** A Circle class defines a circular region with a center/radius and provides
  methods for testing intersections with a given rectangle. **/
  class NIMBLE_API Circle
  {
    public:
      /// Construct a new circle with no parameters
      Circle();

      /// Construct a new circle
      /// @param centre centre of the circle
      /// @param radius radius of the circle
      Circle(const Nimble::Vector2f & center, float radius);

      /// Construct a copy of the given circle
      /// @param circle circle to copy
      Circle(const Nimble::Circle & circle);

      /// Obtain bounding box for circle
      /// @return Rect bounds
      Rectf boundingBox() const;

      /// Set the center of the circle
      /// @param center new center
      void setCenter(const Nimble::Vector2f & center) {m_center=center;}

      /// Obtain centre of circle
      /// @return centre point.
      const Nimble::Vector2f & center() const {return m_center;}

      /// Set the radius of the circle
      /// @param radius new radius
      void setRadius(float radius) {m_radius=radius; m_radiusSquared=radius*radius;}

      /// Obtain radius of circle
      /// @return radius
      float radius() const {return m_radius;}

      /// Radius ^ 2
      float radiusSqr() const { return m_radiusSquared; }

      /// Check if the circle contains the specified rectangle
      /// @param rect bounding box definition
      /// @return true if rect is fully contained within circle, including the case where
      /// one or more points is exactly on the edge, false otherwise
      bool contains(const Nimble::Rectf & rect) const;

      /// Check if the circle contains the specified point
      /// @param point definition
      /// @return true if point is in circle or on the edge, false otherwise
      bool contains(const Nimble::Vector2f & point) const;

      /// Check if the circle intersects with the specified rectangle
      /// @param rect bounding box definition
      /// @return true if circle touches box, false otherwise
      bool intersects(const Nimble::Rectf & rect) const;

    private:
      Nimble::Vector2f m_center;
      float m_radius;
      float m_radiusSquared;
  };
}

#endif
