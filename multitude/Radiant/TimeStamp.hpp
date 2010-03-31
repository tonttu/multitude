/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef RADIANT_TIMESTAMP_HPP
#define RADIANT_TIMESTAMP_HPP

#include <Radiant/Export.hpp>

#include <stdint.h>
#include <stdio.h>
#include <string>

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

  /// @todo remove suffix from create* functions, Documentation
  class RADIANT_API TimeStamp
  {
  public:

    /// The internal, signed 64-bit integer type
    typedef int64_t type;

    enum {
      FRACTIONS_PER_SECOND = 0x1000000
    };

    /// The number of ticks that take place during one second
    static type ticksPerSecond() { return FRACTIONS_PER_SECOND; }
    /// The number of ticks that take place during one minute
    static type ticksPerMinute() { return 1006632960; }
    /// The number of ticks that take place during one hour
    static type ticksPerHour()   { return 60397977600ll; }
    /// The number of ticks that take place during one day
    static type ticksPerDay()    { return 1449551462400ll; }


    TimeStamp(type val = 0) : m_val(val) {}
    ~TimeStamp() {}

    /// Returns a reference to the current timestamp native value
    type & value() { return m_val; }
    /// Returns a const reference to the current timestamp native value
    const type & value() const { return m_val; }

    /// Sets the timestamp to s seconds.
    void setSecondsD(double s)
    { m_val = (type) (s * FRACTIONS_PER_SECOND); }

    /// Sets the timestamp to s seconds.
    void setSecondsI(type s)
    { m_val = (s * FRACTIONS_PER_SECOND); }

    /// Create a time-stamp with a given number of seconds.
    static TimeStamp createSecondsD(double s)
    { return (type) (s * FRACTIONS_PER_SECOND); }

    /// Create a time-stamp with a given number of seconds.
    static TimeStamp createSecondsI(type s)
    { return (s << 24); }

    /// Create a time-stamp with a given number of hours.
    static TimeStamp createHoursD(double hours)
    { return (type) (hours * ticksPerHour()); }

    /// Create a time-stamp with a given number of days.
    static TimeStamp createDaysD(double days)
    { return (type) (days * ticksPerDay()); }

    /// Create a time-stamp with a given number of days.
    static TimeStamp createDaysI(type days)
    { return (type) (days * ticksPerDay()); }

    /** Creates a time-stamp consisting of days, hours, minutes, and seconds. */
    static TimeStamp createDHMS(int days, int hours, int minutes, int seconds)
    {
      type tmp = ((type) seconds) << 24;
      tmp += ticksPerMinute() * (type) minutes;
      tmp += ticksPerHour() * (type) hours;
      return tmp + ticksPerDay() * (type) days;
    }

    static TimeStamp createDate(const char * date,
                const char * delim = "-",
                bool yearfirst = true);

    static TimeStamp createTime(const char * time,
                const char * delim = ":");

    static TimeStamp createDateTime(const char * date,
                    const char * delim,
                    bool yearfirst,
                    const char * time,
                    const char * timedelim);

    /// Returns the number of complete days this timestamp spans
    int64_t days() const { return m_val / ticksPerDay(); }
    /// Returns the number of days (including fractions of a day) this timestamp spans
    double  daysD() const { return m_val / (double) ticksPerDay(); }
    /// Returns the number of full seconds that this time-stamp includes
    int64_t seconds() const { return m_val >> 24; }
    /// Returns the fractions of second that this timestamp includes.
    int64_t fractions() const { return m_val & 0xFFFFFF; }
    /// Returns the number of seconds in this time-stamp, as floating point number
    double secondsD()  const { return m_val / (double) FRACTIONS_PER_SECOND; }
    /// Returns the fractions of a second in this time-stamp, in range 0.0-1.0
    double subSecondsD() const
    { return (m_val & 0xFFFFFF) / (double) FRACTIONS_PER_SECOND; }
    /// Returns the fractions of a second in this time-stamp, in range 0-2^24
    int64_t subSecondsI() const
    { return m_val & 0xFFFFFF; }
    /// Returns the fractions of a second in this time-stamp, in range 0.0-1000000.0
    double subSecondsUS() const { return 1000000.0 * subSecondsD(); }

    /// Returns the number of seconds to the argument time-stamp.
    double secsTo(const TimeStamp & that) const
    { return (that.m_val - m_val) / (double) FRACTIONS_PER_SECOND; }
    /// Returns the number of micro-seconds to the argument time-stamp.
    double usecsTo(const TimeStamp & that) const
    { return (that.m_val - m_val) * 1000000.0/(double) FRACTIONS_PER_SECOND; }

    /** Returns the amount of time passed since this timestamp. */
    TimeStamp since() const { return getTime() - *this; }

    /** Returns the number of seconds passed since this timestamp. */
    double sinceSecondsD() const { return since().secondsD(); }

    /// Automatic cast operator that converts the time-stamp object to int64_t
    inline operator type & () { return m_val; }
    /// Automatic const-cast operator that converts the time-stamp object to int64_t
    inline operator const type & () const { return m_val; }

    /** Returns the current time value, by looking at the wall clock. */
    static type getTime();

    /** Converts the time-stamp to a string. */
    std::string asString() const;

  private:
    type m_val;
  };

}


#endif

