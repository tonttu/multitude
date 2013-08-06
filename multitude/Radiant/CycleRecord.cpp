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
