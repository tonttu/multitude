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

  /** Rolling average calculation.

  This class uses simple first-order IIR filter to provide rolling average calculation.
  The first sample will define the value of the average instantly, while input of
  subsequent samples will have cause slow reaction to the input value.

      */
  template <class T>
  class RollingAverageT
  {
  public:
    /// Data type of the vector
    typedef T type;

    RollingAverageT() : m_any(false) {}

    void reset(const T & value)
    {
      m_value = value;
      m_any = true;
    }

    void reset()
    {
      m_any = false;
    }

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

    bool isValid() const { return m_any; }

    const T & value() const { return m_value; }

  private:
    T m_value;
    bool m_any;
  };


  typedef RollingAverageT<float> RollingAverageFloat;
  typedef RollingAverageT<double> RollingAverageDouble;
  typedef RollingAverageT<Vector2> RollingAverageVector2;
  typedef RollingAverageT<Vector3> RollingAverageVector3;
  typedef RollingAverageT<Vector4> RollingAverageVector4;
}

#endif // NIMBLE_ROLLINGAVERAGE_HPP