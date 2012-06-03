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

#include "WindowConfig.hpp"

namespace Radiant
{
  WindowConfig::WindowConfig(int x, int y, int width, int height, bool fullscreen, bool frameless,
                             bool showCursor, int antiAliasing, const QString &display)
  {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
    this->fullscreen = fullscreen;
    this->frameless = frameless;
    this->display = display;
    this->showCursor = showCursor;
    this->m_antiAliasing = antiAliasing;
  }

}
