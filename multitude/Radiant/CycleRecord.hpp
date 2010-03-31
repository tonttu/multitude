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

#ifndef RADIANT_CYCLE_RECORD_HPP
#define RADIANT_CYCLE_RECORD_HPP

#include <Radiant/cycle.h>
#include <Radiant/Trace.hpp>

#include <vector>

namespace Radiant {

  /** CPU cycle record for performance analysis. */
  /** This class is used to store cpu cycle information. The CPU usage
      is accumulated in a series of buckets. The central idea is that
      you can insert analysis points in your software and monitor how
      the CPU time is used between them. 

      <PRE>
      
      CycleRecord record(3);

      while(run()) {
        record.getTicks();
	DoTask1();
	record.getNewTime(0);
	DoTask2();
	record.getNewTime(1);
	DoTask3();
	record.getNewTime(2);
      }

      record.finalReport();

      </PRE>
  */
  class CycleRecord
  {
  public:
    /** @param n The number of bins. */
    CycleRecord(unsigned n = 50)
    { 
      m_records.resize(n); 
      reset();
    }
    /// The number of bins
    unsigned size() const { return m_records.size(); }
    /// Reset the records to zero
    void reset() { for(unsigned i = 0; i < size(); i++) m_records[i] = 0.0; }
    /// Update the tick counter, without putting the value to any bucket
    void getTicks() { m_latest = getticks(); }
    /// Calculate the number of new ticks, and put the value to the given bucket
    void getNewTime(unsigned forWhich)
    {
      ticks now = getticks();
      m_records[forWhich] += elapsed(now, m_latest);
      m_latest = now;
    }

    /// Normalize the accumulation buffers, so that one can print the report
    void normalize()
    {
      unsigned i;
      double sum = 0.0;

      for(i = 0; i < m_records.size(); i++) 
	sum += m_records[i];
      
      for(i = 0; i < m_records.size() && sum != 0.0; i++)
	m_records[i] /= sum;
    }

    /// Print a CPU cycle report to the screen.
    void finalReport()
    {
      normalize();
      
      for(unsigned i = 0; i < m_records.size(); i++)
        printf("CPU cycles  %u   %.2lf\n", i, m_records[i] * 100.0);
    }

    /// The latest CPU cycle counter value
    ticks m_latest;
    /// The CPU record buckets
    std::vector<double> m_records;
  };

}

#endif

