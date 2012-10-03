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

#ifndef NIMBLE_ROLLINGAVERAGE_HPP
#define NIMBLE_ROLLINGAVERAGE_HPP

#include "Vector4.hpp"

namespace Nimble {

  /// Rolling average calculation.
  ///
  /// This class uses simple first-order IIR filter to provide rolling average
  /// calculation. The first sample will define the value of the average
  /// instantly, while input of subsequent samples will have a slower reaction
  /// to the input value.
  template <class T>
  class RollingAverageT
  {
  public:
    /// Data type of the vector
    typedef T type;

    /// Construct a new RollingAverage
    RollingAverageT() : m_any(false) {}

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
  typedef RollingAverageT<float> RollingAverageFloat;
  /// Rolling average of doubles
  typedef RollingAverageT<double> RollingAverageDouble;
  /// Rolling average of Vector2s
  typedef RollingAverageT<Vector2> RollingAverageVector2;
  /// Rolling average of Vector3s
  typedef RollingAverageT<Vector3> RollingAverageVector3;
  /// Rolling average of Vector4s
  typedef RollingAverageT<Vector4> RollingAverageVector4;
}

#endif // NIMBLE_ROLLINGAVERAGE_HPP
