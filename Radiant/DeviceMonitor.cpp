/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "DeviceMonitor.hpp"

#if defined(RADIANT_LINUX) && !defined(RADIANT_MOBILE)

#include <Radiant/Platform.hpp>
#include <Nimble/Math.hpp>
#include <QRegExp>

#include <libudev.h>
#include <cassert>
#include <poll.h>

namespace Radiant
{
  class UdevDevice : public Device
  {
  public:
    UdevDevice(udev_device *device)
      : m_device(device) { }

    QString getProperty(const QString &name) OVERRIDE
    {
      if(!m_device) {
        return QString();
      }
      const char* value = udev_device_get_property_value(m_device, name.toUtf8().data());
      if(!value) {
        return QString();
      } else {
        return QString(value);
      }
    }

    QString path() const OVERRIDE
    {
      return udev_device_get_devnode(m_device);
    }

    ~UdevDevice() {
      if(m_device) {
        udev_device_unref(m_device);
        m_device = NULL;
      }
    }

  private:
    UdevDevice(const UdevDevice &other);
    void operator=(const UdevDevice &other);
    udev_device *m_device;
  };

  class UdevDeviceMonitor : public DeviceMonitor
  {
  public:
    UdevDeviceMonitor();
    Devices findDevices(const QString &subsystem, const QMap<QString, QString> &params) OVERRIDE;

    ~UdevDeviceMonitor();
  private:
    udev *m_udev;
  };

  UdevDeviceMonitor::UdevDeviceMonitor()
  {
    // TODO - replace these asserts with proper error handling
    m_udev = udev_new();
    assert(m_udev);
  }

  Devices UdevDeviceMonitor::findDevices(const QString &subsystem, const QMap<QString, QString> &params)
  {
    struct udev_enumerate * enumerate = udev_enumerate_new(m_udev);
    assert(enumerate);

    int err = udev_enumerate_add_match_subsystem(enumerate, subsystem.toUtf8().data());
    (void)err;
    assert(err == 0);

    err = udev_enumerate_scan_devices(enumerate);
    assert(err == 0);

    udev_list_entry * devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry * it = 0;

    Devices output;
    udev_list_entry_foreach(it, devices) {
      const char *syspath = udev_list_entry_get_name(it);
      udev_device *dev = udev_device_new_from_syspath(m_udev, syspath);
      if(dev) {
        std::shared_ptr<Device> device = std::make_shared<UdevDevice>(dev);

        QMap<QString, QString>::const_iterator it = params.constBegin();
        bool ok = true;
        while(it != params.constEnd()) {
          ok &= device->getProperty(it.key()) == it.value();
          ++it;
        }

        if(ok) {
          output.push_back(device);
        }
      }
    }

    udev_enumerate_unref(enumerate);

    return output;
  }

/*
 * Probably not needed to poll on devices. Just check at specific times. Worst case,
 * we can do 500ms sleep or something then try again, at least to begin with.
 *
bool UdevDeviceMonitor::haveDevice(const QString &subsystem,
                                   const std::function<bool (Device &)> &predicate,
                                   long int milliseconds)
{
  pollfd fds[1];
  fds[0].fd = udev_monitor_get_fd(m_monitor);
  fds[0].events = POLLIN | POLLPRI;
  fds[0].revents = 0;
  timeval timeout;
  timeout.tv_sec = milliseconds / 1000;  // stored in seconds
  timeout.tv_usec = (milliseconds % 1000) * 1000;  // stored in microseconds
  int ret = poll(fds, 1, timeout);
  if(ret > 0) {
    // have data, read device event
  } else if(ret == 0) {
    // timeout occured
  } else {
    // error occured, see errno
    m_logger.log("Error polling on udev device monitor socket",
                 LogSeverity::Error);
  }
}
*/

  UdevDeviceMonitor::~UdevDeviceMonitor()
  {
    udev_unref(m_udev);
  }

  std::shared_ptr<DeviceMonitor> newUdevMonitor()
  {
    return std::make_shared<UdevDeviceMonitor>();
  }

  QString DeviceMonitor::decode(const QString & value)
  {
    // udev encodes all 'potentially unsafe' chars to the corresponding
    // 2 char hex value prefixed by '\x'.
    //
    // Aka it does this (taken from libudev source, where i is read pos, j is write pos):
    /*
    if (str[i] == '\\' || !is_whitelisted(str[i], NULL)) {
      if (len-j < 4)
        goto err;
      sprintf(&str_enc[j], "\\x%02x", (unsigned char) str[i]);
      j += 4;
    }
    */
    // So we know that all the backslashes in the encoded string are actually
    // due to udev encoded stuff.
    QString output;
    QRegExp re("\\\\x([0-9a-fA-F]{2})");
    int index = 0;
    while(true) {
      int next = value.indexOf(re, index);
      if(next == -1) {
        output.append(QStringRef(&value, index, value.size() - index));
        break;
      } else {
        output.append(QStringRef(&value, index, next - index));

        bool ok = false;
        unsigned char c = static_cast<unsigned char>(re.cap(1).toInt(&ok, 16));
        assert(ok);
        output.append(c);

        index = next + re.cap(0).size();
      }
    }
    return output;
  }
}

#endif
