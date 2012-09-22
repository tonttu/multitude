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

#ifndef LUMINOUS_WINDOWEVENTHOOK_HPP
#define LUMINOUS_WINDOWEVENTHOOK_HPP

namespace Radiant
{
  class DropEvent;
  class KeyEvent;
  class MouseEvent;
  class TabletEvent;
}

namespace Luminous
{

  /// Class for getting window events
  class WindowEventHook
  {
  public:
    virtual ~WindowEventHook() {}

    /// Different mouse buttons
    enum MouseButtonMask {
      NoButton = 0,
      LeftButton = 1,
      RightButton = 2,
      MiddleButton = 4,
      WheelForward = 8,
      WheelBackward = 16
    };

    /// Callback to handle keyboard events
    virtual void handleKeyboardEvent(const Radiant::KeyEvent & event) = 0;
    /// Callback to handle mouse events
    virtual void handleMouseEvent(const Radiant::MouseEvent & event) = 0;
    /// Callback to handle drop events
    virtual void handleDropEvent(const Radiant::DropEvent & event) = 0;
    /// Used to handle tablet events
    virtual void handleTabletEvent(const Radiant::TabletEvent & event) = 0;
    /// Handle resize events
    virtual void handleWindowMove(int x, int y, int width, int height) = 0;
    /// Time since last keyboard or mouse activity
    virtual double lastActivity() const = 0;
  };

}

#endif // WINDOWEVENTHOOK_HPP
