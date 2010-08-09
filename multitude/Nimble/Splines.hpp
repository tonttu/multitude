#ifndef NIMBLE_SPLINES_HPP
#define NIMBLE_SPLINES_HPP

#include <Nimble/Export.hpp>

#include <vector>

#include <Nimble/Vector2.hpp>
namespace Luminous {
  class RenderContext;
}

namespace Nimble
{

    /// Utility functions for evaluating splines
    namespace Splines
    {

        /// Evaluate a cubic Catmull-Rom spline on an interval.
        /// @param t interpolation parameter [0, 1]
        /// @param cp vector of control points (needs at least four)
        /// @param index to the first control point used in interpolation. Used
        ///        control points will be from [index, index + 3]
        /// @return value of the spline at given t
        template<class T>
        T evalCatmullRom(float t, const std::vector<T> & cp, size_t index = 0);

        /// Catmull-Rom
        /// @todo doc
        class NIMBLE_API Interpolating {
          typedef std::vector<Nimble::Vector2> PointList;
          typedef PointList::size_type Index;

          PointList m_points;
          PointList m_tangents;
          Nimble::Vector2 get(Index ii, float h1, float h2, float h3, float h4) const;
          Nimble::Vector2 getPoint(Index ii, float t) const;
        public:
          Nimble::Vector2 getDerivative(Index ii, float t) const;
          /// Evaluates the spline at given t
          /// @param t position where to evaluate the spline. 0 <= t <= size() - 1
          Nimble::Vector2 get(float t) const;
          /// Adds a control point
          void add(Nimble::Vector2 point);
          /// Removes the control point at the given index
          void remove(Index ii);
          /// Returns the number of control points
          Index size() const { return m_points.size(); }

          void clear();

          friend class Luminous::RenderContext;
        };

    }

}

#endif
