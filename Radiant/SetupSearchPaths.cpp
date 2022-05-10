#include "PlatformUtils.hpp"
#include "Trace.hpp"
#include "Radiant.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

namespace
{
  struct SetupSeachPaths
  {
    SetupSeachPaths()
    {
      // Add user home directory and config directory
      QDir::addSearchPath("home", QDir::homePath());
      QDir::addSearchPath("user-config", Radiant::PlatformUtils::getModuleUserDataPath("MultiTouch", false));
#ifdef RADIANT_WINDOWS
      QDir::addSearchPath("system-config", Radiant::PlatformUtils::windowsProgramDataPath() + "\\MultiTaction");
#else
      QDir::addSearchPath("system-config", "/etc/MultiTaction");
#endif

#ifdef RADIANT_WINDOWS
      // Setup Qt plugin path to ensure SQL (and other) Qt plugins are found (#15243)
      const auto qtPluginPath = QFileInfo(QString("%1/../qt/plugins").arg(Radiant::PlatformUtils::getExecutablePath()));

      if(qtPluginPath.exists() && qtPluginPath.isDir()) {
        QCoreApplication::addLibraryPath(qtPluginPath.absoluteFilePath());

        debugRadiant("Searching Qt plugins from %s", qtPluginPath.absoluteFilePath().toUtf8().data());
      } else {
        debugRadiant("Qt plugin folder %s does not exist.", qtPluginPath.absoluteFilePath().toUtf8().data());
      }
#endif
    }
  };

  /// Configure the search paths during static initialization, so they are
  /// usable in main(). Additionally MultiTouch/MultiTouch.cpp defines
  /// "cornerstone" search path, see the comment in the file.
  static SetupSeachPaths setup;
}
