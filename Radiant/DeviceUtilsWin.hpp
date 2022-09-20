/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

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
