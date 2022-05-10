/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIBMLE_RANGE_HPP
#define NIBMLE_RANGE_HPP

#include "Export.hpp"

namespace Nimble {

  /** Numeric range representation.

      By default the low, and high values are set to zero.

  */
  template <class T>
  class RangeT
  {
  public:
    /// Create a new range with low and high values
    /// @param low Initial low value
    /// @param high Initial high value
    RangeT(const T & low, const T & high)
      : m_low(low), m_high(high) {}

    RangeT()
      : m_low(0), m_high(0) {}

    /// Sets the low-, and high values to given value
    inline void reset(const T & v) { m_low = v; m_high = v; }

    /// Check ifthe range is empty
    /** @return This function returns true if the low value is equal to,
        or exceeds the high value. */
    inline bool isEmpty() const { return m_low >= m_high; }

    /// The difference between the upper and lower limit
    inline T span() const { return m_high - m_low; }
    /// Absolute value of the #span
    inline T spanAbs() const { T r = m_high - m_low; return (r >= 0) ? r : -r; }

    /// The lower limit of the range
    inline T low()  const { return m_low; }
    /// The upper limit of the range
    inline T high() const { return m_high; }

    inline void setLow(const T & low) { m_low = low; }
    inline void setHigh(const T & high) { m_high = high; }

    /// Clamps the argument value to be between the low and high limits.
    /// @param v Value to clamp
    /// @return Clamped value
    inline T clamp(const T & v) const
    {
      if(v <= m_low) return m_low;
      if(v >= m_high) return m_high;
      return v;
    }

    /// Checks if the argument value fits into this range
    /// @param v Value to check
    /// @return True if range contains this value
    inline bool contains(const T & v) const
    {
      return (v >= m_low) && (v <= m_high);
    }

    /// Compares two RangeT objects.
    /// @param that Other range to compare to
    /// @return This function returns true if the two ranges are absolutely identical.
    inline bool operator == (const RangeT & that) const
    {
      return m_low == that.m_low && m_high == that.m_high;
    }

    /// Compares two RangeT objects.
    /// @param that Other range to compare to
    /// @return This function returns true if the two ranges are different.
    inline bool operator != (const RangeT & that) const
    {
      return !(*this == that);
    }

    /// Expands the range to include the given value
    inline void expand(const T & v)
    {
      if(m_low > v) m_low = v;
      if(m_high < v) m_high = v;
    }

    /// Expands the range to include the given range
    inline void expand(const RangeT & that)
    {
      if(m_low > that.m_low) m_low = that.m_low;
      if(m_high < that.m_high) m_high = that.m_high;
    }

  private:
    T m_low, m_high;
  };

  /// RangeT for doubles
  typedef RangeT<double> Ranged;
  /// RangeT for float
  typedef RangeT<float> Rangef;
  /// RangeT for long ints
  typedef RangeT<long> Rangel;
  /// RangeT for ints
  typedef RangeT<int> Rangei;

}

#endif // RANGE_HPP
