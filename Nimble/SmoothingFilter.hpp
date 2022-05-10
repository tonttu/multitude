/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_SMOOTHINGFILTER_HPP
#define NIMBLE_SMOOTHINGFILTER_HPP

#include "Vector4.hpp"

namespace Nimble {

  /// This class uses simple first-order IIR filter to provide a smoothing of
  /// input samples. The first input sample will define the value of the filter
  /// output instantly, while input of subsequent samples will have a slower
  /// reaction to the output value.
  template <class T>
  class SmoothingFilterT
  {
  public:
    /// Data type of the vector
    typedef T type;

    /// Construct a new RollingAverage
    SmoothingFilterT() : m_any(false) {}

    /// Reset the average to the given value
    /// @param value value to reset to
    void reset(const T & value)
    {
      m_value = value;
      m_any = true;
    }

    /// Reset the average to invalid state, i.e. isValid() will return false.
    void reset()
    {
      m_any = false;
    }

    /// Add a sample to the average weighted by given smoothing.
    /// @param value sample to add
    /// @param smoothing history weight, ranging from [0,1]. The actual sample is weighted by 1-smoothing.
    void putSample(const T & value, float smoothing)
    {
      if(m_any) {
        m_value = m_value * smoothing + value * (1.0f - smoothing);
      }
      else {
        m_value = value;
        m_any = true;
      }
    }

    /// Check if the average is valid, i.e. it has at least one sample in it.
    /// @return true if the average is valid; otherwise false
    bool isValid() const { return m_any; }

    /// Get the current value of the average
    /// @return current average value
    const T & value() const { return m_value; }

  private:
    T m_value;
    bool m_any;
  };

  /// Rolling average of floats
  typedef SmoothingFilterT<float> SmoothingFilterFloat;
  /// Rolling average of doubles
  typedef SmoothingFilterT<double> SmoothingFilterDouble;
  /// Rolling average of Vector2s
  typedef SmoothingFilterT<Vector2> SmoothingFilterVector2;
  /// Rolling average of Vector3s
  typedef SmoothingFilterT<Vector3> SmoothingFilterVector3;
  /// Rolling average of Vector4s
  typedef SmoothingFilterT<Vector4> SmoothingFilterVector4;
}

#endif // NIMBLE_ROLLINGAVERAGE_HPP
