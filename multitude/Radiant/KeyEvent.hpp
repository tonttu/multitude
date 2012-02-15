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
#ifndef RADIANT_KEY_EVENT_HPP
#define RADIANT_KEY_EVENT_HPP

#include <QKeyEvent>

namespace Radiant
{
  class KeyEvent : public QKeyEvent
  {
  public:
    KeyEvent(const QKeyEvent & event, bool virtualEvent = false)
      : QKeyEvent(event)
      , m_virtual(virtualEvent) {}
    KeyEvent(int key,  Qt::KeyboardModifiers modifiers = Qt::NoModifier, const QString & text = "")
      : QKeyEvent(KeyPress, key, modifiers, text)
      , m_virtual(true) {}
    inline bool virtualEvent() const { return m_virtual; }

    static KeyEvent createKeyPress(int key)
    {
      return KeyEvent(QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier));
    }

    static KeyEvent createKeyRelease(int key)
    {
      return KeyEvent(QKeyEvent(QEvent::KeyRelease, key, Qt::NoModifier));
    }

  private:
    bool m_virtual;
  };
}

#endif
