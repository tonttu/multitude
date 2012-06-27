/* COPYRIGHT
 *
 * This file is part of MultiBrowser2.
 *
 * Copyright: MultiTouch Oy, Finland, http://multitouch.fi
 *
 * All rights reserved, 2007-2010
 *
 * You may use this file only for purposes for which you have a
 * specific, written permission from MultiTouch Oy.
 *
 * See file "MultiBrowser2.hpp" for authors and more details.
 *
 */

#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <Radiant/TimeStamp.hpp>

#include <Nimble/Math.hpp>

#include <vector>

namespace Radiant
{

  /// @todo document and cleanup
  template <typename T>
  class Buffer
  {
  public:
    /// @param history length of history in seconds
    Buffer(const T & t, double history)
      : m_history(Radiant::TimeStamp::createSecondsD(history)),
        m_pos(-1),
        m_value(t),
        m_cached(true)
    {
      m_data.resize(Nimble::Math::Max(10, int(history*120)));
    }

    void add(const T & t, Radiant::TimeStamp ts = Radiant::TimeStamp::getTime())
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

    // Sets the average of the buffer to given value
    void set(const T & t)
    {
      m_value = t;
      for(size_t i = 0; i < m_data.size(); ++i) m_data[i].ts = 0;
      m_cached = true;
    }

    /// Compute the average of the history values
    T avg(Radiant::TimeStamp ts = Radiant::TimeStamp::getTime()) const
    {
      if(m_pos < 0 || m_data.empty()) return T();
      T avg = m_data[m_pos].value;
      int num = 1;
      for(size_t i = 1; i <= m_data.size(); ++i) {
        const int j = m_pos-i;
        const BufferValue & b = m_data[(j + m_data.size() * 2) % m_data.size()];
        if(b.ts == 0 || ts-b.ts > m_history) break;
        ++num;
        avg += b.value;
      }
      return avg / float(num);
    }

    T operator* () const
    {
      if(m_cached) return m_value;
      return avg();
    }

    /// Forget given sample
    void forget(int number)
    {
      m_pos -= number;
      while(m_pos < 0) m_pos += m_data.size();
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
