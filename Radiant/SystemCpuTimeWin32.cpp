#include "SystemCpuTime.hpp"
#include "Trace.hpp"
#include "StringUtils.hpp"

#include <Windows.h>

namespace Radiant
{
  struct CpuTimes
  {
    uint64_t idle = 0;
    uint64_t kernel = 0;
    uint64_t user = 0;
  };

  uint64_t convert(FILETIME t)
  {
    return (uint64_t(t.dwHighDateTime) << 32) | uint64_t(t.dwLowDateTime);
  }

  CpuTimes cpuTimes()
  {
    CpuTimes ret;
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
      ret.idle = convert(idleTime);
      ret.kernel = convert(kernelTime);
      ret.user = convert(userTime);
    } else {
      Radiant::warning("SystemCpuTime # GetSystemTimes failed: %s",
          StringUtils::getLastErrorMessage().toUtf8().data());
    }
    return ret;
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
    const CpuTimes now = cpuTimes();
    const double idle = now.idle - m_d->cpuTimes.idle;
    // idle time is included in kernel time
    const double total = (now.kernel + now.user) - (m_d->cpuTimes.kernel + m_d->cpuTimes.user);
    return total > 0 ? 1.0 - idle / total: 0;
  }

  void SystemCpuTime::reset()
  {
    m_d->cpuTimes = cpuTimes();
  }
} // namespace Radiant
