/* COPYRIGHT
 *
 * This file is part of MultiWidgets.
 *
 * Copyright: MultiTouch Oy, Finland, http://multitou.ch
 *
 * All rights reserved, 2007-2009
 *
 * You may use this file only for purposes for which you have a
 * specific, written permission from MultiTouch Oy.
 *
 * See file "MultiWidgets.hpp" for authors and more details.
 *
 */

#ifndef XFAKER_HPP
#define XFAKER_HPP

namespace Radiant {

  /** Fabricate fake mouse events to the X11 Windowing environment. One day
      we might implement this class also for other platforms.

  */
  class XFaker
  {
  public:
    /// Constructs XFaker for the given display
    XFaker(const char* displayName = 0);
    ~XFaker();

    /// Fakes mouse move event to given coordinates 
    void fakeMouseMove(int x, int y);
    /// Fakes mouse button press
    void fakeMouseButton(int button, bool press);
    /// Fakes mouse wheel event
    void fakeMouseWheel(int dx, int dy);

  private:

    class D;
    D * m_d;

  };

}

#endif
