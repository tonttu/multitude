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
    RangeT(const T & low = 0, const T & high = 0)
      : m_low(low), m_high(high) {}

    /// Check ifthe range is empty
    /** @return This function returns true if the low value is equal to,
        or exceeds the high value. */
    inline bool isEmpty() const { return m_low >= m_high; }

    /// The difference between the upper and lower limit
    inline T span() const { return m_high - m_low; }

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

    /// Compares two RangeT objects.
    /** @return This function returns true if the two ranges are absolutely identical. */
    inline bool operator == (const RangeT & that) const
    {
      return m_low == that.m_low && m_high == that.m_high;
    }

  private:
    T m_low, m_high;
  };

  typedef RangeT<double> Ranged;
  typedef RangeT<float> Rangef;
  typedef RangeT<long> Rangel;
  typedef RangeT<int> Rangei;

}

#endif // RANGE_HPP
