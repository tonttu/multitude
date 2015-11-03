/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "KeyEvent.hpp"

#include <QKeyEvent>
#include <QMouseEvent>

namespace Radiant
{
  class KeyEvent::D
  {
  public:
    D(QKeyEvent * qKeyEvent)
      : m_event(qKeyEvent)
    {}

    std::unique_ptr<QKeyEvent> m_event;
  };

  KeyEvent::KeyEvent(const QKeyEvent & e)
    : m_d(new D(QKeyEvent::createExtendedKeyEvent(e.type(), e.key(), e.modifiers(), e.nativeScanCode(),
                                                  e.nativeVirtualKey(), e.nativeModifiers(), e.text(),
                                                  e.isAutoRepeat(), e.count())))
  {
  }

  KeyEvent::KeyEvent(int key, QEvent::Type type, Qt::KeyboardModifiers modifiers, const QString & text, bool autorep)
    : m_d(new D(new QKeyEvent(type, key, modifiers, text, autorep)))
  {
  }

  KeyEvent::~KeyEvent()
  {
  }

  KeyEvent KeyEvent::createKeyPress(int key, bool isautorepeat)
  {
    return KeyEvent(key, QEvent::KeyPress, Qt::NoModifier, QString(), isautorepeat);
  }

  KeyEvent KeyEvent::createKeyRelease(int key)
  {
    return KeyEvent(key, QEvent::KeyRelease, Qt::NoModifier);
  }

  int KeyEvent::key() const
  {
    return m_d->m_event->key();
  }

  Qt::KeyboardModifiers KeyEvent::modifiers() const
  {
    return m_d->m_event->modifiers();
  }

  bool KeyEvent::isAutoRepeat() const
  {
    return m_d->m_event->isAutoRepeat();
  }

  QEvent::Type KeyEvent::type() const
  {
    return m_d->m_event->type();
  }

  QString KeyEvent::text() const
  {
    return m_d->m_event->text();
  }

  const QKeyEvent & KeyEvent::qKeyEvent() const
  {
    return *m_d->m_event;
  }

  //////////////////////////////////////////////////
  //////////////////////////////////////////////////

  class MouseEvent::D
  {
  public:
    QEvent::Type type;
    Nimble::Vector2f location;
    Qt::MouseButton button;
    Qt::MouseButtons buttons;
    Qt::KeyboardModifiers modifiers;
    int delta;
  };

  MouseEvent::MouseEvent(const QMouseEvent & event)
    : m_d(new D())
  {
    m_d->type = event.type();
    m_d->location = Nimble::Vector2f(event.posF().x(), event.posF().y());
    m_d->button = event.button();
    m_d->buttons = event.buttons();
    m_d->modifiers = event.modifiers();
    m_d->delta = 0;
  }

  MouseEvent::MouseEvent(const QWheelEvent & event)
    : m_d(new D())
  {
    m_d->type = event.type();
    m_d->location = Nimble::Vector2f(event.pos().x(), event.pos().y());
    m_d->button = Qt::NoButton;
    m_d->buttons = event.buttons();
    m_d->modifiers = event.modifiers();
    m_d->delta = event.delta();
  }

  MouseEvent::MouseEvent(QEvent::Type type, const Nimble::Vector2f & location, Qt::MouseButton button,
    Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
    : m_d(new D())
  {
    m_d->type = type;
    m_d->location = location;
    m_d->button = button;
    m_d->buttons = buttons;
    m_d->modifiers = modifiers;
    m_d->delta = 0;
  }

  MouseEvent::MouseEvent(const MouseEvent & e)
    : m_d(new D(*e.m_d))
  {
  }

  MouseEvent & MouseEvent::operator=(const MouseEvent & ev)
  {
    *m_d = *ev.m_d;
    return *this;
  }

  MouseEvent::~MouseEvent()
  {
  }

  Nimble::Vector2f MouseEvent::location() const
  {
    return m_d->location;
  }

  void MouseEvent::setLocation(const Nimble::Vector2f & location)
  {
    m_d->location = location;
  }

  int MouseEvent::delta() const
  {
    return m_d->delta;
  }

  QEvent::Type MouseEvent::type() const
  {
    return m_d->type;
  }

  void MouseEvent::setType(QEvent::Type type)
  {
    m_d->type = type;
  }

  Qt::MouseButton MouseEvent::button() const
  {
    return m_d->button;
  }

  Qt::MouseButtons MouseEvent::buttons() const
  {
    return m_d->buttons;
  }

  Qt::KeyboardModifiers MouseEvent::modifiers() const
  {
    return m_d->modifiers;
  }
}
