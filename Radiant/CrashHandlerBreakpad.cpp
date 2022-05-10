#include "CrashHandler.hpp"
#include "TraceCrashHandlerFilter.hpp"

#include <Radiant/PlatformUtils.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/Version.hpp>

#include <client/linux/handler/exception_handler.h>

#include <QDir>
#include <QMap>

#include <arpa/inet.h>

namespace Radiant
{
  namespace CrashHandler
  {
    static google_breakpad::ExceptionHandler * s_exceptionHandler = nullptr;

    // These are pointers and never released, otherwise we'll have issues with
    // deletion order on shutdown when static objects are being released
    static QMap<QByteArray, QByteArray> * s_annotations = nullptr;
    static std::map<QByteArray, std::pair<void*, size_t>> * s_attachments = nullptr;

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

    // This value is set to SIMULATING_CRASH when we are simulating crash using makeDump.
    // This is not just a boolean so that in case of memory corruption during actual crash
    // we won't read invalid value here.
    static uint64_t s_simulatingCrash = 0;
    static const uint64_t SIMULATING_CRASH = 0x49f4a35d0bad52a8ll;

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
        s_uploadCmd[0] = "/opt/multitaction-breakpad/bin/minidump_upload";
        s_uploadCmd[1] = "-p";
        s_uploadCmd[3] = "-v";
        s_uploadCmd[6] = url.toUtf8();
      }
    }

    static bool crashCallback(const google_breakpad::MinidumpDescriptor & descriptor, void *, bool succeeded)
    {
      if (s_simulatingCrash == SIMULATING_CRASH) {
        // We are not really crashing, do nothing here
        return true;
      }

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

    static google_breakpad::ExceptionHandler * createExceptionHandler(
        const std::string & path)
    {
      if (s_annotations)
        s_extraData = encodeExtraData(*s_annotations);

      auto p = new google_breakpad::ExceptionHandler(
            google_breakpad::MinidumpDescriptor(path), nullptr, crashCallback, nullptr, true, -1);
      p->RegisterAppMemory(s_extraData.data(), s_extraData.size());

      if (s_attachments) {
        for (auto & pair: *s_attachments) {
          auto data = pair.second;
          p->RegisterAppMemory(data.first, data.second);
        }
      }

      return p;
    }

    QString makeDump(bool uploadDump)
    {
      if (s_exceptionHandler) {
        s_simulatingCrash = uploadDump ? 0 : SIMULATING_CRASH;
        bool ok = s_exceptionHandler->WriteMinidump();
        s_simulatingCrash = 0;
        if (ok)
          return s_exceptionHandler->minidump_descriptor().path();
      }
      return QString();
    }

    void reloadSignalHandlers()
    {
      if (s_exceptionHandler) {
        const std::string path = s_exceptionHandler->minidump_descriptor().directory();
        // Deletion order is important with ExceptionHandler, first delete the
        // old one and then create the new one. Otherwise the first one will
        // disable signal handlers on its destructor.
        delete s_exceptionHandler;
        s_exceptionHandler = createExceptionHandler(path);
      }
    }

    void init(const QString & application,
              const QString & url, const QString & db)
    {
      Trace::findOrCreateFilter<Trace::CrashHandlerFilter>();

      const auto version = Radiant::cornerstoneVersionString();

      s_uploadCmd[2] = application.toUtf8();
      s_uploadCmd[4] = version.toUtf8();

      if (!s_annotations)
        s_annotations = new QMap<QByteArray, QByteArray>();

      (*s_annotations)["prod"] = application.toUtf8();
      (*s_annotations)["ver"] = version.toUtf8();
      (*s_annotations)["hostname"] = hostname();

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
      if (!s_annotations)
        s_annotations = new QMap<QByteArray, QByteArray>();

      if (s_annotations->contains(key) && (*s_annotations)[key] == value)
        return;

      (*s_annotations)[key] = value;
      if (s_exceptionHandler) {
        s_exceptionHandler->UnregisterAppMemory(s_extraData.data());
        s_extraData = encodeExtraData(*s_annotations);
        s_exceptionHandler->RegisterAppMemory(s_extraData.data(), s_extraData.size());
      }
    }

    void removeAnnotation(const QByteArray & key)
    {
      if (!s_annotations)
        return;

      auto it = s_annotations->find(key);
      if (it == s_annotations->end())
        return;

      s_annotations->erase(it);
      if (s_exceptionHandler) {
        s_exceptionHandler->UnregisterAppMemory(s_extraData.data());
        s_extraData = encodeExtraData(*s_annotations);
        s_exceptionHandler->RegisterAppMemory(s_extraData.data(), s_extraData.size());
      }
    }

    void setAttachmentPtrImpl(const QByteArray & key, void * data, size_t len)
    {
      if (!s_attachments)
        s_attachments = new std::map<QByteArray, std::pair<void*, size_t>>();

      auto it = s_attachments->find(key);
      if (it != s_attachments->end() && s_exceptionHandler) {
        s_exceptionHandler->UnregisterAppMemory(it->second.first);
      }

      (*s_attachments)[key] = std::make_pair(data, len);
      if (s_exceptionHandler && len > 0) {
        s_exceptionHandler->RegisterAppMemory(data, len);
      }
    }

    void removeAttachment(const QByteArray & key)
    {
      if (!s_attachments)
        return;

      auto it = s_attachments->find(key);
      if (it == s_attachments->end())
        return;

      if (s_exceptionHandler) {
        s_exceptionHandler->UnregisterAppMemory(it->second.first);
      }

      s_attachments->erase(it);
    }

    QString defaultMinidumpPath()
    {
      return QString("~/cornerstone-crash-dumps");
    }

  }
}
