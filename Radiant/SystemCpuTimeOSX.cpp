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
