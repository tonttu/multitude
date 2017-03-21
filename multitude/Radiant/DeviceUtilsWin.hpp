#ifndef RADIANT_DEVICE_UTILS_WIN_HPP
#define RADIANT_DEVICE_UTILS_WIN_HPP

#include "Export.hpp"

#include <QByteArray>

#include <guiddef.h>

#include <vector>

/// @cond

namespace Radiant
{
  /// Utilities for accessing Windows Unified Device Property Model
  namespace DeviceUtils
  {
    struct DeviceInfo
    {
      int bus = -1;
      int link = -1;
      int speed = -1;
      int numaNode = -1;
    };

    RADIANT_API DeviceInfo deviceInfo(const QString & deviceInstanceId,
                                      const GUID * deviceClassGuid);
    RADIANT_API DeviceInfo displayDeviceInfo(const QString & deviceInstanceId);
    RADIANT_API std::vector<int> cpuList(int numaNode);
  }
} // namespace Radiant

/// @endcond

#endif // RADIANT_DEVICE_UTILS_WIN_HPP
