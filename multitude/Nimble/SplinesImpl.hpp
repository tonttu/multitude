#include "Splines.hpp"

#include <strings.h>

namespace Nimble
{
    namespace Splines
    {

        template<class T>
        T evalCatmullRom(float t, const std::vector<T> & cp, size_t index)
        {
            // We need at least four control points to interpolate
          if(cp.size() < 4) {
            T zero;
            bzero(&zero, sizeof(zero));
            return zero;
          }

            float b1 = 0.5 * (-    t * t * t + 2 * t * t - t);
            float b2 = 0.5 * ( 3 * t * t * t - 5 * t * t + 2);
            float b3 = 0.5 * (-3 * t * t * t + 4 * t * t + t);
            float b4 = 0.5 * (     t * t * t -     t * t);

            size_t i1 = index + 0;
            size_t i2 = index + 1;
            size_t i3 = index + 2;
            size_t i4 = index + 3;

            return b1 * cp[i1] + b2 * cp[i2] + b3 * cp[i3] + b4 * cp[i4];
        }

    }
}
