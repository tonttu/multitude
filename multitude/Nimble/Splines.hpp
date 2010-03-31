#ifndef NIMBLE_SPLINES_HPP
#define NIMBLE_SPLINES_HPP

#include <vector>

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

    }

}

#endif
