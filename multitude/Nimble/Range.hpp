#ifndef NIBMLE_RANGE_HPP
#define NIBMLE_RANGE_HPP

#include <Nimble/Export.hpp>

namespace Nimble {

  /** Numeric range representation. */

  template <class T>
  class NIMBLE_API RangeT
  {
  public:
    RangeT(const T & low = 0, const T & high = 0)
      : m_low(low), m_high(high) {}

    inline bool isEmpty() const { return m_low >= m_high; }

    inline T span() const { return m_high - m_low; }

    inline T low()  const { return m_low; }
    inline T high() const { return m_high; }

    inline T clamp(const T & v) const
    {
      if(v <= m_low) return m_low;
      if(v >= m_high) return m_high;
      return v;
    }

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
