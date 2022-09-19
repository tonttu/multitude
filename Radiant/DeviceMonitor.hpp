/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef KTC_DEVICEMONITOR_HPP
#define KTC_DEVICEMONITOR_HPP

#include "Export.hpp"
#include "Platform.hpp"

#if defined(RADIANT_LINUX) && !defined(RADIANT_MOBILE)

#include <QMap>
#include <QString>

#include <memory>
#include <vector>
#include <memory>

namespace Radiant
{
  /// In practice, this will be a udev_device.
  class RADIANT_API Device
  {
  public:
    /// This is probably a udev_device. An easy way to look at property
    /// names and values is to use "udevadm monitor --udev --property". That will
    /// print all device events and full properties.
    virtual QString getProperty(const QString &name) = 0;
    virtual ~Device() { }

    virtual QString path() const = 0;
  };
  typedef std::shared_ptr<Device> DevicePtr;
  typedef std::vector<DevicePtr> Devices;

  class RADIANT_API DeviceMonitor {
  public:
    virtual Devices findDevices(const QString &subsystem,
                                const QMap<QString, QString>& params) = 0;
    virtual ~DeviceMonitor() { }

    static QString decode(const QString & value);
  };

  RADIANT_API std::shared_ptr<DeviceMonitor> newUdevMonitor();
}

#endif

#endif // KTC_DEVICEMONITOR_HPP

