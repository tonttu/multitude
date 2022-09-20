/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "CycleRecord.hpp"
#include "Trace.hpp"

namespace Radiant {

  CycleRecord::CycleRecord(unsigned n)
  {
    m_records.resize(n);
    reset();
  }

  size_t CycleRecord::size() const {
    return m_records.size();
  }

  void CycleRecord::reset() {
    for(unsigned i = 0; i < size(); i++)
      m_records[i] = 0.0;
  }

  void CycleRecord::getTicks() {
    m_latest = getticks();
  }

  void CycleRecord::getNewTime(unsigned forWhich)
  {
    ticks now = getticks();
    m_records[forWhich] += elapsed(now, m_latest);
    m_latest = now;
  }

  double CycleRecord::normalize()
  {
    unsigned i;
    double sum = 0.0;

    for(i = 0; i < m_records.size(); i++)
      sum += m_records[i];

    for(i = 0; i < m_records.size() && sum != 0.0; i++)
      m_records[i] /= sum;

    return sum;
  }

  void CycleRecord::finalReport()
  {
    normalize();

    for(unsigned i = 0; i < m_records.size(); i++)
      Radiant::info("CPU cycles  %u   %.2lf", i, m_records[i] * 100.0);
  }


}
