/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef NIMBLE_MATH_HPP
#define NIMBLE_MATH_HPP

#include "Export.hpp"

#include <Radiant/Platform.hpp>

#include <cmath>
#include <algorithm>
#ifdef WIN32
# include <float.h>
#endif

#include <QtGlobal>

#include <cstdint>
#include <limits>
#include <cstdlib>

namespace Nimble {

  /** Math functions and constants for Nimble.
      This namespace contains mathematics constants (pi, ln2 etc.) and
      basic functions. **/
  namespace Math
  {
    /// Pi
    const double PI         = 3.1415926535897931;
    /// Pi times two
    const double TWO_PI     = 6.2831853071795862;
    /// Pi divided by two
    const double HALF_PI    = 1.57079632679489660;
    /// Pi divided by four
    const double QUARTER_PI = 0.78539816339744828;

    /// Square root of two
    const double SQRT2      = 1.41421356237309514547;
    /// Square root of two divided by two
    const double SQRT2_PER2 = 0.70710678118654757273;

    /// Returns the inverse square root
    inline float InvSqrt(float v) { return 1.0f / std::sqrt(v); }
    /// Returns the logarithm in base 2
#if defined(RADIANT_CXX11) && !defined(CLANG_XML)
    inline float Log2(float v) { return std::log2(v); }
#else
    inline float Log2(float v) { return std::log(v) / std::log(2.f); }
#endif

    /// Returns the inverse square root
    inline double InvSqrt(double v) { return 1.0 / std::sqrt(v); }
    /// Returns the logarithm in base 2
#if defined(RADIANT_CXX11) && !defined(CLANG_XML)
    inline double Log2(double v) { return std::log2(v); }
#else
    inline double Log2(double v) { return std::log(v) / std::log(2.0); }
#endif

    /// Converts degrees into radians
    inline double degToRad(const double degrees) { return (degrees * PI / 180.0); }
    /// Converts radians to degrees
    inline double radToDeg(const double radians) { return (radians * 180.0 / PI); }

    /// Checks if the given value if finite
    inline bool isFinite(float v)
    {
#if defined(_MSC_VER) && !defined(CLANG_XML)
      return _finite(v) != 0;
#elif defined(_MSC_VER) && defined(CLANG_XML)
      return v == v && (v == 0 || v != 2*v);
#else
      return std::isfinite(v);
#endif
    }
    /// Checks if the given number is a NaN
    /// @param v number to check
    /// @return Check result
    inline bool isNAN(float v)
    {
#if defined(_MSC_VER) && !defined(CLANG_XML)
      return _isnan(v) != 0;
#elif defined(_MSC_VER) && defined(CLANG_XML)
      return v != v;
#else
      return std::isnan(v);
#endif
    }

    /// Return sign.
    template <class T>
    inline int Sign(T v) { return ((v < T(0)) ? -1 : ((v == T(0)) ? 0 : 1)); }

    /// Returns the maximum of the values
    template <class T>
    inline T Max(T x, T y) { return x > y ? x : y; }
    /// Returns the maximum of the values
    template <class T>
    inline T Max(T a, T b, T c) { return std::max(a, std::max(b,c)); }
    /// Returns the maximum of the values
    template <class T>
    inline T Max(T a, T b, T c, T d) { return std::max(std::max(a, b), std::max(c, d)); }

    /// Returns the minimum of the values
    template <class T>
    inline T Min(T x, T y) { return x < y ? x : y; }
    /// Returns the minimum of the values
    template <class T>
    inline T Min(T a, T b, T c) { return std::min(a, std::min(b, c)); }
    /// Returns the minimum of the values
    template <class T>
    inline T Min(T a, T b, T c, T d) { return std::min(std::min(a, b), std::min(c, d)); }

    /// Calculates the fraction of the floating point number
    template <class T>
    inline T Fraction(T x) { return x - (int) x; }

    /** Checks if two (floating point) numbers are close to each
      other.  This function is usually used to check if two
      floating-point numbers are close to each other - the numbers do
      not need to be exactly the same.

      @param a The first value to compare.
      @param b The second value to compare.
      @param limit The maximum difference between the values.
      @return True if the difference between the numbers is small enough
      */
    template <class T>
    bool IsClose(const T & a, const T & b, const T & limit)
    {
      return std::abs(a - b) < limit;
    }

    /// Compare two values
    /// @param a first value to compare
    /// @param b second value to compare
    /// @return true if the values are close to identical; otherwise false
    template<typename T>
    bool fuzzyCompare(const T & a, const T & b)
    {
      return qFuzzyCompare(a, b);
    }

    template<>
    inline bool fuzzyCompare(const int & a, const int & b)
    {
      return a == b;
    }

    template<class T, typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
    bool isNull(T value)
    {
      return value == T(0);
    }

    template<class T, typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
    bool isNull(T value)
    {
      return qIsNull(value);
    }

    /// Rounds the given number to nearest integer
    inline int Round(float x) { return x > 0.0f ? (int) (x + 0.5f) : (int) (x - 0.5f); }
    /// Rounds the given number to nearest integer
    inline int Round(double x) { return x > 0.0 ? (int) (x + 0.5) : (int) (x - 0.5); }
    /// Same thing as std::round with floating point types
    /// Only exists because visual studio 2012 doesn't implement std::round
    template <typename T>
    T Roundf(T x) { return x >= T(0) ? std::floor(x + T(0.5)) : std::ceil(x - T(0.5)); }

    /// Clamp a value between minimum and maximum values
    /// @param x The input value to limit.
    /// @param low The minimum value for comparison
    /// @param high The maximum value for comparison
    /// @return Clamped value
    template <class T>
    inline T Clamp(T x, T low, T high)
    {
      if(x < low) return low;
      if(x > high) return high;
      return x;
    }

    /// Wraps the input value between minimum and maximum values
    /// For example you can normalize radians by Wrap(angle, 0, 2*PI)
    /// @param x The input value to wrap.
    /// @param low The minimum value
    /// @param high The maximum value
    /// @return Wrapped values
    template <class T>
    inline T Wrap(T x, T low, T high)
    {
      T diff = high - low;
      return x - std::floor((x - low) / diff) * diff;
    }

    /// Calculates the determinant of a 2x2 matrix, which is given in
    /// the argument values.
    /// @param a upper-left of the matrix
    /// @param b upper-right of the matrix
    /// @param c lower-left of the matrix
    /// @param d lower-right of the matrix
    /// @return Determinant
    template <class T>
    inline T Det(T a, T b, T c, T d)
    {
      return a * d - b * c;
    }

    /// Calculates the average of arguments a and b.
    /// @param a first argument
    /// @param b second argument
    /// @return (a + b) * 0.5f;
    template <class T>
    inline T Average(const T & a, const T & b)
    {
      return (a + b) * 0.5f;
    }

    /// Given three points on a line, interpolate between them
    template <class T>
        inline T threePointInterpolation
        (float x1, const T & v1, float x2, const T & v2, float x3, const T & v3,
         float x)
    {
      if(x < x1)
        return v1;
      else if(x > x3)
        return v3;
      else if(x < x2) {
        float dx = x2 - x1;
        float rel = (x - x1) / dx;
        return rel * v2 + (1.0f - rel) * v1;
      }
      else {
        float dx = x3 - x2;
        float rel = (x - x2) / dx;
        return rel * v3 + (1.0f - rel) * v2;
      }
    }

    /// Perform linear interpolation between two samples
    template<class T>
    T lerp(const T & a, const T & b, float t)
    {
      return (1.f - t) * a + t * b;
    }

    /// Perform bi-linear interpolation between four samples
    template<class T>
    T bilerp(const T & s00, const T & s10, const T & s01, const T & s11, float u, float v)
    {
      return lerp<T>(lerp<T>(s00, s10, u),
                     lerp<T>(s01, s11, u),
                     v);
    }

    /// Interpolate smoothly between two values based on third (Texturing and
    /// Modeling, Third Edition: A Procedural Approach, by Ken Perlin)
    template<class T>
    T smoothstep(const T & a, const T & b, float t)
    {
      t = Clamp((t - a) / (b - a), T(0), T(1));

      return t * t * t * (t * (t * T(6) - T(15)) + T(10));
    }

    /// Calculates the mean and variance of a buffer of values
    template <class T>
        inline void calculateMeanVariance(const T * values, int n, T * mean, T * variance)
    {
      T ave = 0;
      for(int i = 0; i < n; i++)
        ave += values[i];

      ave /= (double) n;
      *mean = ave;

      T vari = 0;
      for(int i = 0; i < n; i++) {
        T tmp = values[i] - ave;
        vari += tmp * tmp;
      }
      *variance = vari / (double) n;
    }

    /// Calculates the sum of the absolute values in the argument array.
    /// @param values Array of values
    /// @param n Number of elements in the array
    /// @return Sum of absolute values
    template <class T>
    inline T calculateAbsSum(const T * values, int n)
    {
      T sum = 0;
      for(int i = 0; i < n; i++)
        sum += std::abs(values[i]);

      return sum;
    }

    /// Calculate the two principal axes in 2d data
    /// Length of the axes are the corresponding eigenvalues
    /// @param values Array of values, the value type must support operator[] for indices 0 and 1
    /// @param n Number of values
    /// @param axis1 The major axis
    /// @param axis2 The minor axis
    /// @param variance1 Variance for the projection of values to the major axis
    /// @param variance2 Variance for the projection of values to the minor axis
    template <class T, class U>
    void calculatePrincipalAxes(const T* values, int n, U & axis1, U & axis2, float * variance1 = 0, float * variance2 = 0)
    {
      double mean[] = { 0, 0 };

      for(int i=0; i < n; ++i) {
        mean[0] += values[i][0];
        mean[1] += values[i][1];
      }
      mean[0] /= n;
      mean[1] /= n;

      float covariance[3] = { 0, 0, 0 };


      for(int i=0; i < n; ++i) {
        double vx = values[i][0] - mean[0];
        double vy = values[i][1] - mean[1];

        covariance[0] += vx*vx;
        covariance[1] += vx*vy;
        covariance[2] += vy*vy;
      }

      covariance[0] /= n;
      covariance[1] /= n;
      covariance[2] /= n;


      if(std::abs(covariance[1]) < 1e-5f) {

        double smaller = std::min(covariance[0], covariance[2]);
        double bigger = std::max(covariance[0], covariance[2]);

        axis1[0] = 1;
        axis1[1] = 0;

        axis2[0] = 0;
        axis2[1] = 1;

        if(variance1)
          *variance1 = bigger;
        if(variance2)
          *variance2 = smaller;

      } else {
        // Eigenvalues are roots of x^2 + bx + c = 0
        double b = -covariance[0] - covariance[2];
        double c = -covariance[1]*covariance[1] + covariance[0]*covariance[2];

        double discr = std::sqrt(b*b - 4.0*c);
        double q = -0.5*(b + Nimble::Math::Sign(b)*discr);

        double eigs[] = { q, c/q };

        double e1 = Max(eigs[0], eigs[1]);
        double e2 = Min(eigs[0], eigs[1]);

        double v1x = (e1-covariance[2])/covariance[1];
        double v2x = (e2-covariance[2])/covariance[1];

        double l1 = 1.0/std::sqrt(v1x*v1x+1);
        double l2 = 1.0/std::sqrt(v2x*v2x+1);

        axis1[0] = (v1x * l1);
        axis1[1] = l1;

        axis2[0] = v2x * l2;
        axis2[1] = l2;

        if(variance1)
          *variance1 = e1;
        if(variance2)
          *variance2 = e2;
      }
    }
  }

}

#endif
