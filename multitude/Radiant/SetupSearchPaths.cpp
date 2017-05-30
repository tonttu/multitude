#include "PlatformUtils.hpp"

#include <QDir>

namespace
{
  struct SetupSeachPaths
  {
    SetupSeachPaths()
    {
      // Add user home directory and config directory
      QDir::addSearchPath("home", QDir::homePath());
      QDir::addSearchPath("user-config", Radiant::PlatformUtils::getModuleUserDataPath("MultiTouch", false));
    }
  };

  /// Configure the search paths during static initialization, so they are
  /// usable in main(). Additionally MultiTouch/MultiTouch.cpp defines
  /// "cornerstone" search path, see the comment in the file.
  static SetupSeachPaths setup;
}
