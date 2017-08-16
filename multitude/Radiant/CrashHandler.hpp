#pragma once

#include "Export.hpp"

#include <QString>

namespace Radiant
{

  namespace CrashHandler
  {
    RADIANT_API void init(const QString & application, const QString & cornerstoneVersion,
                          const QString & url, const QString & db = QString());
    /// Add a single key-value -pair annotation to the crash report. Key and
    /// value can be max 255 characters long, and the crash report includes
    /// max 64 entries.
    RADIANT_API void setAnnotation(const QByteArray & key, const QByteArray & value);
    /// Create a minidump immediately without crashing
    RADIANT_API bool makeDump();
    RADIANT_API void reloadSignalHandlers();
    /// Get default path to store minidump files
    RADIANT_API QString defaultMinidumpPath();
  }
}
