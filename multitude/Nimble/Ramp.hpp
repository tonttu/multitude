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

#ifndef NIMBLE_RAMPT_HPP
#define NIMBLE_RAMPT_HPP

#include <Nimble/Export.hpp>

namespace Nimble {
  
  /// Linear interpolation.
  /** This class is used to interpolate between key-point values, with
      fixed-length intervals. */
  template <class T>
  class /*NIMBLE_API*/ RampT
  {
  public:
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
    void setTarget(const T & target, unsigned n)
    {
      m_target = target;
      m_step = (target - m_current) / (float) n;
      m_left = n;
    }

    /// Jumps directly to the target value, bypassing interpolation
    void toTarget()
    {
      m_current = m_target;
      m_left = 0;
    }

    /// Updates the interpolator
    void update()
    {
      if(m_left) {
	if(!--m_left)
	  m_current = m_target;
	else
	  m_current += m_step;
      }
    }
    
    /// Gets the current value
    const T & value() const { return m_current; }

    /// Gets the target value
    const T & target() const { return m_target; }

    /// The number of steps left to reach the target value
    unsigned left() const { return m_left; }

  private:
    
    T m_step;
    T m_current;
    T m_target;
    unsigned m_left;
  };

  /// Ramp of floats
  typedef RampT<float> Rampf;
  /// Ramp of doubles
  typedef RampT<double> Rampd;

  /// @todo not needed anymore?
#ifdef WIN32
#ifdef NIMBLE_EXPORT
  // In WIN32 template classes must be instantiated to be exported
  template class RampT<float>;
  template class RampT<double>;
#endif
#endif

}

#endif
