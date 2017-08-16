#include "CrashHandler.hpp"

#include <Radiant/PlatformUtils.hpp>
#include <Radiant/Trace.hpp>

#include <client/linux/handler/exception_handler.h>

#include <QDir>
#include <QMap>
#include <QSysInfo>

#include <arpa/inet.h>

namespace Radiant
{
  namespace CrashHandler
  {
    static std::unique_ptr<google_breakpad::ExceptionHandler> s_exceptionHandler;
    static QMap<QByteArray, QByteArray> s_annotations;

    // minidump_upload -p application -v version minidump url nullptr
    static QByteArray s_uploadCmd[7];

    // Since breakpad for Linux doesn't support extra fields, we encode our own
    // field to arbitrary memory region dump. This region will start with magic
    // (s_extraDataMagic) and has a list of binary key-value data blocks:
    //   uint32_t keyLength; // big endian
    //   uint32_t valueLength; // big endian
    //   char key[keyLength];
    //   char value[valueLength];

    static QByteArray s_extraData;
    static const char * s_extraDataMagic = "\xb0\x2d\x68\xa6";

    static QByteArray extraDataFieldHeader(uint32_t keyLength, uint32_t valueLength)
    {
      struct Hdr
      {
        uint32_t keyLength;
        uint32_t valueLength;
      } __attribute__((packed));

      Hdr hdr;
      hdr.keyLength = ntohl(keyLength);
      hdr.valueLength = ntohl(valueLength);

      return QByteArray(reinterpret_cast<const char*>(&hdr), sizeof(Hdr));
    }

    static QByteArray hostname()
    {
      char buffer[256];
      gethostname(buffer, sizeof(buffer));
      buffer[sizeof(buffer) - 1] = '\0';
      return buffer;
    }

    static QByteArray encodeExtraData(const QMap<QByteArray, QByteArray> & map)
    {
      QByteArray buffer;
      buffer += s_extraDataMagic;
      for (auto it = map.begin(); it != map.end(); ++it) {
        buffer += extraDataFieldHeader(it.key().size(), it.value().size());
        buffer += it.key();
        buffer += it.value();
      }
      return buffer;
    }

    static void setMinidumpUrl(const QString & url)
    {
      if (url.isEmpty()) {
        s_uploadCmd[0] = nullptr;
      } else {
        s_uploadCmd[0] = "minidump_upload";
        s_uploadCmd[1] = "-p";
        s_uploadCmd[3] = "-v";
        s_uploadCmd[6] = url.toUtf8();
      }
    }

    static bool crashCallback(const google_breakpad::MinidumpDescriptor & descriptor, void *, bool succeeded)
    {
      Radiant::error("CRASHING - Wrote minidump to %s", descriptor.path());

      if (s_uploadCmd[0].isEmpty()) {
        return succeeded;
      }

      Radiant::error("Uploading minidump to %s", s_uploadCmd[6].data());
      char * cmd[8];
      for (int i = 0; i < 7; ++i) {
        cmd[i] = s_uploadCmd[i].data();
      }
      cmd[5] = const_cast<char*>(descriptor.path());
      cmd[7] = nullptr;

      // Make sure we won't upload this again if we crash again
      s_uploadCmd[0].clear();

      pid_t pid = fork();
      if (pid == -1) {
        Radiant::error("Failed to fork, sending the minidump in this process");
        execvp(cmd[0], cmd);
      } else if (pid == 0) {
        setsid();
        execvp(cmd[0], cmd);
      }
      return succeeded;
    }

    static std::unique_ptr<google_breakpad::ExceptionHandler> createExceptionHandler(
        const std::string & path)
    {
      s_extraData = encodeExtraData(s_annotations);

      auto p = new google_breakpad::ExceptionHandler(
            google_breakpad::MinidumpDescriptor(path), nullptr, crashCallback, nullptr, true, -1);
      p->RegisterAppMemory(s_extraData.data(), s_extraData.size());
      return std::unique_ptr<google_breakpad::ExceptionHandler>(p);
    }

    bool makeDump()
    {
      if (s_exceptionHandler) {
        return s_exceptionHandler->WriteMinidump();
      }
      return false;
    }

    void reloadSignalHandlers()
    {
      if (s_exceptionHandler) {
        const std::string path = s_exceptionHandler->minidump_descriptor().directory();
        // Deletion order is important with ExceptionHandler, first delete the
        // old one and then create the new one. Otherwise the first one will
        // disable signal handlers on its destructor.
        s_exceptionHandler.reset();
        s_exceptionHandler = createExceptionHandler(path);
      }
    }

    void init(const QString & application, const QString & version,
              const QString & url, const QString & db)
    {
      s_uploadCmd[2] = application.toUtf8();
      s_uploadCmd[4] = version.toUtf8();

      s_annotations["prod"] = application.toUtf8();
      s_annotations["ver"] = version.toUtf8();
      s_annotations["hostname"] = hostname();

      QString tmp = db;
      tmp.replace("~/", Radiant::PlatformUtils::getUserHomePath() + "/");
      QDir().mkpath(tmp);
      if (s_exceptionHandler) {
        google_breakpad::MinidumpDescriptor desc(tmp.toStdString());
        s_exceptionHandler->set_minidump_descriptor(desc);
      } else {
        s_exceptionHandler = createExceptionHandler(tmp.toStdString());
      }

      setMinidumpUrl(url);
    }

    void setAnnotation(const QByteArray & key, const QByteArray & value)
    {
      if (s_annotations.contains(key) && s_annotations[key] == value)
        return;

      s_annotations[key] = value;
      reloadSignalHandlers();
    }

    QString defaultMinidumpPath()
    {
      return QString("~/cornerstone-crash-dumps");
    }

  }
}
