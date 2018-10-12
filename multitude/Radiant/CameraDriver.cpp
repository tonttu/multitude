/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "CameraDriver.hpp"
#include "Mutex.hpp"
#include "Radiant.hpp"

#ifdef RADIANT_IOS
# undef CAMERA_DRIVER_1394
#endif // RADIANT_IOS

#ifdef CAMERA_DRIVER_CMU
#	include <Radiant/VideoCameraCMU.hpp>
#endif

#ifdef CAMERA_DRIVER_1394
#	include <Radiant/VideoCamera1394.hpp>
#endif

#include <Radiant/Trace.hpp>

namespace Radiant
{

  CameraDriver::CameraDriver()
  {}

  CameraDriver::~CameraDriver()
  {}

  CameraDriverFactory::CameraDriverFactory()
  {}

  CameraDriverFactory::~CameraDriverFactory()
  {
    for(DriverMap::iterator it = m_drivers.begin(); it != m_drivers.end(); ++it)
      delete it->second;
  }

  VideoCamera * CameraDriverFactory::createCamera(const QString & driverName)
  {
    CameraDriver * driver = getCameraDriver(driverName);
    if(driver)
      return driver->createCamera();

    return 0;
  }

  VideoCamera * CameraDriverFactory::createPreferredCamera()
  {
    CameraDriver * cd = getPreferredCameraDriver();
    if(cd) return cd->createCamera();

    return 0;
  }

  CameraDriver * CameraDriverFactory::getCameraDriver(const QString & driverName)
  {
    // If the user has not registered any drivers, we register the defaults here once
    MULTI_ONCE {
      assert(m_drivers.empty());
#ifdef CAMERA_DRIVER_CMU
      registerDriver(new CameraDriverCMU());
#endif

#ifdef CAMERA_DRIVER_1394
      registerDriver(new CameraDriver1394());
#endif
    }

    DriverMap::iterator it = m_drivers.find(driverName);
    if(it != m_drivers.end())
      return it->second;

    return 0;
  }

  CameraDriver * CameraDriverFactory::getPreferredCameraDriver()
  {
    // If no preferences are set, use defaults
#ifdef WIN32
    if(m_preferredDrivers.empty())
      setDriverPreference("cmu");
#else
    if(m_preferredDrivers.empty())
      setDriverPreference("libdc,cmu");
#endif

    std::vector<VideoCamera::CameraInfo> cameras;

    Q_FOREACH(QString str, m_preferredDrivers) {

      CameraDriver * cd = getCameraDriver(str);

      debugRadiant("CameraDriverFactory::getPreferredCameraDriver # Checking driver %s = %p",
          str.toUtf8().data(), cd);
      if(cd) {
        // Make sure there is at least one camera available using this driver
          size_t cameraCount = cd->queryCameras(cameras);

        if(cameraCount > 0)
            return cd;
      }
    }

    return 0;
  }

  void CameraDriverFactory::registerDriver(CameraDriver * driver)
  {
    m_drivers.insert(std::make_pair(driver->driverName(), driver));
  }

  void CameraDriverFactory::setDriverPreference(const QString & pref)
  {
    m_preferredDrivers = pref.split(",", QString::SkipEmptyParts);
  }

}
