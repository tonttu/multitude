#ifndef RADIANT_SYSTEM_CPU_TIME_HPP
#define RADIANT_SYSTEM_CPU_TIME_HPP

#include "Export.hpp"

#include <memory>

namespace Radiant
{
  /// Class that can be used to monitor system-wide CPU load
  /// @note This class is not implemented on OS X.
  class RADIANT_API SystemCpuTime
  {
  public:
    SystemCpuTime();
    ~SystemCpuTime();

    /// Returns CPU load from 0 (CPUs have been idle) to 1 (CPUs have been in full use)
    /// since the class was constructed or reset() called
    double cpuLoad() const;

    /// Resets counters, next call to cpuLoad will be relative to this moment
    void reset();

  private:
    class D;
    std::unique_ptr<D> m_d;
  };


} // namespace Radiant

#endif // RADIANT_SYSTEM_CPU_TIME_HPP
