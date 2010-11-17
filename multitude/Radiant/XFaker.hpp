/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
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
