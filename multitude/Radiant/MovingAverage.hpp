/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef MOVING_AVERAGE_HPP
#define MOVING_AVERAGE_HPP

#include <Radiant/TimeStamp.hpp>

#include <Nimble/Math.hpp>

#include <vector>

namespace Radiant
{

  /// This class provides an implementation for calculating a moving average
  /// with a fixed window.
  template <typename T>
  class MovingAverage
  {
  public:
    /// Construct a new buffer
    /// @param t initial value
    /// @param history history length in seconds
    MovingAverage(const T & t, double history)
      : m_history(Radiant::TimeStamp::createSeconds(history)),
        m_pos(-1),
        m_value(t),
        m_cached(true)
    {
      m_data.resize(std::max(10, int(history*120)));
    }

    /// Add a sample with the given timestamp to the buffer
    /// @param t sample value
    /// @param ts timestamp of the value
    void add(const T & t, Radiant::TimeStamp ts = Radiant::TimeStamp::currentTime())
    {
      m_pos = (m_pos+1) % m_data.size();
      if(ts - m_data[m_pos].ts < m_history) {
        m_data.resize(m_data.size() + 10); // std::vector doubles the capacity when needed
        for(int i = m_pos; i < int(m_data.size())-10; ++i) {
          m_data[i+10] = m_data[i];
          m_data[i] = BufferValue();
        }
      }
      m_data[m_pos] = BufferValue(t, ts);
      m_cached = false;
    }

    /// Sets the average of the buffer to given value
    /// @param t new buffer average
    void set(const T & t)
    {
      m_value = t;
      for(size_t i = 0; i < m_data.size(); ++i) m_data[i].ts = Radiant::TimeStamp(0);
      m_cached = true;
    }

    /// Compute the average of the history values before given timestamp
    /// @param ts time limit
    /// @return average
    T avg(Radiant::TimeStamp ts = Radiant::TimeStamp::currentTime()) const
    {
      if(m_pos < 0 || m_data.empty()) return T();
      T avg = m_data[m_pos].value;
      int num = 1;
      for(size_t i = 1; i <= m_data.size(); ++i) {
        const int j = m_pos-static_cast<int>(i);
        const BufferValue & b = m_data[(j + m_data.size() * 2) % m_data.size()];
        if(b.ts == Radiant::TimeStamp(0) || ts-b.ts > m_history) break;
        ++num;
        avg += b.value;
      }
      return avg / float(num);
    }

    /// Implicit conversion to value-type
    /// @return average of all samples in the buffer
    T operator* () const
    {
      if(m_cached) return m_value;
      return avg();
    }

    /// Forget samples
    /// @param number of samples to forget from the end of the buffer
    void forget(int number)
    {
      m_pos -= number;
      while(m_pos < 0) m_pos += m_data.size();
    }

    std::pair<T, Radiant::TimeStamp> interpolatedSample(Radiant::TimeStamp ts) const
    {
      if (m_data[(m_pos+1) % m_data.size()].ts >= ts) {
        // This was the oldest sample, ts is before our time window started, return the oldest sample
        auto s = m_data[(m_pos+1) % m_data.size()];
        return std::make_pair(s.value, s.ts);
      }

      for (std::size_t i = 1; i < m_data.size(); ++i) {
        std::size_t idx = (m_pos + i + 1) % m_data.size();
        if (m_data[idx].ts >= ts) {
          auto s0 = m_data[(m_pos + i) % m_data.size()];
          auto s1 = m_data[idx];

          // This is uninitialized sample, so s1 is the actual oldest sample, just return it
          if (s0.ts.value() == 0)
            return std::make_pair(s1.value, s1.ts);

          double t = double((ts - s0.ts).value()) / (s1.ts - s0.ts).value();
          return std::make_pair(Nimble::Math::lerp(s0.value, s1.value, t), ts);
        }
      }

      // Processed all samples, ts is after any samples, give the latest sample
      return std::make_pair(m_data[m_pos].value, m_data[m_pos].ts);
    }

  private:
    struct BufferValue
    {
      T value;
      Radiant::TimeStamp ts;
      BufferValue(const T & value_, Radiant::TimeStamp ts_) : value(value_), ts(ts_) {}
      BufferValue() : value(T()) {}
    };

    Radiant::TimeStamp m_history;
    int m_pos;
    std::vector<BufferValue> m_data;
    T m_value;
    bool m_cached;
  };

}

#endif
