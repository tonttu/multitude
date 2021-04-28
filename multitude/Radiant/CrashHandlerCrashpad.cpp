#include "CrashHandler.hpp"
#include "TraceCrashHandlerFilter.hpp"

#include <Radiant/PlatformUtils.hpp>
#include <Radiant/Trace.hpp>

#include <Radiant/Version.hpp>

#include <client/crashpad_client.h>
#include <client/simulate_crash_win.h>
#include <client/crashpad_info.h>
#include <client/crash_report_database.h>
#include <client/settings.h>

#include <QApplication>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QStandardPaths>

#include <memory>
#include <set>

#ifndef RADIANT_WINDOWS
#include <dlfcn.h>
#endif

#define TOSTR2(x) #x
#define TOSTR(x) TOSTR2(x)

namespace crashpad
{
  inline bool operator<(const UUID & a, const UUID & b)
  {
    return memcmp(&a, &b, sizeof(UUID)) < 0;
  }
}

namespace
{
  // These are pointers and never released, otherwise we'll have issues with
  // deletion order on shutdown when static objects are being released
  crashpad::SimpleStringDictionary * s_simpleAnnotations = nullptr;
  crashpad::SimpleAddressRangeBag * s_extraMemoryRanges = nullptr;
  std::unique_ptr<crashpad::CrashReportDatabase> s_database;
  std::unique_ptr<crashpad::CrashpadClient> s_client;
  std::map<QByteArray, std::pair<void*, size_t>> * s_attachments = nullptr;
}

namespace Radiant
{
  namespace CrashHandler
  {
    namespace
    {
      using crashpad::CrashReportDatabase;

      bool logHandler(logging::LogSeverity severity,
                      const char * filePath,
                      int line,
                      size_t message_start,
                      const std::string & string)
      {
        Radiant::Trace::Severity traceSeverity;
        if (severity <= logging::LOG_VERBOSE ||
            string.find("DuplicateHandle hStd") != std::string::npos) {
          traceSeverity = Radiant::Trace::DEBUG;
        } else if (severity == logging::LOG_INFO) {
          traceSeverity = Radiant::Trace::INFO;
        } else if (severity == logging::LOG_WARNING) {
          traceSeverity = Radiant::Trace::WARNING;
        } else {
          traceSeverity = Radiant::Trace::FAILURE;
        }

        const char * file = filePath;
        for (const char * it = filePath; *it; ++it)
          if (*it == '/' || *it == '\\')
            file = it + 1;

        QByteArray msg = QByteArray::fromStdString(string).trimmed().
            mid(static_cast<int>(message_start));
        Radiant::Trace::trace("Crashpad", traceSeverity, "%s:%d # %s",
                              file, line, msg.data());
        return true;
      }

      void printError(const char * prefix, CrashReportDatabase::OperationStatus status)
      {
        switch (status) {
        case CrashReportDatabase::kNoError:
          break;

        case CrashReportDatabase::kReportNotFound:
          Radiant::error("%s: Report not found", prefix);
          break;

        case CrashReportDatabase::kFileSystemError:
          Radiant::error("%s: File system error", prefix);
          break;

        case CrashReportDatabase::kDatabaseError:
          Radiant::error("%s: DB error", prefix);
          break;

        case CrashReportDatabase::kBusyError:
          Radiant::debug("%s: The DB is locked", prefix);
          break;

        case CrashReportDatabase::kCannotRequestUpload:
          Radiant::error("%s: Can't request upload", prefix);
          break;
        }
      }

      void uploadPreviousCrashReport(CrashReportDatabase & db, std::vector<CrashReportDatabase::Report> & reports)
      {
        time_t latestTime = 0;
        CrashReportDatabase::Report * latestReport = nullptr;
        for (auto & r: reports) {
          if (r.creation_time > latestTime) {
            latestTime = r.creation_time;
            latestReport = &r;
          }
        }

        if (latestReport && !latestReport->uploaded) {
          time_t age = QDateTime::currentDateTime().toTime_t() - latestTime;
          if (age <= 60*60*24*7) {
            Radiant::info("Crashpad # Trying to upload older crash report %s",
                          latestReport->uuid.ToString().c_str());
            auto op = db.RequestUpload(latestReport->uuid);
            if (op != CrashReportDatabase::kNoError)
              printError("Failed to request crash dump uploading", op);
          }
        }
      }
    }

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
        test = QString(getenv("CORNERSTONE_DEPS_PATH")) + "/manual/crashpad/bin/crashpad_handler.exe";
      }
      if (!QFile::exists(test)) {
        test = QString("C:/%1/manual/crashpad/bin/crashpad_handler.exe").arg(TOSTR(MULTITACTION_DEPENDENCY_PATH));
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

    QString makeDump(bool uploadDump)
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
        settings->SetUploadsEnabled(uploadDump);
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

    void init(const QString & application,
              const QString & url, const QString & db)
    {
      if (s_client) {
        Radiant::error("Radiant::CrashHandler::init # Tried to reinitialize crash handler");
        return;
      }

      Trace::findOrCreateFilter<Trace::CrashHandlerFilter>();

      logging::SetLogMessageHandler(logHandler);

      const auto version = Radiant::cornerstoneVersionString();

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
      arguments.push_back("--no-identify-client-via-url");
      // The diagnostics server unfortunately doesn't accept gzip encoded data
      arguments.push_back("--no-upload-gzip");

      bool restartable = true;
      bool asynchronous_start = false;

      if (!s_client->StartHandler(handlerPath, dbPath, metricsPath, url.toStdString(),
                                  annotations, arguments, restartable, asynchronous_start))
        Radiant::error("Failed to initialize Crashpad handler. Crash reporting is disabled");

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
      // Create this dir manually, otherwise Crashpad will trigger a number of
      // error messages every time we access the DB.
      QDir().mkpath(db + "/attachments");

      crashpad::Settings * settings = s_database->GetSettings();
      settings->SetUploadsEnabled(true);

      int cleaned = s_database->CleanDatabase(60*30);
      if (cleaned > 0)
        Radiant::info("Crashpad # cleaned %d invalid records from the crash report DB", cleaned);

      std::vector<crashpad::CrashReportDatabase::Report> reports;
      crashpad::CrashReportDatabase::OperationStatus status =
          s_database->GetPendingReports(&reports);

      if (status == crashpad::CrashReportDatabase::kNoError) {
        if (reports.empty()) {
          s_database->GetCompletedReports(&reports);
          if (status == crashpad::CrashReportDatabase::kNoError) {
            // If there are no pending reports, check if the last crash report
            // weren't uploaded for some reason, and try again.
            uploadPreviousCrashReport(*s_database, reports);
          } else {
            printError("Crashpad # Failed to get completed crash reports", status);
          }
        }
      } else {
        printError("Crashpad # Failed to get pending crash reports", status);
      }
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
