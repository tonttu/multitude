/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef RESONANT_LIMITERALGORITHM_HPP
#define RESONANT_LIMITERALGORITHM_HPP

#include <Radiant/RingBuffer.hpp>

namespace Resonant {

  /** A level meter. This class is completely inlined.

      A level meter measures signal amplitude. It is assumed that the
      signal is always positive (take absolute value of your data,
      before feeding it here, if necessary).  */

  template <class T> class LevelMeasurement
  {
  public:
    /// Creates a new level meter (no further initializations needed)
    LevelMeasurement() {reset();}
    ~LevelMeasurement() {}

    /** Puts a new sample to the meter. */
    void put(
        /// New sample (absolute value)
        T vabs,
        /// Time to hold the peak value
        unsigned holdTime)
    {
      if(!--m_time) {
        m_time = holdTime;
        m_v1 = m_v2;
        m_v2 = 0.0;
      }

      if(vabs > m_v1) {
        m_time = holdTime;
        m_v2 = m_v1;
        m_v1 = vabs;
      }
      else if(vabs > m_v2) {
        m_v2 = vabs;
      }
    }

    /** Puts a new sample to the meter. */
    void put(
        /// New sample
        T value,
        /// Floor value
        T vfloor,
        /// Time to hold the peak value
        unsigned holdTime)
    {
      if(!--m_time) {
        m_time = holdTime;
        m_v1 = m_v2;
        m_v2 = vfloor;
      }

      if(value > m_v1) {
        m_time = holdTime;
        m_v2 = m_v1;
        m_v1 = value;
      }
      else if(value > m_v2) {
        m_v2 = value;
      }
    }

    /// Resets level to zero.
    void reset()
    { m_time = 1; m_v1 = m_v2 = 0.0; }

    /// Resets level to zero.
    void reset(T value)
    { m_time = 1; m_v1 = m_v2 = value; }

    /// Current peak value
    const T &peak() const {return m_v1;}

  protected:
    unsigned m_time;
    T m_v1, m_v2;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class ChannelLimiter
  {
  public:

    ChannelLimiter() {}

    void prepare(float thresholdLog, unsigned maxDelay)
    {
      m_buffer.resize(maxDelay);
      m_buffer.setAll(0);
      m_logBuffer.resize(maxDelay);
      m_logBuffer.setAll(thresholdLog);
      m_level.reset(thresholdLog);
      m_gain = 0.0;
      m_step  = 0.0;
      m_untilPeak = 0;
      m_maxDelay = maxDelay;
    }

    float putGet(float insample,
                 float thresholdLog,
                 unsigned attackTime,
                 unsigned releaseTime);

    float gain() const{return m_gain; }

  protected:
    Radiant::RingBufferDelay<float> m_buffer;
    Radiant::RingBufferDelay<float> m_logBuffer;
    LevelMeasurement<float>         m_level;
    float                           m_gain;
    float                           m_step;
    unsigned                        m_untilPeak;
    unsigned                        m_maxDelay = 0;
    unsigned                        m_zeroSamples = 0;
  };

  }

#endif // LIMITERALGORITHM_HPP
