/* COPYRIGHT
 */

#ifndef NIMBLE_RAMPT_HPP
#define NIMBLE_RAMPT_HPP

namespace Nimble {

  /// Linear interpolation.
  /** This class is used to interpolate between key-point values. The stepping/time value N
      van be an integer or floating point number. */
  template <class T, class N>
  class /*NIMBLE_API*/ RampT
  {
  public:
    RampT(const T & val) { reset(val); }
    RampT() {}
    ~RampT() {}

    /// Resets this interpolator to the given value
    void reset(const T & val)
    {
      m_current = m_target = val;
      m_left = 0;
    }

    /// Sets the interpolation target
    /** @param target The target value for interpolation
    @param n The number of updates required to reach the target value */
    void setTarget(const T & target, N n)
    {
      m_target = target;
      m_step = (T) ((target - m_current) / (double) n);
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
    inline operator const T & () const { return m_current; }

    /// Gets the current value
    inline const T & value() const { return m_current; }

    /// Gets the target value
    inline const T & target() const { return m_target; }

    /// The number of steps left to reach the target value
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

#ifdef WIN32
#ifdef NIMBLE_EXPORT
  // In WIN32 template classes must be instantiated to be exported
  template class RampT<float, unsigned>;
  template class RampT<double, unsigned>;
  template class RampT<float, float>;
  template class RampT<double, double>;
#endif
#endif
}

#endif
