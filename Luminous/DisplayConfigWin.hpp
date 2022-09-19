/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#ifndef LUMINOUS_DISPLAY_CONFIG_WIN_HPP
#define LUMINOUS_DISPLAY_CONFIG_WIN_HPP

#include "Export.hpp"

#include <Windows.h>

#include <QString>
#include <QRect>

#include <cstring>
#include <vector>

/// @cond

namespace Luminous
{
  /// Class for detecting display configuration on Windows. This is written
  /// so that it is easy to implement also functions to modify the current
  /// configuration.
  /// @todo Should use this as one backend for ScreenDetector
  class LUMINOUS_API DisplayConfigWin
  {
  public:
    struct AdapterId
    {
      AdapterId() {}

      AdapterId(LUID luid)
        : m_luid(luid)
      {}

      bool operator==(AdapterId o) const { return m_value == o.m_value; }
      bool operator<(AdapterId o) const { return m_value < o.m_value; }

      LUID luid() const { return m_luid; }

    private:
      static_assert(sizeof(LUID) == sizeof(uint64_t), "LUID size check");
      union {
        uint64_t m_value = 0;
        LUID m_luid;
      };
    };

    struct Target
    {
      DISPLAYCONFIG_PATH_TARGET_INFO info;
      QString devicePath;
      QString friendlyDeviceName;
    };

    struct Output
    {
      Output()
      {
        std::memset(&preferredTargetMode, 0, sizeof(preferredTargetMode));
      }

      QRect rect;

      AdapterId adapterId;
      uint32_t id = 0;

      QString adapterDevicePath;
      QString sourceGdiDeviceName;

      bool active = false;

      Target activeTarget;

      QSize preferredTargetResolution;
      DISPLAYCONFIG_TARGET_MODE preferredTargetMode;
      std::vector<Target> targets;
    };

  public:
    static QString gdiDeviceToId(const QString & sourceGdiDeviceName);
    static QString cleanInstanceId(const QString & adapterDevicePath);

    /// @throws std::runtime_error
    static DisplayConfigWin currentConfig();

    Output & findOrCreate(AdapterId adapterId, uint32_t id);
    const Output * find(const QString & gdiId) const;

  public:
    std::vector<Output> outputs;
  };

} // namespace Luminous

/// @endcond

#endif // LUMINOUS_DISPLAY_CONFIG_WIN_HPP
