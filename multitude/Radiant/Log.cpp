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

#include "Log.hpp"

#include "DateTime.hpp"
#include "Mutex.hpp"
#include "Sleep.hpp"
#include "Thread.hpp"
#include "TimeStamp.hpp"
#include "Trace.hpp"

#include <list>
#include <string>
#include <cassert>
#include <stdarg.h>

namespace Radiant {

  class LogThread : public Thread
  {
  public:
    LogThread() : m_file(0), m_ready(false) {}

    void add(const char * msg)
    {
      if(!m_file)
        return;
      Guard g( & m_mutex);
      m_messages.push_back(msg);
    }

    void setFile(FILE * f)
    {
      FILE * old = (FILE *) m_file;
      m_file = f;
      if(old)
        fclose(old);
    }

  protected:

    virtual void childLoop()
    {
      while(true) {
        m_ready = true;
        Sleep::sleepS(1);
        Guard g( & m_mutex);

        for(container::iterator it = m_messages.begin();
        it != m_messages.end(); it++) {

          if(m_file) {
            DateTime dt((*it).m_time);

            sprintf(m_buf, "%.2d/%.2d/%.4d,%.2d:%.2d:%.2d.%.3d",
                    dt.monthDay() + 1, dt.month() + 1, dt.year(),
                    dt.hour(), dt.minute(), dt.second(), dt.milliSecond());

            fprintf((FILE *) m_file, "%s,%s\n", m_buf, (*it).m_str.c_str());

            // info("LOG: %s", m_buf);
          }
        }

        m_messages.clear();
      }
    }

  private:

    class Item
    {
    public:
      Item(const char * str) : m_str(str), m_time(TimeStamp::getTime()) {}
      std::string m_str;
      TimeStamp   m_time;
    };
    typedef std::list<Item> container;
    container m_messages;

    MutexAuto m_mutex;

    volatile FILE * m_file;
    volatile bool   m_ready;
    char m_buf[4096];
  };

  // int kjh = 23;
  static LogThread * __logthread = 0;

  static void makeThread()
  {
    if(!__logthread) {
      __logthread = new LogThread();
      __logthread->run();
    }

  }

  bool Log::setLogFile(const char * logfile)
  {
    makeThread();
    FILE * file = fopen(logfile, "w");
    if(file)
      __logthread->setFile(file);

    return file != 0;
  }

  bool Log::setTimedLogFile(const char * appname)
  {
    makeThread();

    DateTime dt(TimeStamp::getTime());
    char buf[128], buf2[128];
    dt.print(buf2);
    sprintf(buf, "%s-%s-log.txt", appname, buf2);
    return setLogFile(buf);
  }

  /*
  void Log::log(const char * str)
  {
    assert(str != 0);

    makeThread();

    __logthread->add(str);
  }
  */

  void Log::log(const char * msg, ...)
  {
    assert(msg != 0);

    char buf[4096];
    va_list args;

    va_start(args, msg);
    vsnprintf(buf, 4096, msg, args);
    va_end(args);

    makeThread();

    __logthread->add(buf);
  }

}

