#include "KeyEvent.hpp"

#include <QKeyEvent>
#include <QMouseEvent>

namespace Radiant
{
  class KeyEvent::D : public QKeyEvent
  {
  public:
    D(const QKeyEvent & qKeyEvent)
      : QKeyEvent(qKeyEvent)
    {}

    D(QEvent::Type type, int key, Qt::KeyboardModifiers modifiers, const QString& text = QString(),
              bool autorep = false, ushort count = 1)
      : QKeyEvent(type, key, modifiers, text, autorep, count)
    {}

    void setAutoRepeat(bool isAutoRepeat) { autor = isAutoRepeat; }
  };

  KeyEvent::KeyEvent(const QKeyEvent & qKeyEvent)
    : m_d(new D(qKeyEvent))
  {
  }

  KeyEvent::KeyEvent(int key, QEvent::Type type, Qt::KeyboardModifiers modifiers, const QString & text, bool autorep)
    : m_d(new D(type, key, modifiers, text, autorep))
  {
  }

  KeyEvent::~KeyEvent()
  {
    delete m_d;
  }

  bool KeyEvent::virtualEvent() const
  {
    /// @todo should be removed
    return false;
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
    return m_d->key();
  }

  Qt::KeyboardModifiers KeyEvent::modifiers() const
  {
    return m_d->modifiers();
  }

  bool KeyEvent::isAutoRepeat() const
  {
    return m_d->isAutoRepeat();
  }

  void KeyEvent::setAutoRepeat(bool isAutoRepeat)
  {
    m_d->setAutoRepeat(isAutoRepeat);
  }

  QEvent::Type KeyEvent::type() const
  {
    return m_d->type();
  }

  QString KeyEvent::text() const
  {
    return m_d->text();
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
  };

  MouseEvent::MouseEvent(const QMouseEvent & event)
    : m_d(new D({event.type(), Nimble::Vector2f(event.posF().x(), event.posF().y()),
                event.button(), event.buttons(), event.modifiers()}))
  {
  }

  MouseEvent::MouseEvent(QEvent::Type type, const Nimble::Vector2f & location, Qt::MouseButton button,
              Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
    : m_d(new D({type, location, button, buttons, modifiers}))
  {
  }

  MouseEvent::MouseEvent(const MouseEvent & ev)
    : m_d(new D(*ev.m_d))
  {
  }

  MouseEvent & MouseEvent::operator=(const MouseEvent & ev)
  {
    *m_d = *ev.m_d;
    return *this;
  }

  MouseEvent::~MouseEvent()
  {
    delete m_d;
  }

  Nimble::Vector2f MouseEvent::location() const
  {
    return m_d->location;
  }

  void MouseEvent::setLocation(const Nimble::Vector2f & location)
  {
    m_d->location = location;
  }

  QEvent::Type MouseEvent::type() const
  {
    return m_d->type;
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
