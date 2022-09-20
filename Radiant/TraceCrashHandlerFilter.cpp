/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "TraceCrashHandlerFilter.hpp"
#include "CrashHandler.hpp"
#include "TimeStamp.hpp"

#include <ctime>

namespace Radiant
{
  namespace Trace
  {
    static const char * s_prefixes[] = {
      "[DEBUG] ",
      "",
      "[WARNING] ",
      "[ERROR] ",
      "[FATAL] "
    };

    CrashHandlerFilter::CrashHandlerFilter()
      : Filter(ORDER_OUTPUT)
    {
      CrashHandler::setAttachmentBuffer("Application log", m_buffer);
    }

    CrashHandlerFilter::~CrashHandlerFilter()
    {
      CrashHandler::removeAttachment("Application log");
    }

    bool CrashHandlerFilter::trace(Message & msg)
    {
      const TimeStamp now = msg.timestamp();

      time_t t = now.value() >> 24;
      /// localtime is not thread-safe on unix, and localtime_r isn't defined in windows
      #ifdef RADIANT_WINDOWS
        struct tm * ts = localtime(&t);
      #else
        struct tm tmp;
        struct tm * ts = localtime_r(&t, &tmp);
      #endif

      char buffer[1024*8];
      size_t maxSize = std::min<size_t>(sizeof(buffer), m_buffer.maxDataSize());
      int len;

      if (msg.module.isEmpty()) {
        len = snprintf(buffer, maxSize, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] %s%s\n",
                       ts->tm_year+1900, ts->tm_mon+1, ts->tm_mday,
                       ts->tm_hour, ts->tm_min, ts->tm_sec,
                       int(now.subSecondsUS()) / 1000,
                       s_prefixes[msg.severity],
                       msg.text.toUtf8().data());
      } else {
        len = snprintf(buffer, maxSize, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] %s> %s%s\n",
                       ts->tm_year+1900, ts->tm_mon+1, ts->tm_mday,
                       ts->tm_hour, ts->tm_min, ts->tm_sec,
                       int(now.subSecondsUS()) / 1000,
                       msg.module.data(),
                       s_prefixes[msg.severity],
                       msg.text.toUtf8().data());
      }

      if (len <= 0)
        return false;

      m_buffer.write(buffer, std::min<size_t>(maxSize, static_cast<size_t>(len)));

      return false;
    }
  } // namespace Trace
} // namespace Radiant
