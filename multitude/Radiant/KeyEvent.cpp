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

  KeyEvent::KeyEvent(int key, QEvent::Type type, Qt::KeyboardModifiers modifiers, const QString & text)
    : m_d(new D(type, key, modifiers, text))
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

  KeyEvent KeyEvent::createKeyPress(int key)
  {
    return KeyEvent(key, QEvent::KeyPress, Qt::NoModifier);
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

  class MouseEvent::D : public QMouseEvent
  {
  public:
    D(const QMouseEvent & qMouseEvent)
      : QMouseEvent(qMouseEvent)
    {}

    D(QEvent::Type type, const QPoint & pos,
                    Qt::MouseButton button, Qt::MouseButtons buttons,
                    Qt::KeyboardModifiers modifiers)
      : QMouseEvent(type, pos, button, buttons, modifiers)
    {}
  };

  MouseEvent::MouseEvent(const QMouseEvent & event)
    : m_d(new D(event))
  {
  }

  MouseEvent::MouseEvent(QEvent::Type type, const Nimble::Vector2i & pos, Qt::MouseButton button,
              Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
    : m_d(new D(type, QPoint(pos.x, pos.y), button, buttons, modifiers))
  {
  }

  MouseEvent::~MouseEvent()
  {
    delete m_d;
  }

  int MouseEvent::x() const
  {
    return m_d->x();
  }

  int MouseEvent::y() const
  {
    return m_d->y();
  }

  QEvent::Type MouseEvent::type() const
  {
    return m_d->type();
  }

  Qt::MouseButton MouseEvent::button() const
  {
    return m_d->button();
  }

  Qt::MouseButtons MouseEvent::buttons() const
  {
    return m_d->buttons();
  }

  Qt::KeyboardModifiers MouseEvent::modifiers() const
  {
    return m_d->modifiers();
  }
}
