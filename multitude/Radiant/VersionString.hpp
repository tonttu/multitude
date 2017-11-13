#pragma once

#include "Export.hpp"

#include <QString>

namespace Radiant
{

  /// Wraps a string that is treated as a version number (1.23.45-foobar etc)
  /// and is sorted right ("1.2.10" > "1.2.9", "1 < y")
  /// @todo this is still wrong: "1.2.3" < "1.2.3-rc1"
  ///       "rc" should have a special meaning compared to any other string, like
  ///       "1.2.3" < "1.2.3-update1" or "1.2.3" < "1.2.3-halloween-mega-edition"
  class RADIANT_API VersionString
  {
  public:
    VersionString(const QString & str = "");

    bool operator<(const VersionString & other) const;
    bool operator<=(const VersionString & other) const;
    bool operator>(const VersionString & other) const;
    bool operator>=(const VersionString & other) const;
    bool operator==(const VersionString & other) const;
    bool operator!=(const VersionString & other) const;
    const QString& str() const;

  private:
    QString m_str;
  };

}
