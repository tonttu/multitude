#include "CrashHandler.hpp"

namespace Radiant
{
  namespace CrashHandler
  {
    void init(const QString &, const QString &, const QString &, const QString &) {}
    void setAnnotation(const QByteArray &, const QByteArray &) {}
    void setAttachmentPtrImpl(const QByteArray &, void *, size_t) {}
    QString makeDump() { return QString(); }
    void reloadSignalHandlers() {}
    QString defaultMinidumpPath() { return QString(); }
  }
}
