/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include <QMap>
#include <QPair>
#include <QDir>

#include "ScreenDetector.hpp"
#include <Radiant/Platform.hpp>

#include "ScreenDetectorQt.hpp"

#if !defined(RADIANT_OSX) && !defined(RADIANT_ARM64)
# include "ScreenDetectorAMD.hpp"
# include "ScreenDetectorNV.hpp"
#endif

#if defined (RADIANT_LINUX)
#include <X11/Xlib.h>
#endif

namespace Luminous
{
#ifdef RADIANT_WINDOWS
  QString getFriendlyNameFromTarget(LUID adapterId, UINT32 targetId) {
      DISPLAYCONFIG_TARGET_DEVICE_NAME deviceName;
      DISPLAYCONFIG_DEVICE_INFO_HEADER header;
      header.size = sizeof(DISPLAYCONFIG_TARGET_DEVICE_NAME);
      header.adapterId = adapterId;
      header.id = targetId;
      header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
      deviceName.header = header;
      DisplayConfigGetDeviceInfo( (DISPLAYCONFIG_DEVICE_INFO_HEADER*) &deviceName );
      return QString::fromWCharArray(deviceName.monitorFriendlyDeviceName);
  }

/*
    Gets GDI Device name from Source (e.g. \\.\DISPLAY4).
*/
QString getGDIDeviceNameFromSource(LUID adapterId, UINT32 sourceId) {
    DISPLAYCONFIG_SOURCE_DEVICE_NAME deviceName;
    DISPLAYCONFIG_DEVICE_INFO_HEADER header;
    header.size = sizeof(DISPLAYCONFIG_SOURCE_DEVICE_NAME);
    header.adapterId = adapterId;
    header.id = sourceId;
    header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    deviceName.header = header;
    DisplayConfigGetDeviceInfo( (DISPLAYCONFIG_DEVICE_INFO_HEADER*) &deviceName );
    return QString::fromWCharArray(deviceName.viewGdiDeviceName);
}


  QMap<QString, QString> queryMonitorNames()
  {
    QMap<QString, QString> monitor_name_map;
    UINT32 num_of_paths = 0;
    UINT32 num_of_modes = 0;
    DISPLAYCONFIG_PATH_INFO* displayPaths = NULL;
    DISPLAYCONFIG_MODE_INFO* displayModes = NULL;


    GetDisplayConfigBufferSizes(QDC_ALL_PATHS, &num_of_paths, &num_of_modes);


    // Allocate paths and modes dynamically
    displayPaths = (DISPLAYCONFIG_PATH_INFO*)calloc((int)num_of_paths, sizeof(DISPLAYCONFIG_PATH_INFO));
    displayModes = (DISPLAYCONFIG_MODE_INFO*)calloc((int)num_of_modes, sizeof(DISPLAYCONFIG_MODE_INFO));

    // Query for the information
    QueryDisplayConfig(QDC_ALL_PATHS, &num_of_paths, displayPaths, &num_of_modes, displayModes, NULL);

    for (int i = 0; i < num_of_paths; i++) {
       if(displayPaths[i].flags & DISPLAYCONFIG_PATH_ACTIVE)
       {
         LUID sluid = displayPaths[i].sourceInfo.adapterId;
         UINT32 sid = displayPaths[i].sourceInfo.id;
         LUID tluid = displayPaths[i].targetInfo.adapterId;
         UINT32 tid = displayPaths[i].targetInfo.id;

         QString GDIName = getGDIDeviceNameFromSource(sluid, sid);
         QString friendlyName = getFriendlyNameFromTarget(tluid, tid);
         monitor_name_map[GDIName] = friendlyName;
       }
    }

    free(displayPaths);
    free(displayModes);
    return monitor_name_map;
  }
#endif //RADIANT_WINDOWS

  void ScreenDetector::scan(bool /*forceRescan*/)
  {
    m_results.clear();

    // FIXME: Implement on arm64
#if defined(RADIANT_LINUX) && !defined(RADIANT_ARM64)
    const int screens = XScreenCount(X11Display());
    for(int screen = 0; screen < screens; ++screen) {
      if(ScreenDetectorNV::detect(screen, m_results)) continue;
      if(ScreenDetectorAMD::detect(screen, m_results)) continue;
    }
#elif RADIANT_WINDOWS
    ScreenDetectorNV::detect(0, m_results);
    ScreenDetectorAMD::detect(0, m_results);
#endif
    if (m_results.empty()) {
      ScreenDetectorQt::detect(m_results);
    }
  }


#ifdef RADIANT_WINDOWS
  QString ScreenDetector::monitorFriendlyNameFromGDIName(QString GDIName)
  {
    QMap<QString, QString> map = queryMonitorNames();

    QString monitor_name = map[GDIName];
    if(monitor_name.isEmpty())
    {
      DISPLAY_DEVICEA dd;
      memset(&dd, 0, sizeof(dd));
      dd.cb = sizeof(dd);
      EnumDisplayDevicesA(GDIName.toUtf8().data(), 0, &dd, 0);
      monitor_name = dd.DeviceString;
    }

    return monitor_name;
  }
#endif


#ifdef RADIANT_LINUX
  X11Display::X11Display(X11Display &&display)
    : m_display(std::move(display.m_display))
  {

  }

  X11Display::X11Display(bool detectDisplay)
    : m_display(nullptr)
  {
    if(!detectDisplay)
      return;

    open();
  }

  X11Display::X11Display(const QByteArray & displayName)
    : m_display(XOpenDisplay(displayName.data()))
  {}

  X11Display::~X11Display()
  {
    close();
  }

  bool X11Display::open(const QByteArray &displayName)
  {
    close();

    if(!displayName.isEmpty()) {
      m_display = XOpenDisplay(displayName.data());
      return m_display;
    } else {
      // First try using DISPLAY environment variable
      m_display = XOpenDisplay(nullptr);

      if(!m_display) {
        QDir socketsDir("/tmp/.X11-unix");
        QStringList files = socketsDir.entryList(QDir::System);
        QRegExp displayRegex = QRegExp("^X([0-9]).*");
        for(const QString &file : files) {
          if(displayRegex.exactMatch(file)) {
            QString displayName = QString(":%1").arg(displayRegex.cap(1));
            m_display = XOpenDisplay(displayName.toUtf8().data());
            if(m_display) {
              break;
            }
          }
        }
      }
    }

    return m_display != nullptr;
  }

  bool X11Display::close()
  {
    if(m_display) {
      XCloseDisplay(m_display);
      m_display = nullptr;
      return true;
    }

    return false;
  }

  X11Display::operator Display * ()
  {
    return m_display;
  }

  X11Display::operator const Display * () const
  {
    return m_display;
  }
#endif
}
