#ifndef NIMBLE_CIRCLE_H
#define NIMBLE_CIRCLE_H

#include "Export.hpp"
#include "Rect.hpp"

namespace Nimble {
  /** A Circle class defines a circle with a center/radius and provides
  methods for testing intersections with a given rectangle. **/
  class NIMBLE_API Circle
  {
    public:
      /// Construct a new circle
      /// @param centre centre of the circle
      /// @param radius radius of the circle
      Circle(const Nimble::Vector2f & center, float radius);

      /// Obtain bounding box for circle
      /// @return Rect bounds
      Nimble::Rect boundingBox() const;

      /// Obtain centre of circle
      /// @return centre point.
      const Nimble::Vector2f & center() const {return m_center;}

      /// Obtain radius of circle
      /// @return radius
      float radius() const {return m_radius;}

      /// Check if the circle contains the specified rectangle
      /// @param rect bounding box definition
      /// @return true if rect is fully contained within circle, false otherwise
      bool contains(const Nimble::Rectf & rect) const;

      /// Check if the circle contains the specified point
      /// @param point definition
      /// @return true if point is in cirlce, false otherwise
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

#endif // CANVUS_CIRCLE_H
