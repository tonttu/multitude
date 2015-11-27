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

#include <Valuable/XMLArchive.hpp>

#include <Patterns/NotCopyable.hpp>

#ifdef RADIANT_WINDOWS
#  include <windows.h>
#endif //RADIANT_WINDOW

#ifdef RADIANT_LINUX
typedef struct _XDisplay Display;
#endif

namespace Luminous
{
  class ScreenInfo
  {
  public:
    enum Rotation
    {
      ROTATE_0   = 0,
      ROTATE_90  = 90000,
      ROTATE_180 = 180000,
      ROTATE_270 = 270000
    };

    ScreenInfo() : m_logicalScreen(0), m_numid(-1), m_rotation(ROTATE_0) {}

    /// For example "GPU-0.DFP-3"
    QString id() const { return m_gpu + "." + m_connection; }

    /// Display name got from EDID
    const QString & name() const { return m_name; }
    void setName(const QString & name) { m_name = name; }

    /// For example "GPU-0" or "GPU-0,GPU-1"
    const QString & gpu() const { return m_gpu; }
    inline void setGpu(const QString & gpu);

    /// Display adapter name, for example "GeForce 9800 GT"
    const QString & gpuName() const { return m_gpuName; }
    void setGpuName(const QString & gpuName) { m_gpuName = gpuName; }

    /// For example "DFP-0"
    const QString & connection() const { return m_connection; }
    inline void setConnection(const QString & connection);

    /// With X11 this is the X screen number
    int logicalScreen() const { return m_logicalScreen; }
    void setLogicalScreen(int logicalScreen) { m_logicalScreen = logicalScreen; }

    /// Size and location relative to this logical screen
    const Nimble::Recti & geometry() const { return m_geometry; }
    void setGeometry(const Nimble::Recti & geometry) { m_geometry = geometry; }

    /// Screen resolution. With full hd displays, this will return 1920x1080 regardless of the rotation
    Nimble::Size resolution() const
    {
      auto s = geometry().size();
      if (m_rotation == ROTATE_90 || m_rotation == ROTATE_270)
        s.transpose();
      return s;
    }

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

    Rotation rotation() const { return m_rotation; }
    void setRotation(Rotation r) { m_rotation = r; }

    float rotationRadians() const { return m_rotation / 1000.0f / 180.0f * Nimble::Math::PI; }

    bool operator==(const ScreenInfo & s) const
    {
      return m_name == s.m_name && m_gpu == s.m_gpu && m_gpuName == s.m_gpuName &&
          m_connection == s.m_connection && m_logicalScreen == s.m_logicalScreen &&
          m_logicalScreen == s.m_logicalScreen && m_geometry == s.m_geometry &&
          m_numid == s.m_numid && m_rotation == s.m_rotation;
    }

  private:
    QString m_name;
    QString m_gpu;
    QString m_gpuName;
    QString m_connection;
    int m_logicalScreen;
    Nimble::Recti m_geometry;
    int m_numid;
    Rotation m_rotation;
  };

  class LUMINOUS_API ScreenDetector
  {
  public:
    void scan(bool forceRescan = false);
    inline const QList<ScreenInfo> & results() const { return m_results; }
#ifdef RADIANT_WINDOWS
    /* get the real monitor name (e.g. Prisma2 1080p) from GDI Device
     * name (e.g. \\.\DISPLAY1). This is needed because EnumDisplayDevices
     * doesn't always return the correct name, returning instead the dreaded
     * 'Generic PnP Monitor'
    */
    static  QString monitorFriendlyNameFromGDIName(QString GDIName);
#endif //RADIANT_WINDOWS
  private:
    QList<ScreenInfo> m_results;
  };

  void ScreenInfo::setGpu(const QString & gpu)
  {
    m_gpu = Valuable::XMLArchive::cleanElementName(gpu);
  }

  void ScreenInfo::setConnection(const QString & connection)
  {
    m_connection = Valuable::XMLArchive::cleanElementName(connection);
  }

#ifdef RADIANT_LINUX
  class LUMINOUS_API X11Display : public Patterns::NotCopyable
  {
  public:
    X11Display(X11Display && display);
    X11Display(bool detectDisplay = true);
    X11Display(const QByteArray & displayName);

    ~X11Display();

    /// @param displayName name of display to open. If not given, name is autodetected.
    /// @return true if a display was opened
    bool open(const QByteArray & displayName="");
    /// @return true if the display was closed
    bool close();
    operator Display * ();
    operator const Display * () const;

  private:
    Display * m_display;
  };
#endif

}

/// @endcond

#endif // LUMINOUS_SCREENDETECTOR_HPP
