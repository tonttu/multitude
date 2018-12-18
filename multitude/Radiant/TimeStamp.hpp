/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_TIMESTAMP_HPP
#define RADIANT_TIMESTAMP_HPP

#include "Export.hpp"
#include "Defines.hpp"

#include <cstdint>
#include <stdio.h>
#include <QString>

class QDateTime;

namespace Radiant {

  /** A high-resolution time-stamp object. A time-stamp has 40 bits
     for the seconds and 24 bits for fractions of a second. The
     seconds are as they come from the UNIX clock: 1.1.1970 the
     counter was zero.

     Thus there are 2^24 (16777216) ticks in a second, which works as the basic timing unit.
     This gives about 16.78 ticks per microsecond, which is good enough to do highly accurate
     timing with any reasonable media source. At the same time the 40-bit second field is
     sufficient to store all dates within the current century, and a few forward/backward.
     The time value is internally stored in a signed 64-bit integer. In most applications
     time-stamps could be unsigned, but this leads easily to annoying bugs if the
     unsignedness is not taken into account in various time computations.

     TimeStamps can be easily casted to/from integer types.

     This design is basically copy of the OSC timing system.
  */

  class RADIANT_API TimeStamp
  {
  public:

    /// The internal, signed 64-bit integer type
    typedef int64_t type;

    static constexpr int64_t FRACTIONS_PER_SECOND { 0x1000000 };

    /// The number of ticks that take place during one second
    /// @return Number of ticks per second
    static TimeStamp ticksPerSecond() { return TimeStamp(FRACTIONS_PER_SECOND); }

    /// The number of ticks that take place during one minute
    /// @return Number of ticks per minute
    static TimeStamp ticksPerMinute() { return TimeStamp(1006632960); }

    /// The number of ticks that take place during one hour
    /// @return Number of ticks per hour
    static TimeStamp ticksPerHour()   { return TimeStamp(60397977600ll); }

    /// The number of ticks that take place during one day
    /// @return Number of ticks per day
    static TimeStamp ticksPerDay()    { return TimeStamp(1449551462400ll); }

    /// Constructs a timestamp dated to 1.1.1970
    TimeStamp() : m_val(0) {}

    /// Construct timestamp with the given time internal representation
    /// @param val internal time presentation
    explicit TimeStamp(type val) : m_val(val) {}

    /// Construct timestamp from QDateTime object
    explicit TimeStamp(const QDateTime & datetime);

    ~TimeStamp() {}

    /// Returns a reference to the current timestamp native value
    /// @return Reference to the timestamp value
    type & value() { return m_val; }

    /// Returns the current timestamp native value
    /// @return Value of the timestamp
    type value() const { return m_val; }

    /// Sets the current native timestamp value
    /// @param val New value of the timestamp
    void setValue(type val) { m_val = val; }

    /// Sets the timestamp to s seconds.
    /// @param s number of seconds
    /// @tparam T Type of parameter for seconds
    template<typename T>
    void setSeconds(T s)
    {
      static_assert(std::is_arithmetic<T>::value, "TimeStamp::setSeconds only works with arithmetic types");
      m_val = static_cast<type>(s * FRACTIONS_PER_SECOND);
    }

    /// Create a TimeStamp with the given number of milliseconds
    /// @param s number of milliseconds
    /// @return specified timestamp
    /// @tparam T Type of parameter for milliseconds
    template<class T>
    static TimeStamp createMilliSeconds(T s, typename std::enable_if<std::is_integral<T>::value>::type* = 0)
    {
      type secs = s / 1000;
      type millis = s % 1000;
      return TimeStamp(FRACTIONS_PER_SECOND * secs + (millis * FRACTIONS_PER_SECOND) / 1000);
    }

    /// @copydoc createMilliSeconds
    template<class T>
    static TimeStamp createMilliSeconds(T s, typename std::enable_if<std::is_floating_point<T>::value>::type* = 0)
    {
      type integral = static_cast<type>(s);
      double fract = s - integral;
      return createMilliSeconds(integral) + TimeStamp((fract * FRACTIONS_PER_SECOND) / 1000);
    }

    /// Create a TimeStamp with the given number of seconds
    /// @param s number of seconds
    /// @return specified timestamp
    /// @tparam T Type of parameter for seconds
    template<class T>
    static TimeStamp createSeconds(T s, typename std::enable_if<std::is_integral<T>::value>::type* = 0)
    {
      return TimeStamp(static_cast<type>(s) * FRACTIONS_PER_SECOND);
    }

    /// @copydoc createSeconds
    template<class T>
    static TimeStamp createSeconds(T s, typename std::enable_if<std::is_floating_point<T>::value>::type* = 0)
    {
      type integral = static_cast<type>(s);
      double fract = s - integral;
      return TimeStamp((integral * FRACTIONS_PER_SECOND) + (fract * FRACTIONS_PER_SECOND));
    }

    /// Create a TimeStamp with the given number of minutes
    /// @param s number of minutes
    /// @return specified timestamp
    /// @tparam T Type of parameter for minutes
    template<typename T>
    static TimeStamp createMinutes(T s)
    {
      static_assert(std::is_arithmetic<T>::value, "TimeStamp::createMinutes only works with arithmetic types");
      return TimeStamp(static_cast<type>(s * ticksPerMinute().value()));
    }

    /// Create a time-stamp with a given number of hours.
    /// @param hours number of hours
    /// @return specified timestamp
    /// @tparam T Type of parameter for hours
    template<typename T>
    static TimeStamp createHours(T hours)
    {
      static_assert(std::is_arithmetic<T>::value, "TimeStamp::createHours only works with arithmetic types");
      return TimeStamp(static_cast<type>(hours * ticksPerHour().value()));
    }

    /// Create a time-stamp with a given number of days.
    /// @param days number of days
    /// @return specified timestamp
    /// @tparam T Type of parameter for days
    template<typename T>
    static TimeStamp createDays(T days)
    {
      static_assert(std::is_arithmetic<T>::value, "TimeStamp::createDays only works with arithmetic types");
      return TimeStamp(static_cast<type> (days * ticksPerDay().value()));
    }

    /// Creates a time-stamp consisting of days, hours, minutes, and seconds.
    /// @param days number of days
    /// @param hours number of hours
    /// @param minutes number of minutes
    /// @param seconds number of seconds
    /// @return new time-stamp
    static TimeStamp createDHMS(int days, int hours, int minutes, int seconds)
    {
      type tmp = ((type) seconds) << 24;
      tmp += ticksPerMinute().value() * (type) minutes;
      tmp += ticksPerHour().value() * (type) hours;
      return TimeStamp(tmp + ticksPerDay().value() * (type) days);
    }

    /// Creates a timestamp from date string
    /// @param date String for date
    /// @param delim Delimiter string between fields
    /// @param If true the date is formatted as YdMdD, where d is the value of
    ///        delim. If false the date has format DdMdY. Here each field can have
    ///        arbitrary integer.
    /// @return TimeStamp corresponding to given string
    static TimeStamp createDate(const char * date,
                const char * delim = "-",
                bool yearfirst = true);

    /// Creates a timestamp from time string
    /// @param time String for time formatted as HdMdS, where d stands for the value of
    ///        delim, H for hours, M for minutes and S for seconds. Each field can contain
    ///        arbitrary integer.
    /// @param delim Delimiter string between fields
    /// @return TimeStamp corresponding to given string
    static TimeStamp createTime(const char * time,
                const char * delim = ":");

    /// Creates a timestamp from date and time string
    /// @param date String for date
    /// @param delim Delimiter string between fields of date
    /// @param If true the date is formatted as YdMdD. Otherwise date has format DdMdY.
    /// @param time String for time formatted as HdMdS.
    /// @param timedelim Delimiter string between fields of time
    /// @return TimeStamp corresponding to given strings
    /// @sa createDate, createTime
    static TimeStamp createDateTime(const char * date,
                    const char * delim,
                    bool yearfirst,
                    const char * time,
                    const char * timedelim);

    /// Returns the number of complete days this timestamp spans
    /// @return Complete days from 1.1.1970
    int64_t days() const { return m_val / ticksPerDay().value(); }

    /// Returns the number of complete hours this timestamp spans
    /// @return Complete hours from 1.1.1970
    int64_t hours() const { return m_val / ticksPerHour().value(); }

    /// Returns the number of complete hours this timestamp spans
    /// @return Complete minutes from 1.1.1970
    int64_t minutes() const { return m_val / ticksPerMinute().value(); }

    /// Returns the number of days (including fractions of a day) this timestamp spans
    /// @return Days (including fractions) from 1.1.1970
    double  daysD() const { return m_val / (double) ticksPerDay().value(); }

    /// Returns the number of full seconds that this time-stamp includes
    /// @return Complete seconds from 1.1.1970
    int64_t seconds() const { return m_val >> 24; }

    /// Returns the number of full milliseconds that this time-stamp includes
    /// @return Complete milliseconds from 1.1.1970
    int64_t milliseconds() const
    {
      type sec = m_val / FRACTIONS_PER_SECOND;
      type fract = m_val % FRACTIONS_PER_SECOND;

      return (1000 * sec) + ((1000 * fract) / FRACTIONS_PER_SECOND);
    }

    /// Returns the fractions of second that this timestamp includes, in range 0-2^24
    /// @return Fraction of seconds part from the internal representation (24 lowest bits)
    int64_t fractions() const { return m_val & 0xFFFFFF; }

    /// Returns the number of seconds in this time-stamp, as floating point number
    /// @return Seconds (including fractions) from 1.1.1970
    double secondsD()  const { return m_val / (double) FRACTIONS_PER_SECOND; }

    /// Returns the fraction of a second in this time-stamp, in range 0.0-1.0
    /// @return The non-integral part of the seconds from 1.1.1970 normalized to unit interval
    double subSecondsD() const
    { return (m_val & 0xFFFFFF) / (double) FRACTIONS_PER_SECOND; }

    /// Returns the fraction part of a second in microseconds (in range 0.0-1000000.0)
    /// @return The non-integral part of the seconds from 1.1.1970 as milliseconds
    double subSecondsUS() const { return 1000000.0 * subSecondsD(); }

    /// Returns the number of seconds to the argument time-stamp.
    /// @return How many seconds that is ahead of this
    double secsTo(const TimeStamp & that) const
    { return (that.m_val - m_val) / (double) FRACTIONS_PER_SECOND; }

    /// Returns the number of micro-seconds to the argument time-stamp.
    /// @return How many microseconds that is ahead of this
    double usecsTo(const TimeStamp & that) const
    { return (that.m_val - m_val) * 1000000.0/(double) FRACTIONS_PER_SECOND; }

    /// Returns the amount of time passed since this timestamp.
    /// Computes the difference between @ref currentTime and this time-stamp
    /// @return amount of time passed since this time-stamp
    TimeStamp since() const { return TimeStamp(currentTime().m_val - this->m_val); }

    /// Returns the number of seconds passed since this timestamp.
    /// @return seconds passed since this time-stamp
    double sinceSecondsD() const { return since().secondsD(); }

    /// Returns the current time value, by looking at the wall clock
    /// @return current time
    static TimeStamp currentTime();

    /// Converts the time-stamp to a string
    /// @return time-stamp as string
    QString asString() const;

    /// Returns string representation in ISO8601 format:
    ///   YYYY-MM-DDTHH:mm:ss.SSSZ
    ///
    /// Time is always in UTC timezone and milliseconds are included
    QString asStringISO8601() const;

    /// Converts the time-stamp to QDateTime object, with Qt::UTC timespec
    QDateTime asQDateTime() const;

    /// @cond
    TimeStamp operator+(const TimeStamp & o) const { return TimeStamp(m_val + o.m_val); }
    TimeStamp operator-(const TimeStamp & o) const { return TimeStamp(m_val - o.m_val); }

    TimeStamp & operator+=(const TimeStamp & o) { m_val += o.m_val; return *this; }
    TimeStamp & operator-=(const TimeStamp & o) { m_val -= o.m_val; return *this; }

    bool operator<=(const TimeStamp & o) const { return m_val <= o.m_val; }
    bool operator>=(const TimeStamp & o) const { return m_val >= o.m_val; }

    bool operator<(const TimeStamp & o) const { return m_val < o.m_val; }
    bool operator>(const TimeStamp & o) const { return m_val > o.m_val; }

    bool operator==(const TimeStamp & o) const { return m_val == o.m_val; }
    bool operator!=(const TimeStamp & o) const { return m_val != o.m_val; }
    /// @endcond

  private:
    type m_val;
  };

  /// Output a timestamp to a stream
  /// @param os stream to write to
  /// @param ts timestamp to write
  /// @return reference to the output stream
  RADIANT_API std::ostream & operator<<(std::ostream & os, const TimeStamp & ts);

  /// Read a timestamp from a stream
  /// @param is stream to read from
  /// @param ts timestamp to read
  /// @return reference to the input stream
  RADIANT_API std::istream & operator>>(std::istream & is, TimeStamp & ts);

}

#endif

