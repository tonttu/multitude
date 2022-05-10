/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_RANDOM_HPP
#define NIMBLE_RANDOM_HPP

#include "Export.hpp"
#include "Rect.hpp"

#include <cstdint>

#include <random>

namespace Nimble {

  /// Random number generator with uniform distribution
  /** This class generates random numbers with uniform
      distribution.

      It uses STL's Mersenne Twister implementation for random number
      generation.

      The random number sequence is identical on all platforms, given
      the same seed value.
  */
  class NIMBLE_API RandomUniform
  {
  public:
    /// Constructs a new random number generator with the given seed value
    RandomUniform(uint64_t val = randomSeed()) : m_rand(val)
    {
    }

    ~RandomUniform() {}

    /// Returns a random number in a half-open interval [0, 1)
    inline float rand01()
    {
      std::uniform_real_distribution<float> dst;
      return dst(m_rand);
    }

    /// Returns a random number in a half-open interval [0, x)
    template <class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
    inline T rand0X(T x)
    {
      std::uniform_real_distribution<T> dst(0, x);
      return dst(m_rand);
    }

    /// Returns a random number in a half-open interval [0, x)
    template <class T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    inline T rand0X(T x)
    {
      assert(x != 0);
      std::uniform_int_distribution<T> dst(0, x - 1);
      return dst(m_rand);
    }

    /// Returns a random number in a half-open interval [0, x)
    // This version will be called by types that implicitly convert to uint32_t
    // but are not integral types, like enum values
    inline uint32_t rand0X(uint32_t x)
    {
      assert(x != 0);
      std::uniform_int_distribution<uint32_t> dst(0, x - 1);
      return dst(m_rand);
    }

    /// Returns a random number in a closed interval [-1, 1]
    inline float rand11()
    {
      std::uniform_real_distribution<float> dst(-1.f, std::nextafter(1.0f, std::numeric_limits<float>::max()));
      return dst(m_rand);
    }

    /// Returns a random number in a closed interval [-x, x]
    inline float randXX(float x)
    {
      std::uniform_real_distribution<float> dst(-x, std::nextafter(x, std::numeric_limits<float>::max()));
      return dst(m_rand);
    }

    /// Returns a random number in a half-open interval [a, b) if a < b, else [b, a)
    inline float randRange(float a, float b)
    {
      if(b < a) std::swap(a, b);
      return randMinMax(a, b);
    }

    /// Returns a random number in a half-open interval [min, max)
    inline float randMinMax(float min, float max)
    {
      std::uniform_real_distribution<float> dst(min, max);
      return dst(m_rand);
    }

    /// Returns a random number in a half-open interval [0, 2^32)
    inline uint32_t rand()
    {
      std::uniform_int_distribution<uint32_t> dst;
      return dst(m_rand);
    }

    /// Returns a random number in a half-open interval [0, 2^24)
    inline uint32_t rand24()
    {
      std::uniform_int_distribution<uint32_t> dst(0, (1 << 24) - 1);
      return dst(m_rand);
    }

    /// Returns a random number in a half-open interval [0, 2^32)
    inline uint32_t rand32()
    {
      return rand();
    }

    /// Returns a random number in a half-open interval [0, 2^64)
    inline uint64_t rand64()
    {
      return m_rand();
    }

    /// Random 2d vector inside a rectangle
    inline Nimble::Vector2f randVec2InRect(const Nimble::Rectf & r)
    {
      return Nimble::Vector2f(randMinMax(r.low().x, r.high().x),
                              randMinMax(r.low().y, r.high().y));
    }

    /// Random 2d vector on a circle
    /// @param radius The radius of the circle
    inline Nimble::Vector2f randVecOnCircle(float radius = 1.0f)
    {
      float a = rand0X(static_cast<float>(Math::TWO_PI));
      return Nimble::Vector2f(cosf(a) * radius, sinf(a) * radius);
    }

    /// Random 2d vector on or inside a circle
    /// @param radius The radius of the circle
    inline Nimble::Vector2f randVecInCircle(float radius = 1.0f)
    {
      return randVecOnCircle(radius)*rand01();
    }

    /// Random 3d vector on a sphere
    /// @param radius The radius of the sphere
    inline Nimble::Vector3f randVecOnSphere(float radius = 1.0f)
    {
      // see http://mathworld.wolfram.com/SpherePointPicking.html
      float fi = acos(rand0X(2.f) - 1);
      float theta = rand0X(static_cast<float>(Nimble::Math::TWO_PI));

      float sinfi = sin(fi);
      Nimble::Vector3f v(cos(theta)*sinfi, sin(theta)*sinfi, cos(fi));
      return radius*v;
    }

    /// Random 3d vector inside or on a sphere
    /// @param radius The radius of the sphere
    inline Nimble::Vector3f randVecInSphere(float radius = 1.0f)
    {
      float r = rand01();
      return randVecOnSphere(radius)*powf(r, 1.f/3.f);
    }

    /// Random boolean
    /// @return True or false.
    inline bool randBool()
    {
      return std::uniform_int_distribution<uint32_t>(0,1)(m_rand);
    }

    /// Returns the internal pseudo random number generator engine
    inline std::mt19937_64 & engine()
    {
      return m_rand;
    }

    /// Returns a reference to an instance.
    static RandomUniform & instance() { return m_instance; }

    /// Returns a new random seed
    static uint64_t randomSeed();

  private:
    std::mt19937_64 m_rand;
    static RandomUniform m_instance;
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
      RandomGaussian(float mean = 0.0f, float stdDev = 1.0f, uint64_t seed = RandomUniform::randomSeed())
        : m_rand(seed), m_dist(mean, stdDev) {}

      /// Generate a random number from the distribution
      /// @return a pseudo-random number
      inline float rand()
      {
        return m_dist(m_rand);
      }

      /// Returns the internal pseudo random number generator engine
      inline std::mt19937_64 & engine()
      {
        return m_rand;
      }

    private:
      std::mt19937_64 m_rand;
      std::normal_distribution<float> m_dist;

  };
}

#endif
