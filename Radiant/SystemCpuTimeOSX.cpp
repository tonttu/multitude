/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "SystemCpuTime.hpp"

namespace Radiant
{
  struct CpuTimes
  {
  };

  CpuTimes cpuTimes()
  {
    /// @todo not implemented
    return CpuTimes();
  }

  class SystemCpuTime::D
  {
  public:
    CpuTimes cpuTimes;
  };

  SystemCpuTime::SystemCpuTime()
    : m_d(new D())
  {
    reset();
  }

  SystemCpuTime::~SystemCpuTime()
  {
  }

  double SystemCpuTime::cpuLoad() const
  {
    /// @todo not implemented
    return 0;
  }

  void SystemCpuTime::reset()
  {
    m_d->cpuTimes = cpuTimes();
  }
} // namespace Radiant
