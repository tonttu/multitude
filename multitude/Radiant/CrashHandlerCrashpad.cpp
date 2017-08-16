#include "CrashHandler.hpp"

#include <Radiant/PlatformUtils.hpp>
#include <Radiant/Trace.hpp>

#include <MultiTouch/Version.hpp>

#include <client/crashpad_client.h>
#include <client/simulate_crash_win.h>
#include <client/crashpad_info.h>
#include <client/crash_report_database.h>
#include <client/settings.h>

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

#include <memory>

#ifndef RADIANT_WINDOWS
#include <dlfcn.h>
#endif

namespace Radiant
{
  namespace CrashHandler
  {
    std::unique_ptr<crashpad::SimpleStringDictionary> s_simpleAnnotations;
    std::unique_ptr<crashpad::CrashReportDatabase> s_database;
    std::unique_ptr<crashpad::CrashpadClient> s_client;

#ifdef RADIANT_WINDOWS
    static QString libraryPath()
    {
      return QFileInfo(Radiant::PlatformUtils::getLibraryPath(
                         "Radiant." CORNERSTONE_SHORT_VERSION_STR
  #ifdef RADIANT_DEBUG
                         "_d.dll"
  #else
                         ".dll"
  #endif
                         )).absolutePath();
    }

    QString crashpadHandler()
    {
      QString lib = libraryPath();
      QString test = lib + "/../bin/crashpad_handler.exe";
      if (!QFile::exists(test) && getenv("CORNERSTONE_DEPS_PATH")) {
        test = QString(getenv("CORNERSTONE_DEPS_PATH")) + "/crashpad/bin/crashpad_handler.exe";
      }
      if (!QFile::exists(test)) {
        test = "C:/Cornerstone-" CORNERSTONE_SHORT_VERSION_STR "-deps/crashpad/bin/crashpad_handler.exe";
      }
      if (!QFile::exists(test)) {
        test = "crashpad_handler.exe";
      }
      if (QFile::exists(test)) {
        return QFileInfo(test).absoluteFilePath();
      }
      return "crashpad_handler.exe";
    }
#endif

#ifdef RADIANT_UNIX
    static QString libraryPath()
    {
      Dl_info info;
      if (dladdr((const void*)Radiant::CrashHandler::init, &info)) {
        return QFileInfo(info.dli_fname).absolutePath();
      }
      return QString();
    }
#endif

#ifdef RADIANT_LINUX
    QString crashpadHandler()
    {
      QString lib = libraryPath();
      QString test = lib + "/../bin/crashpad_handler";
      if (QFile::exists(test)) {
        return QFileInfo(test).absoluteFilePath();
      }
      return "crashpad_handler";
    }
#endif

#ifdef RADIANT_OSX
    QString crashpadHandler()
    {
      return "crashpad_handler";
    }
#endif

    QString defaultMinidumpPath()
    {
      // Change the name temporarily, since you can't give it as a parameter to QStandardPaths
      auto oldName = QApplication::applicationName();
      QApplication::setApplicationName("MultiTaction");
      QString path = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
      QApplication::setApplicationName(oldName);

      if (path.isEmpty()) {
        path = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
      }

      if (!QFileInfo::exists(path)) {
        QDir().mkpath(path);
      }

      return path + "/CrashDumps";
    }

    bool makeDump()
    {
      if (s_client) {
        CRASHPAD_SIMULATE_CRASH();
        return true;
      }
      return false;
    }

    void reloadSignalHandlers()
    {
    }

    void init(const QString & application, const QString & version,
              const QString & url, const QString & db)
    {
      if (s_client) {
        Radiant::error("Radiant::CrashHandler::init # Tried to reinitialize crash handler");
        return;
      }

      base::FilePath handlerPath(crashpadHandler().toStdWString());
      base::FilePath dbPath(db.toStdWString());
      base::FilePath metricsPath;

      s_client.reset(new crashpad::CrashpadClient());

      std::map<std::string, std::string> annotations;
      annotations["prod"] = application.toStdString();
      annotations["ver"] = version.toStdString();
      annotations["hostname"] = QSysInfo::machineHostName().toStdString();

      std::vector<std::string> arguments;
      arguments.push_back("--no-rate-limit");

      bool restartable = true;
      bool asynchronous_start = false;

      s_client->StartHandler(handlerPath, dbPath, metricsPath, url.toStdString(),
                             annotations, arguments, restartable, asynchronous_start);

      crashpad::CrashpadInfo * info = crashpad::CrashpadInfo::GetCrashpadInfo();
      info->set_gather_indirectly_referenced_memory(crashpad::TriState::kDisabled, 0);

      // Forward the crash to the system's crash reporter in debug mode
#ifdef RADIANT_DEBUG
      info->set_system_crash_reporter_forwarding(crashpad::TriState::kEnabled);
#else
      info->set_system_crash_reporter_forwarding(crashpad::TriState::kDisabled);
#endif

      if (!s_simpleAnnotations) {
        s_simpleAnnotations.reset(new crashpad::SimpleStringDictionary());
      }
      info->set_simple_annotations(s_simpleAnnotations.get());

      s_database = crashpad::CrashReportDatabase::Initialize(dbPath);

      crashpad::Settings * settings = s_database->GetSettings();
      settings->SetUploadsEnabled(true);
    }

    void setAnnotation(const QByteArray & key, const QByteArray & value)
    {
      if (!s_simpleAnnotations) {
        s_simpleAnnotations.reset(new crashpad::SimpleStringDictionary());
      }
      s_simpleAnnotations->SetKeyValue(key.data(), value.data());
    }

  }
}
