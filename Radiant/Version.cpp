/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "Version.hpp"
#include "Trace.hpp"

namespace Radiant
{

#define VERSION_STR_(...) #__VA_ARGS__
#define VERSION_STR(s) VERSION_STR_(s)
  QString cornerstoneVersionString(VersionType type)
  {
    switch(type) {
      case FULL:
      {
        const QString product = "Cornerstone " CORNERSTONE_FULL_VERSION_STR;
        QString hash = cornerstoneVersionString(VERSION_GIT_HASH);
        if (!hash.isEmpty()) {
          hash = " [" + hash + "]";
        }

        const QString platform = QString(" on %1 %2, %3 %4").arg(
              QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture(),
              QSysInfo::kernelType(), QSysInfo::kernelVersion());

#ifdef CORNERSTONE_BUILD_NUMBER
        const QString build = " (build number " VERSION_STR(CORNERSTONE_BUILD_NUMBER) ", " __DATE__ " " __TIME__ ")";
#else
        const QString build = " (built " __DATE__ " " __TIME__ ")";
#endif

        return product + hash + platform + build;
      }
        break;
      case VERSION_ONLY:
        return CORNERSTONE_FULL_VERSION_STR;
        break;
      case VERSION_MAJOR:
        return QString::number(CORNERSTONE_VERSION_MAJOR);
        break;
      case VERSION_MINOR:
        return QString::number(CORNERSTONE_VERSION_MINOR);
        break;
      case VERSION_PATCH:
        return QString::number(CORNERSTONE_VERSION_PATCH);
        break;
      case VERSION_GIT_HASH:
        return QString(CORNERSTONE_GIT_HASH);
        break;
      case VERSION_BUILD_NUMBER:
#ifdef CORNERSTONE_BUILD_NUMBER
        return "" VERSION_STR(CORNERSTONE_BUILD_NUMBER);
#else
        return QString();
#endif
        break;
      default:
        Radiant::error("Application::versionString # unknown version type requested");
        return QString();
        break;
    }
  }

}
