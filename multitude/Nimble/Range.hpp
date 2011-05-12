#ifndef NIBMLE_RANGE_HPP
#define NIBMLE_RANGE_HPP

#include <Nimble/Export.hpp>

namespace Nimble {

  /** Numeric range representation.

      By default the low, and high values are set to zero.

  */
  template <class T>
  class NIMBLE_API RangeT
  {
  public:
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

    /** Clamps the argument value to be between the low and high limits. */
    inline T clamp(const T & v) const
    {
      if(v <= m_low) return m_low;
      if(v >= m_high) return m_high;
      return v;
    }

    /// Checks if the argument value fits into this range
    inline bool contains(const T & v) const
    {
      return (v >= m_low) && (v <= m_high);
    }

    /// Compares two RangeT objects.
    /** @return This function returns true if the two ranges are absolutely identical. */
    inline bool operator == (const RangeT & that) const
    {
      return m_low == that.m_low && m_high == that.m_high;
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

  typedef RangeT<double> Ranged;
  typedef RangeT<float> Rangef;
  typedef RangeT<long> Rangel;
  typedef RangeT<int> Rangei;

}

// These are needed under Windows
#ifdef WIN32
#   ifdef NIMBLE_EXPORT
        template Nimble::RangeT<double>;
        template Nimble::RangeT<float>;
        template Nimble::RangeT<long>;
        template Nimble::RangeT<int>;
#   endif
#endif

#endif // RANGE_HPP
