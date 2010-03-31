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

#ifndef NIMBLE_MATH_HPP
#define NIMBLE_MATH_HPP

#include <Nimble/Export.hpp>

#include <math.h>
#ifdef WIN32
#	include <float.h>
#endif

namespace Nimble {

  /// Mathematics functions and constants for Nimble.
  /** This namespace contains mathematics constants (pi, ln2 etc.) and
      basic functions. */
  namespace Math
  {
    // Constants:

    // for real number comparisons
    const double TOLERANCE  = 0.000001f;

    const double PI         = 3.1415926535897931;
    const double TWO_PI     = 6.2831853071795862;
    const double HALF_PI    = 1.57079632679489660;
    const double QUARTER_PI = 0.78539816339744828;

    const double SQRT2      = 1.41421356237309514547;
    const double SQRT2_PER2 = 0.70710678118654757273;
    const double EPSILON    = 1.0e-10;

    // float & double inlines:

    inline float Cos(float v)  { return cosf(v); }
    inline float Sin(float v)  { return sinf(v); }  
    inline float Tan(float v)  { return tanf(v); }  
    inline float Sqrt(float v) { return sqrtf(v); }
    inline float InvSqrt(float v) { return 1.0f / sqrtf(v); }
    inline float Exp(float v)  { return expf(v); }
    inline float Log(float v)  { return logf(v); }
	inline float Log2(float v) { return Log(v) / Log(2.f); }
    inline float Pow(float x, float y)    { return powf(x, y); }

    inline float ACos(float v)  { return acosf(v); }
    inline float ASin(float v)  { return asinf(v); }
    inline float ATan(float v)  { return atanf(v); }
    inline float ATan2(float x, float y)  { return atan2f(x, y); }

    inline double Cos(double v)  { return cos(v); }
    inline double Sin(double v)  { return sin(v); } 
    inline double Tan(double v)  { return tan(v); }  
    inline double Sqrt(double v) { return sqrt(v); }
    inline double InvSqrt(double v) { return 1.0 / sqrt(v); }
    inline double Exp(double v)  { return exp(v); }
    inline double Log(double v)  { return log(v); }
	inline double Log2(double v)  { return Log(v) / Log(2.0); }
    inline double Pow(double x, double y)    { return pow(x, y); }

    inline double ACos(double v)  { return acos(v); }
    inline double ASin(double v)  { return asin(v); }
    inline double ATan(double v)  { return atan(v); }
    inline double ATan2(double x, double y)  { return atan2(x, y); }

    inline float Sqrt(int v) { return sqrtf(float(v)); }

    inline double degToRad(const double degrees) { return (degrees * PI / 180.0); }
    inline double radToDeg(const double radians) { return (radians * 180.0 / PI); }

    inline bool isFinite(float v) 
	{
#ifdef WIN32
      return _finite(v) != 0;
#else
    return finite(v);
#endif      
    }
	
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

    template <class T>
    inline T Max(T x, T y) { return x > y ? x : y; }
    template <class T>
    inline T Max(T a, T b, T c) { return Max(a, Max(b,c)); }
    template <class T>
    inline T Max(T a, T b, T c, T d) { return Max(Max(a, b), Max(c, d)); }

    template <class T>
    inline T Min(T x, T y) { return x < y ? x : y; }
    template <class T>
    inline T Min(T a, T b, T c) { return Min(a, Min(b, c)); }
    template <class T>
    inline T Min(T a, T b, T c, T d) { return Min(Min(a, b), Min(c, d)); }

    /// Calculates the absolute value of the argument.
    template <class T>
    inline T Abs(T x) { return (x > T(0)) ? x : -x; }

    /// Calculates the fraction of the floating point number
    template <class T>
    inline T Fraction(T x) { return x - (int) x; }

    /// Seeks the maximum value in an vector
    /** VMax = vector maximum. 
     */
    template <class T>
    inline T VMax(const T * vals, int n)
    {
      T v = *vals;
      const T * sentinel = vals;
      vals++;

      for(; vals < sentinel; vals++)
	if(v < *vals) v = * vals;

      return v;
    }

    /// Seeks the minimum value in an vector
    /** VMin = vector minimum. */
    template <class T>
    inline T VMin(const T * vals, int n)
    {
      T v = *vals;
      const T * sentinel = vals;
      vals++;

      for(; vals < sentinel; vals++)
	if(v > *vals) v = * vals;

      return v;
    }

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

    inline int Round(float x) { return x > 0.0f ? (int) (x + 0.5f) : (int) (x - 0.5f); }
    inline int Round(double x) { return x > 0.0 ? (int) (x + 0.5) : (int) (x - 0.5); }

    inline int Ceil(float x) { return x >= 0.0f ? (int) (x + 0.99999f) : (int) (x); }
    inline int Floor(float x) { return x >= 0.0f ? (int)x : (int) (x - 0.9999f); }
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

    /** Calculates the determinant of a 2x2 matrix, which is given in
	the argument values. */
    template <class T>
    inline T Det(T a, T b, T c, T d) 
    { 
      return a * d - b * c;
    }

    /** Calculates the average of arguments a and b.
	
	@return (a + b) * 0.5f;
     */
    template <class T>
    inline T Average(const T & a, const T & b)
    {
      return (a + b) * 0.5f;
    }

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
  }

  
}

#endif
