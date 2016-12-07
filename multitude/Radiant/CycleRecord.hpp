/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_CYCLE_RECORD_HPP
#define RADIANT_CYCLE_RECORD_HPP

#include "Export.hpp"
#include <cstddef>

// FIXME: Implement on arm64
#if defined(RADIANT_IOS) || defined(RADIANT_ARM64)
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
  class RADIANT_API CycleRecord
  {
  public:
    /// Constructor
    /// @param n The number of bins.
    CycleRecord(unsigned n = 50);

    /// The number of bins
    /// @return Number of bins
    size_t size() const;

    /// Reset the records to zero
    void reset();

    /// Update the tick counter, without putting the value to any bucket
    void getTicks();

    /// Calculate the number of new ticks, and put the value to the given bucket
    /// @param forWich Bin to modify
    void getNewTime(unsigned forWhich);

    /// Normalize the accumulation buffers, so that one can print the report
    /// @return total CPU cycle count.
    double normalize();

    /// Print a CPU cycle report to the screen.
    void finalReport();

  private:
    /// The latest CPU cycle counter value
    ticks m_latest;
    /// The CPU record buckets
    std::vector<double> m_records;
  };

}

#endif

