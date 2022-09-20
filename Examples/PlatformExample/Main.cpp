/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */


#include <Radiant/PlatformUtils.hpp>
#include <Radiant/Trace.hpp>

int main()
{
  Radiant::info("Application path = %s",
    Radiant::PlatformUtils::getExecutablePath().toUtf8().data());
  Radiant::info("Application system-wide resource dir = %s",
		Radiant::PlatformUtils::getModuleGlobalDataPath
    ("MultiTude/PlatformTest", true).toUtf8().data());
  Radiant::info("Application user-specific resource dir = %s",
		Radiant::PlatformUtils::getModuleUserDataPath
    ("MultiTude/PlatformTest", true).toUtf8().data());
  Radiant::info("Application memory footprint = %lld", (long long)
		Radiant::PlatformUtils::processMemoryUsage());
}
