/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_SCREENDETECTOR_HPP
#define LUMINOUS_SCREENDETECTOR_HPP

/// @cond

#include "Export.hpp"

#include <Nimble/Rect.hpp>

#include <QString>
#include <QList>
#include <Radiant/Trace.hpp>

#ifdef RADIANT_WINDOWS
  /* get the real monitor name (e.g. Prisma2 1080p) from GDI Device
   * name (e.g. \\.\DISPLAY1). This is needed because EnumDisplayDevices
   * doesn't always return the correct name, returning instead the dreaded
   * 'Generic PnP Monitor'
  */
#  include <windows.h>
#endif //RADIANT_WINDOW

namespace Luminous
{
#ifdef RADIANT_WINDOWS
  /* get the real monitor name (e.g. Prisma2 1080p) from GDI Device
   * name (e.g. \\.\DISPLAY1). This is needed because EnumDisplayDevices
   * doesn't always return the correct name, returning instead the dreaded
   * 'Generic PnP Monitor'
  */
#  include <windows.h>

extern LUMINOUS_API QString monitorFriendlyNameFromGDIName(QString GDIName);
#endif //RADIANT_WINDOWS

  class ScreenInfo
  {
  public:
    ScreenInfo() : m_logicalScreen(0),m_numid(-1) {}

    /// For example "GPU-0.DFP-3"
    QString id() const { return m_gpu + "." + m_connection; }

    /// Display name got from EDID
    const QString & name() const { return m_name; }
    void setName(const QString & name) { m_name = name; }

    /// For example "GPU-0" or "GPU-0,GPU-1"
    const QString & gpu() const { return m_gpu; }
    void setGpu(const QString & gpu) { m_gpu = gpu; }

    /// Display adapter name, for example "GeForce 9800 GT"
    const QString & gpuName() const { return m_gpuName; }
    void setGpuName(const QString & gpuName) { m_gpuName = gpuName; }

    /// For example "DFP-0"
    const QString & connection() const { return m_connection; }
    void setConnection(const QString & connection) { m_connection = connection; }

    /// With X11 this is the X screen number
    int logicalScreen() const { return m_logicalScreen; }
    void setLogicalScreen(int logicalScreen) { m_logicalScreen = logicalScreen; }

    /// Size and location relative to this logical screen
    const Nimble::Recti & geometry() const { return m_geometry; }
    void setGeometry(const Nimble::Recti & geometry) { m_geometry = geometry; }
    /// Unique Number that identifies the screen
    void setNumId(int nid) {
      m_numid = nid;
    }
    int numId() const {return m_numid;}
    QString displayGroup() const {return gpu()+"-"+QString("%1").arg(logicalScreen());}
    bool isMTDevice()
    {
      //TODO find a better way to do this
        return name().contains("MultiTouchVM1") || name().contains("Prisma2 1080p");
    }

    bool isTaction()
    {
      //TODO find a better way to do this
        return name().contains("MultiTouchVM1");
    }

  private:
    QString m_name;
    QString m_gpu;
    QString m_gpuName;
    QString m_connection;
    int m_logicalScreen;
    Nimble::Recti m_geometry;
    int m_numid;
  };

  class LUMINOUS_API ScreenDetector
  {
  public:
    void scan(bool forceRescan = false);
    inline const QList<ScreenInfo> & results() const { return m_results; }

  private:
    QList<ScreenInfo> m_results;
  };
}

/// @endcond

#endif // LUMINOUS_SCREENDETECTOR_HPP
