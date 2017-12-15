#include "CrashHandler.hpp"
#include "TraceCrashHandlerFilter.hpp"

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
#include <set>

#ifndef RADIANT_WINDOWS
#include <dlfcn.h>
#endif

namespace crashpad
{
  inline bool operator<(const UUID & a, const UUID & b)
  {
    return memcmp(&a, &b, sizeof(UUID)) < 0;
  }
}

namespace Radiant
{
  namespace CrashHandler
  {
    // These are pointers and never released, otherwise we'll have issues with
    // deletion order on shutdown when static objects are being released
    crashpad::SimpleStringDictionary * s_simpleAnnotations = nullptr;
    crashpad::SimpleAddressRangeBag * s_extraMemoryRanges = nullptr;
    std::unique_ptr<crashpad::CrashReportDatabase> s_database;
    std::unique_ptr<crashpad::CrashpadClient> s_client;
    std::map<QByteArray, std::pair<void*, size_t>> * s_attachments = nullptr;

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

    QString makeDump()
    {
      if (s_client) {
        std::set<crashpad::UUID> all;
        std::vector<crashpad::CrashReportDatabase::Report> tmp;
        s_database->GetPendingReports(&tmp);
        for (auto & r: tmp)
          all.insert(r.uuid);

        tmp.clear();
        s_database->GetCompletedReports(&tmp);
        for (auto & r: tmp)
          all.insert(r.uuid);

        crashpad::Settings * settings = s_database->GetSettings();
        settings->SetUploadsEnabled(false);
        CRASHPAD_SIMULATE_CRASH();
        settings->SetUploadsEnabled(true);

        tmp.clear();
        s_database->GetCompletedReports(&tmp);
        for (crashpad::CrashReportDatabase::Report & r: tmp) {
          // If this report wasn't in the database before simulating crash,
          // then it must be our new report
          if (all.count(r.uuid) == 0) {
            return QString::fromStdWString(r.file_path.value());
          }
        }
      }
      return QString();
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

      Trace::findOrCreateFilter<Trace::CrashHandlerFilter>();

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
        s_simpleAnnotations = new crashpad::SimpleStringDictionary();
      }
      info->set_simple_annotations(s_simpleAnnotations);

      if (!s_extraMemoryRanges) {
        s_extraMemoryRanges = new crashpad::SimpleAddressRangeBag();
      }
      info->set_extra_memory_ranges(s_extraMemoryRanges);

      s_database = crashpad::CrashReportDatabase::Initialize(dbPath);

      crashpad::Settings * settings = s_database->GetSettings();
      settings->SetUploadsEnabled(true);
    }

    void setAnnotation(const QByteArray & key, const QByteArray & value)
    {
      if (!s_simpleAnnotations) {
        s_simpleAnnotations = new crashpad::SimpleStringDictionary();
      }
      s_simpleAnnotations->SetKeyValue(key.data(), value.data());
    }

    void removeAnnotation(const QByteArray & key)
    {
      if (!s_simpleAnnotations)
        return;

      s_simpleAnnotations->RemoveKey(key.data());
    }

    void setAttachmentPtrImpl(const QByteArray & key, void * data, size_t len)
    {
      if (!s_extraMemoryRanges)
        s_extraMemoryRanges = new crashpad::SimpleAddressRangeBag();

      if (!s_attachments)
        s_attachments = new std::map<QByteArray, std::pair<void*, size_t>>();

      auto it = s_attachments->find(key);
      if (it != s_attachments->end()) {
        auto pair = it->second;
        s_extraMemoryRanges->Remove(pair.first, pair.second);
      }

      (*s_attachments)[key] = std::make_pair(data, len);
      if (len > 0)
        s_extraMemoryRanges->Insert(data, len);
    }

    void removeAttachment(const QByteArray & key)
    {
      if (!s_extraMemoryRanges || !s_attachments)
        return;

      auto it = s_attachments->find(key);
      if (it == s_attachments->end())
        return;

      auto pair = it->second;
      s_extraMemoryRanges->Remove(pair.first, pair.second);
      s_attachments->erase(it);
    }
  }
}
