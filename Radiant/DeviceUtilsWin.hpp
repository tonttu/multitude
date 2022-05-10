#ifndef RADIANT_DEVICE_UTILS_WIN_HPP
#define RADIANT_DEVICE_UTILS_WIN_HPP

#include "Export.hpp"

#include <QByteArray>
#include <QMap>

#include <guiddef.h>

#include <vector>

#include <Windows.h>
#include <Setupapi.h>


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

    struct Device
    {
      QMap<QByteArray, QString> keys;
      std::vector<Device> children;
    };

    RADIANT_API DeviceInfo deviceInfo(const QString & deviceInstanceId,
                                      const GUID * deviceClassGuid);
    RADIANT_API DeviceInfo displayDeviceInfo(const QString & deviceInstanceId);
    RADIANT_API std::vector<int> cpuList(int numaNode);

    RADIANT_API QStringList busRelations(const QString & deviceId);
    RADIANT_API std::vector<Device> allDevices();
    RADIANT_API void dump();
    RADIANT_API void dump(const std::vector<Device>& devices);

    RADIANT_API QMap<QByteArray, QString> parseProperties(HDEVINFO& devinfo,
                                                          SP_DEVINFO_DATA& data);
  }
} // namespace Radiant

/// @endcond

#endif // RADIANT_DEVICE_UTILS_WIN_HPP
