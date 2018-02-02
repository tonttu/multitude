#pragma once

#include "Export.hpp"
#include "VersionGenerated.hpp"

#include <QString>

namespace Radiant
{

  /// Version string type
  enum VersionType {
    /// Full version string, including build number, time and platform
    FULL,
    /// Only version number, without build info
    VERSION_ONLY,
    /// Major version number only
    VERSION_MAJOR,
    /// Minor version number only
    VERSION_MINOR,
    /// Patch version number only
    VERSION_PATCH,
    /// Git hash
    VERSION_GIT_HASH,
    /// CI build number, if applicable
    VERSION_BUILD_NUMBER
  };

  /// Get the Cornerstone version string
  /// @param type version string type to get
  /// @return Cornerstone version of requested type
  RADIANT_API QString cornerstoneVersionString(VersionType type = FULL);

}
