/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "CrashHandler.hpp"

namespace Radiant
{
  namespace CrashHandler
  {
    void init(const QString &, const QString &, const QString &) {}
    void setAnnotation(const QByteArray &, const QByteArray &) {}
    void removeAnnotation(const QByteArray &) {}
    void setAttachmentPtrImpl(const QByteArray &, void *, size_t) {}
    void removeAttachment(const QByteArray &) {}
    QString makeDump(bool) { return QString(); }
    void reloadSignalHandlers() {}
    QString defaultMinidumpPath() { return QString(); }
  }
}
