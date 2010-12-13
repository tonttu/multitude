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

#ifndef WINDOWEVENTHOOK_HPP
#define WINDOWEVENTHOOK_HPP

namespace Posh
{

  /// Class for getting window events
  class WindowEventHook
  {
  public:

    /// Different mouse buttons
    enum MouseButtonMask {
      NoButton = 0,
      LeftButton = 1,
      RightButton = 2,
      MiddleButton = 4,
      WheelForward = 8,
      WheelBackward = 16
    };

    /// Special keys
    enum SpecialKeycode
    {
      ARROW_UP    = 0x01000,
      ARROW_DOWN,
      ARROW_LEFT,
      ARROW_RIGHT,
      SPACE,
      ESCAPE,

      MOD_SHIFT = 0x1000,
      MOD_CONTROL = 0x2000,
      MOD_ALT = 0x4000
    };

    /// Callback to handle keyboard events
    virtual void handleKeyboardEvent(int keyCode, bool keyDown) = 0;
    /// Callback to handle mouse move events
    virtual void handleMouseMove(int mouseX, int mouseY, MouseButtonMask buttonMask) = 0;
    ///  Callback to handle mouse button events
    virtual void handleMouseButton(MouseButtonMask buttonMask, int mouseX, int mouseY, bool buttonPressed) = 0;
  };

}

#endif // WINDOWEVENTHOOK_HPP
