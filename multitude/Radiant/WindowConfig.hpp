/* COPYRIGHT
 *
 * This file is part of ThreadedRendering.
 *
 * Copyright: MultiTouch Oy, Finland, http://multitouch.fi
 *
 * All rights reserved, 2007-2010
 *
 * You may use this file only for purposes for which you have a
 * specific, written permission from MultiTouch Oy.
 *
 * See file "ThreadedRendering.hpp" for authors and more details.
 *
 */

#ifndef WINDOWCONFIG_HPP
#define WINDOWCONFIG_HPP

#include "Export.hpp"

#include <QString>

namespace Radiant
{

  /// Window configuration information holder
  /// @todo remove this class, and use MultiHead::Window
  struct RADIANT_API WindowConfig
  {
  public:
    /// Constructs a WindowConfig object, with given values
    WindowConfig(int x, int y, int width, int height,
                 bool fullscreen, bool frameless, bool showCursor, int antiAliasing,
                 const QString & display = ":0.0");
    ///@cond
    // Window position and size
    int x, y, width, height;
    ///@endcond

    /// Use fullscreen?
    bool fullscreen;
    /// Frameless?
    bool frameless;
    /// Show cursor?
    bool showCursor;
    /// Controls anti-aliasing
    int m_antiAliasing;
    /// Iconify (minimize) on startup?
    bool iconify;
    /// Which display (X screen) to put the window on
    QString display;

  };

}

#endif
