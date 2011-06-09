/* COPYRIGHT
 */

#ifndef NIMBLE_MATH_HPP
#define NIMBLE_MATH_HPP

#include <Nimble/Export.hpp>

#include <cmath>
#ifdef WIN32
#	include <float.h>
#endif

#include <stdint.h>

namespace Nimble {

  /** Math functions and constants for Nimble.
      This namespace contains mathematics constants (pi, ln2 etc.) and
      basic functions. **/
  namespace Math
  {
    // Constants:

    /// Tolerance used when comparing floating point numbers
    const double TOLERANCE  = 0.000001f;

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
    /// A small number
    const double EPSILON    = 1.0e-10;

    // float & double inlines:

    /// Returns the cosine
    inline float Cos(float v)  { return cosf(v); }
    /// Returns the sine
    inline float Sin(float v)  { return sinf(v); }
    /// Returns the tangent
    inline float Tan(float v)  { return tanf(v); }
    /// Returns the square root
    inline float Sqrt(float v) { return sqrtf(v); }
    /// Returns the inverse square root
    inline float InvSqrt(float v) { return 1.0f / sqrtf(v); }
    /// Returns the exponential function
    inline float Exp(float v)  { return expf(v); }
    /// Returns the logarithm in base 10
    inline float Log(float v)  { return logf(v); }
    /// Returns the logarithm in base 2
    inline float Log2(float v) { return Log(v) / Log(2.f); }
    /// Raises x to the yth power
    inline float Pow(float x, float y)    { return powf(x, y); }

    /// Returns the arccosine
    inline float ACos(float v)  { return acosf(v); }
    /// Returns the arcsine
    inline float ASin(float v)  { return asinf(v); }
    /// Returns the arctangent
    inline float ATan(float v)  { return atanf(v); }
    /// Returns the arctangent
    inline float ATan2(float x, float y)  { return atan2f(x, y); }

    /// Returns the cosine
    inline double Cos(double v)  { return cos(v); }
    /// Returns the sine
    inline double Sin(double v)  { return sin(v); }
    /// Returns the tangent
    inline double Tan(double v)  { return tan(v); }
    /// Returns the square root
    inline double Sqrt(double v) { return sqrt(v); }
    /// Returns the inverse square root
    inline double InvSqrt(double v) { return 1.0 / sqrt(v); }
    /// Returns the exponential function
    inline double Exp(double v)  { return exp(v); }
    /// Returns the logarithm in base 10
    inline double Log(double v)  { return log(v); }
    /// Returns the logarithm in base 2
    inline double Log2(double v)  { return Log(v) / Log(2.0); }
    /// Raises x to the yth power
    inline double Pow(double x, double y)    { return pow(x, y); }

    /// Returns the arccosine
    inline double ACos(double v)  { return acos(v); }
    /// Returns the arcsine
    inline double ASin(double v)  { return asin(v); }
    /// Returns the arctangent
    inline double ATan(double v)  { return atan(v); }
    /// Returns the arctangent
    inline double ATan2(double x, double y)  { return atan2(x, y); }

    /// Returns the square root
    inline float Sqrt(int v) { return sqrtf(float(v)); }
    /// Returns the square root
    inline double Sqrt(int64_t v) { return sqrt(double(v)); }
    /// Returns the square root
    inline double Sqrt(uint64_t v) { return sqrt(double(v)); }

    /// Converts degrees into radians
    inline double degToRad(const double degrees) { return (degrees * PI / 180.0); }
    /// Converts radians to degrees
    inline double radToDeg(const double radians) { return (radians * 180.0 / PI); }

    /// Checks if the given value if finite
    inline bool isFinite(float v)
    {
#ifdef WIN32
      return _finite(v) != 0;
#else
    return finite(v);
#endif
    }
  /// Checks if the given number is not one
  /// @param v number to check
    inline bool isNAN(float v)
    {
#ifdef WIN32
    return _isnan(v) != 0;
#else
        return isnan(v);
#endif
    }

    /// Return sign.
    template <class T>
    inline int Sign(T v) { return ((v < T(0)) ? -1 : ((v == T(0)) ? 0 : 1)); }

    // Min & Max inlines:

    /// Returns the maximum of the values
    template <class T>
    inline T Max(T x, T y) { return x > y ? x : y; }
    /// Returns the maximum of the values
    template <class T>
    inline T Max(T a, T b, T c) { return Max(a, Max(b,c)); }
    /// Returns the maximum of the values
    template <class T>
    inline T Max(T a, T b, T c, T d) { return Max(Max(a, b), Max(c, d)); }

    /// Returns the minimum of the values
    template <class T>
    inline T Min(T x, T y) { return x < y ? x : y; }
    /// Returns the minimum of the values
    template <class T>
    inline T Min(T a, T b, T c) { return Min(a, Min(b, c)); }
    /// Returns the minimum of the values
    template <class T>
    inline T Min(T a, T b, T c, T d) { return Min(Min(a, b), Min(c, d)); }

    /// Calculates the absolute value of the argument.
    template <class T>
    inline T Abs(T x) { return (x > T(0)) ? x : -x; }

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
      */
    template <class T>
    bool IsClose(const T & a, const T & b, const T & limit)
    {
      return Abs(a - b) < limit;
    }

    /// Rounds the given number to nearest integer
    inline int Round(float x) { return x > 0.0f ? (int) (x + 0.5f) : (int) (x - 0.5f); }
    /// Rounds the given number to nearest integer
    inline int Round(double x) { return x > 0.0 ? (int) (x + 0.5) : (int) (x - 0.5); }

    /// Rounds the given number up to nearest integer
    inline int Ceil(float x) { return x >= 0.0f ? (int) (x + 0.99999f) : (int) (x); }
    /// Rounds the given number down to nearest integer
    inline int Floor(float x) { return x >= 0.0f ? (int)x : (int) (x - 0.9999f); }
    /// Rounds the given number down to nearest integer
    inline int Floor(double x) { return x >= 0.0f ? (int)x : (int) (x - 0.9999); }

    /// Convert degrees to radians
    template <class T>
    inline T degToRad(T deg)
    {
      return deg * ((T) PI / (T) 180);
    }

    /// Convert radians to degrees
    template <class T>
    inline T radToDeg(T rad)
    {
      return rad * ((T) 180 / (T) PI);
    }

    /// Clamp a value between minimum and maximum values
    /** @param x The input value to limit.
    @param low The minimum value for comparison
    @param high The maximum value for comparison
     */
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
    template <class T>
    inline T Wrap(T x, T low, T high)
    {
      T diff = high - low;
      return x - Floor((x - low) / diff) * diff;
    }

    /** Calculates the determinant of a 2x2 matrix, which is given in
  the argument values.
    @param a upper-left of the matrix
    @param b upper-right of the matrix
    @param c lower-left of the matrix
    @param d lower-right of the matrix */
    template <class T>
    inline T Det(T a, T b, T c, T d)
    {
      return a * d - b * c;
    }

    /** Calculates the average of arguments a and b.
    @param a first argument
    @param b second argument
    @return (a + b) * 0.5f;
     */
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
      t = Clamp((t - a) / (b - a), 0.f, 1.f);

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

    /** Calculates the sum of the absolute values in the argument array. */
    template <class T>
        inline T calculateAbsSum(const T * values, int n)
    {
      T sum = 0;
      for(int i = 0; i < n; i++)
        sum += Abs(values[i]);

      return sum;
    }

  }
}

#endif
