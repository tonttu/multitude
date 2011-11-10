/* COPYRIGHT
 */

#ifndef RADIANT_CYCLE_RECORD_HPP
#define RADIANT_CYCLE_RECORD_HPP

#include "Export.hpp"
#include "Trace.hpp"

#ifdef RADIANT_IOS
// Dummy implementation
typedef size_t ticks;
inline ticks getticks() { return 0; }
inline double elapsed(ticks, ticks) { return 0.0; }
#else
# include "cycle.h"
#endif

#include <stdio.h>
#include <vector>

#include <stdio.h>

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
    size_t size() const { return m_records.size(); }
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
    /// @return total CPU cycle count.
    double normalize()
    {
      unsigned i;
      double sum = 0.0;

      for(i = 0; i < m_records.size(); i++)
        sum += m_records[i];

      for(i = 0; i < m_records.size() && sum != 0.0; i++)
        m_records[i] /= sum;

      return sum;
    }

    /// Print a CPU cycle report to the screen.
    void finalReport()
    {
      normalize();

      for(unsigned i = 0; i < m_records.size(); i++)
        Radiant::info("CPU cycles  %u   %.2lf", i, m_records[i] * 100.0);
    }

    /// The latest CPU cycle counter value
    ticks m_latest;
    /// The CPU record buckets
    std::vector<double> m_records;
  };

}

#endif

