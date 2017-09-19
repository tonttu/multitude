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
    : m_d(new D(new QKeyEvent(e.type(), e.key(), e.modifiers(), e.nativeScanCode(),
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
    Nimble::Vector2f widgetLocation{0, 0};
    Qt::MouseButton button;
    Qt::MouseButtons buttons;
    Qt::KeyboardModifiers modifiers;
    int delta;
    bool synthesized;
  };

  MouseEvent::MouseEvent(const QMouseEvent & event)
    : m_d(new D())
  {
    m_d->type = event.type();
    m_d->location = Nimble::Vector2f(event.pos().x(), event.pos().y());
    m_d->button = event.button();
    m_d->buttons = event.buttons();
    m_d->modifiers = event.modifiers();
    m_d->delta = 0;
    m_d->synthesized = event.source() != Qt::MouseEventNotSynthesized;
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
    m_d->synthesized = event.source() != Qt::MouseEventNotSynthesized;
  }

  MouseEvent::MouseEvent(QEvent::Type type, const Nimble::Vector2f & location, Qt::MouseButton button,
    Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, bool isSynthesized)
    : m_d(new D())
  {
    m_d->type = type;
    m_d->location = location;
    m_d->button = button;
    m_d->buttons = buttons;
    m_d->modifiers = modifiers;
    m_d->delta = 0;
    m_d->synthesized = isSynthesized;
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

  Nimble::Vector2f MouseEvent::widgetLocation() const
  {
    return m_d->widgetLocation;
  }

  void MouseEvent::setWidgetLocation(const Nimble::Vector2f & widgetLocation)
  {
    m_d->widgetLocation = widgetLocation;
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

  QString MouseEvent::toString() const
  {
    QString out;
    switch (m_d->type) {
    case QEvent::MouseButtonPress:
      out += "MouseButtonPress";
      break;
    case QEvent::MouseButtonRelease:
      out += "MouseButtonRelease";
      break;
    case QEvent::MouseButtonDblClick:
      out += "MouseButtonDblClick";
      break;
    case QEvent::MouseMove:
      out += "MouseMove";
      break;
    case QEvent::Wheel:
      out += "Wheel";
      break;
    default:
      out += QString("Unknown event [%1]").arg(m_d->type);
    }

    out += QString(" location: [%1, %2]").arg(m_d->location.x).arg(m_d->location.y);
    out += QString(", button: %1").arg(m_d->button, 0, 16);
    out += QString(", buttons: %1").arg(m_d->buttons, 0, 16);
    out += QString(", modifiers: %1").arg(m_d->modifiers, 0, 16);
    out += QString(", delta: %1").arg(m_d->delta);
    return out;
  }

  bool MouseEvent::isSynthesized() const
  {
    return m_d->synthesized;
  }

  void MouseEvent::setSynthesized(bool s)
  {
    m_d->synthesized = s;
  }
}
