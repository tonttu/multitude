/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_CAMERA_DRIVER_HPP
#define RADIANT_CAMERA_DRIVER_HPP

#include "Export.hpp"
#include "StringUtils.hpp"
#include "VideoCamera.hpp"

#include <QString>
#include <map>
#include <vector>

#include <QStringList>

namespace Radiant
{

  /// Each camera driver should implement this interface in addition to VideoCamera interface.
  class RADIANT_API CameraDriver
  {
  public:
    CameraDriver();
    virtual ~CameraDriver();
    /// Get a list of available cameras on the system that this driver supports
    /// @param cameras array of cameras where the new cameras will be appended to
    /// @return number of cameras found by this driver
    virtual size_t queryCameras(std::vector<VideoCamera::CameraInfo> & cameras) = 0;
    /// Create a new camera object using this interface
    virtual VideoCamera * createCamera() = 0;
    /// Get the name of this driver
    /// @return name of the driver, e.g. "libdc"
    virtual QString driverName() const = 0;
  };

  /// CameraDriverFactory provides high-level access to different drivers
  class RADIANT_API CameraDriverFactory
  {
  public:
    CameraDriverFactory();
    ~CameraDriverFactory();

    /// Create a camera instance using the specified driver
    /// @param driver name of the driver to use
    /// @return the camera driver instance
    VideoCamera * createCamera(const QString & driver);
    /// Create a camera instance using the first matching driver. The registered drivers are iterated in the order they appear on the preferred drivers list.
    /// @return camera instance from the first matching driver
    VideoCamera * createPreferredCamera();

    /// Get the specified camera driver
    CameraDriver * getCameraDriver(const QString & driverName);
    /// Get the preferred camera driver
    CameraDriver * getPreferredCameraDriver();

    /// Register a new camera driver. The memory used by the driver is freed when the
    /// factory is destroyed.
    /// @param driver driver to register
    void registerDriver(CameraDriver * driver);

    /// Specify the preferred order of using drivers
    /// @param pref driver names separated by comma, e.g. "libdc,ptgrey,cmu"
    void setDriverPreference(const QString & pref);

  private:
    typedef std::map<QString, CameraDriver *> DriverMap;
    DriverMap m_drivers;

    QStringList m_preferredDrivers;
  };

}

#endif
