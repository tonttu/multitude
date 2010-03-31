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

#ifndef NIMBLE_RANDOM_HPP
#define NIMBLE_RANDOM_HPP

#include <Nimble/Vector2.hpp>

#include <stdint.h>

namespace Nimble {

  /// Random number generator with uniform distribution
  /** This class generates random numbers with uniform
      distribution.

      It uses a fast and cheap modulo-based random number
      generation. The side effext of this is that the lower bits of
      the random number sequence are not so very random. If you need
      small integers with random values you will need to use the
      #rand24 function, which uses only the 24 higher bits that are
      fairly random.

      The random number sequence is identical on all platforms, given
      the same seed value.
  */
  class NIMBLE_API RandomUniform
  {
  public:
    RandomUniform(uint32_t val = 0) : m_val(val) {}
    ~RandomUniform() {}

    /// Random numbers between 0 and 1
    inline float rand01()
    {
      uint32_t tmp = m_val * m_randMul + 1;
      m_val = tmp;
      return (float) tmp * (1.0f / (float) ((uint32_t) 0xffffffff));
    }

    /// Random numbers between 0 and x
    inline float rand0X(float x)
    {
      uint32_t tmp = m_val * m_randMul + 1;
      m_val = tmp;
      return (float) tmp * (x / (float) ((uint32_t) 0xffffffff));
    }

    /// Random numbers between 0 and x
    inline double rand0X(double x)
    {
      uint32_t tmp = m_val * m_randMul + 1;
      m_val = tmp;
      return (double) tmp * (x / (double) ((uint32_t) 0xffffffff));
    }

    /// Random numbers between 0 and x-1
    inline uint32_t rand0X(uint32_t x)
    {
      uint32_t tmp = m_val * m_randMul + 1;
      m_val = tmp;
      return tmp % x;
    }

    /// Random numbers between -1 and 1
    inline float rand11()
    {
      uint32_t tmp = m_val * m_randMul + 1;
      m_val = tmp;
      return (float) tmp * (2.0f / (float) ((uint32_t) 0xffffffff)) - 1.0f;
    }

    /// Random numbers between -x and x
    inline float randXX(float x)
    {
      uint32_t tmp = m_val * m_randMul + 1;
      m_val = tmp;
      return (float) tmp * (2.0f * x / (float) ((uint32_t) 0xffffffff)) - x;
    }

    /// Random numbers between min and max
    inline float randMinMax(float min, float max)
    {
      return rand0X(max - min) + min;
    }

    /** A random number in range 0-2^32. The lower bits of the random
        number are not totally random. */
    inline uint32_t rand()
    {
      m_val = m_val * m_randMul + 1;
      return m_val;
    }

    /** A random number in range 0-2^24. All bits of the value should
        be fairly random. */
    inline uint32_t rand24()
    {
      m_val = m_val * m_randMul + 1;
      return m_val >> 8;
    }

    /** A random number in range 0-2^32. All bits of the value should
        be fairly random. */
    inline uint32_t rand32()
    {
      /* Generate two random numbers are take the 16 higher bits from
         both. */
      uint32_t v1 = m_val * m_randMul + 1;
      uint32_t v2 = v1 * m_randMul + 1;
      m_val = v2;
      return (v1 & 0xFFFF0000) | (v2 >> 16);
    }

    /** Get random numbers between 0 and range-1. */
    inline uint32_t randN(uint32_t range)
    {
      m_val = m_val * m_randMul + 1;
      return (m_val >> 8) % range;
    }

    /// Random 2d unit vector
    /** This function is deprecated, as it is a duplicate for #randVecOnCircle(). */
    /// @todo remove
    inline Nimble::Vector2f randVec2()
    {
      return randVecOnCircle();
    }

    /// Random 2d vector on a unit circle
    inline Nimble::Vector2f randVecOnCircle()
    {
      float a = rand0X(Math::TWO_PI);
      return Nimble::Vector2f(cosf(a), sinf(a));
    }

    static RandomUniform & instance() { return m_instance; }

    /// @todo add static members inside Nimble::Math ?

  private:
    uint32_t m_val;
    static const uint32_t m_randMul = 134695621;
    static RandomUniform  m_instance;
  };

  /// RandomGaussian generates pseudo-random numbers from a normal (gaussian) distribution.
  class NIMBLE_API RandomGaussian
  {
    public:
      /// Construct a generator with given parameters for the distribution of
      /// the random numbers.
      /// @param mean the mean of the normal distribution
      /// @param stdDev the standard deviation for the normal distribution
      /// @param seed seed value for the pseudo-random sequence
      RandomGaussian(float mean = 0.0f, float stdDev = 1.0f, uint32_t seed = 0) : m_uniform(seed), m_mean(mean), m_stdDev(stdDev) {}

      /// Generate a random number from the distribution
      /// @return a pseudo-random number
      inline float rand() {
        float x1, x2, rsq;

        do {
          // Pick two uniform numbers within a unit-square and test if they are
          // within a unit-circle, if not, try again
          x1 = 2.0f * m_uniform.rand01() - 1.0f;
          x2 = 2.0f * m_uniform.rand01() - 1.0f;
          rsq = x1 * x1 + x2 * x2;
        } while(rsq >= 1.0f || rsq == 0.0f);

        // Box-Muller transformation and return the other number
        float fac = sqrt((-2.0f * log(rsq)) / rsq);
        return (x2 * fac) * m_stdDev + m_mean;
      }

    private:
      RandomUniform m_uniform;
      float m_mean;
      float m_stdDev;
  };
}

#endif
