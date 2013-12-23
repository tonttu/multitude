/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_RAMPT_HPP
#define NIMBLE_RAMPT_HPP

namespace Nimble {

  /// Linear interpolation. This class is used to interpolate between key-point
  /// values. The stepping/time value N van be an integer or floating point
  /// number.
  template <class T, class N>
  class RampT
  {
  public:
    /// Construct a new ramp
    RampT() {}
    /// Construct a new ramp
    /// @param val initial value
    RampT(const T & val) { reset(val); }
    /// Destructor
    ~RampT() {}

    /// Resets this interpolator to the given value
    /// @param val value to reset to
    void reset(const T & val)
    {
      m_current = m_target = val;
      m_left = 0;
    }

    /// Sets the interpolation target
    /// @param target The target value for interpolation
    /// @param n The number of updates required to reach the target value
    void setTarget(const T & target, N n)
    {
      m_target = target;
      if(n != 0)
        m_step = (T) ((target - m_current) / (double) n);
      else {
        m_step = 0;
        m_current = target;
      }
      m_left = n;
    }

    /// Jumps directly to the target value, bypassing interpolation
    void toTarget()
    {
      m_current = m_target;
      m_left = 0;
    }

    /// Updates the interpolator
    inline void update()
    {
      if(m_left > 0) {
        m_left -= 1;
        if(m_left <= 0)
          m_current = m_target;
        else
          m_current += m_step;
      }
    }

    /// Updates the interpolator with given amount of time
    /// @param n steps to interpolate
    inline void update(N n)
    {
      if(m_left > 0) {
        m_left -= n;
        if(m_left <= 0)
          m_current = m_target;
        else {
          m_current += (m_step * n);
        }
      }
    }

    /// Typecast operator that returns the current value of the interpolator
    /// @return current value of the ramp
    inline operator const T & () const { return m_current; }

    /// Gets the current value
    /// @return current value of the ramp
    inline const T & value() const { return m_current; }

    /// Gets the target value
    /// @return target value of the ramp
    inline const T & target() const { return m_target; }

    /// The number of steps left to reach the target value
    /// @return steps left to reach the target
    inline unsigned left() const { return m_left; }

  private:

    T m_step;
    T m_current;
    T m_target;
    N m_left;
  };

  /// Ramp of floats
  typedef RampT<float, unsigned> Rampf;
  /// Ramp of doubles
  typedef RampT<double, unsigned> Rampd;

  /// Ramp of floats
  typedef RampT<float, float> Rampff;
  /// Ramp of doubles
  typedef RampT<double, double> Rampdd;
}

#endif
