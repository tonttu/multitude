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

#ifndef RADIANT_DATETIME_HPP
#define RADIANT_DATETIME_HPP

#include <Radiant/Export.hpp>
#include <Radiant/TimeStamp.hpp>

namespace Radiant {

  /** Combination of date and time information. */

  class RADIANT_API DateTime
  {
  public:

    enum DateFormat {
      /// Date and time in ISO format
      DATE_TIME_ISO,
      /// Just the date in ISO format
      DATE_ISO
    };

    DateTime();
    DateTime(const TimeStamp & );
    ~DateTime();

    /// Year (anno domini)
    int year() const { return m_year; }
    /// Month of year (January = 0)
    int month() const { return m_month; }
    /// Day of month (0-30)
    int monthDay() const { return m_monthDay; }
    /// Day of week (Sunday = 0)
    int weekDay() const { return m_weekDay; }
    /// Hour since midnight (0-23)
    int hour() const { return m_hour; }
    /// Minutes since last full hour (0-59)
    int minute() const { return m_minute; }
    /// Seconds since last full minute (0-59)
    int second() const { return m_second; }
    int milliSecond() const { return m_microsecond / 1000; }
    int microSecond() const { return m_microsecond; }
    /// Reset the hour, minute and second values to zero
    void clearTime();

    void setYear(int year) { m_year = year; }
    void setMonth(int month) { m_month = month; }
    void setMonthDay(int monthDay) { m_monthDay = monthDay; }
    void setWeekDay(int weekDay) { m_weekDay = weekDay; }
    void setHour(int hour) { m_hour = hour; }
    void setMinute(int minute) { m_minute = minute; }
    void setSecond(int second) { m_second = second; }

    void toNextYear();
    void toNextMonth();
    void toNextMonthDay();
    bool fromString(const std::string & s, DateFormat format = DATE_ISO);

    /** Returns the number of days in the month. This function does
    take the leap years into account, so the length of Febuary changes
    between 28 and 29 days, depending on the year.

    @todo Make the leap-year calculations take the loonger cycles
    into account.
    */
    static int daysInMonth(int month, int year);

    /** Returns the number of days in the current month. This method
    takes the leap-years into account. */
    int daysInMonth();

    TimeStamp asTimeStamp() const;

    /** Prints the date-time information to a string. */
    void print(char * buf, bool isotime = false);

  private:
    int m_year;
    int m_month;
    int m_monthDay;
    int m_weekDay;
    int m_hour;
    int m_minute;
    int m_second;
    int m_microsecond;
    bool m_summerTime;
  };
}

#endif
