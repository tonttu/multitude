#ifndef KTC_DEVICEMONITOR_HPP
#define KTC_DEVICEMONITOR_HPP

#include "Export.hpp"
#include "Platform.hpp"

#ifdef RADIANT_LINUX

#include <QMap>
#include <QString>

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

  class DeviceMonitor {
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

