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

#include <Radiant/Export.hpp>
#include "Defines.hpp"

#include <Nimble/Vector2.hpp>

#include <QEvent>
#include <QWheelEvent>
#include <QString>

class QKeyEvent;
class QMouseEvent;

namespace Radiant
{

  /// This class describes a key event. Key events are sent from virtual
  /// keyboard or actual keyboard.
  class RADIANT_API KeyEvent
  {
  public:
    /// Constructs a new key event from a QKeyEvent
    /// @param event Qt key event
    KeyEvent(const QKeyEvent & event);

    /// Constructs a new key event
    /// @param key key code of the event (from Qt::Key)
    /// @param type type of the event. Must be KeyPress or KeyRelease.
    /// @param modifiers active keyboard modifiers for the event
    /// @param text Unicode representation of the text generated by the event
    /// @param autoRepeat is the event generated by keyboard auto-repeat
    KeyEvent(int key, QEvent::Type type = QEvent::KeyPress,
             Qt::KeyboardModifiers modifiers = Qt::NoModifier, const QString & text = "",
             bool autoRepeat = false);

    virtual ~KeyEvent();

    /// @cond
    // Not currently used, always returns false
    bool virtualEvent() const;
    /// @endcond

    /// Key code of the event (from Qt::Key)
    /// @return key code
    int key() const;

    /// Active keyboard modifiers
    /// @return bit field of active modifiers
    Qt::KeyboardModifiers modifiers() const;

    /// Returns boolean indicating a repeating event
    /// @return true if this is a repeating event
    bool isAutoRepeat() const;

    /// Sets the repeating flag of the event
    /// @param isAutoRepeat true if the event is a repeating event
    void setAutoRepeat(bool isAutoRepeat);

    /// Returns the type of the keyboard event
    /// @return type of the event
    QEvent::Type type() const;

    /// Returns the unicode text representation of the key event
    QString text() const;

    /// Creates a key press KeyEvent object with the specified key code
    /// @param key key code of the event
    /// @param isautorepeat is the event generated by keyboard auto-repeat
    /// @return key event
    static KeyEvent createKeyPress(int key, bool isautorepeat = false);

    /// Creates a key release KeyEvent object with the specified key code
    /// @param key key code of the event
    /// @return key event
    static KeyEvent createKeyRelease(int key);

  private:
    class D;
    D * m_d;
  };

  /// This class describes a mouse event. Mouse events are produced when the
  /// application has input focus and a mouse is either moved, or a button is
  /// pressed or relesed.
  class RADIANT_API MouseEvent
  {
  public:
    /// Construct a MouseEvent
    /// @param event QMouseEvent to copy
    MouseEvent(const QMouseEvent & event);
    /// Construct a MouseEvent
    /// @param event QWheelEvent to copy
    MouseEvent(const QWheelEvent & event);
    /// Construct a MouseEvent
    /// @param type must be one of QEvent::MouseButtonPress, QEvent::MouseButtonRelease, QEvent::MouseButtonDblClick, or QEvent::MouseMove
    /// @param location mouse cursor's location
    /// @param button the button that caused the event. If the event type is QEvent::MouseMove, the appropriate button for the event is Qt::NoButton
    /// @param buttons state of the mouse buttons
    /// @param modifiers state of keyboard modifiers
    MouseEvent(QEvent::Type type, const Nimble::Vector2f & location, Qt::MouseButton button,
                Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers);

    /// Construct a copy of a MouseEvent
    /// @param e event to copy
    MouseEvent(const MouseEvent & e);
    /// Construct a copy of a MouseEvent
    /// @param ev event to copy
    /// @return reference to this
    MouseEvent & operator=(const MouseEvent & ev);

    virtual ~MouseEvent();

    /// @cond

    MULTI_ATTR_DEPRECATED("Use location() instead", int x() const ) { return location().x; }
    MULTI_ATTR_DEPRECATED("Use location() instead", int y() const ) { return location().y; }

    /// @endcond

    /// Get the location of the mouse cursor at the time of the event
    /// @return location of the mouse cursor
    Nimble::Vector2f location() const;
    /// Set the location of the mouse cursor at the time of the event
    /// @param location location to set
    void setLocation(const Nimble::Vector2f & location);

    /// Return the distance the mouse wheel is rotated, in eights of a degree.
    /// @return distance the wheel is rotated
    int delta() const;

    /// Get the type of the event
    /// @return event type
    QEvent::Type type() const;

    /// Return the button that generated the event
    /// @return button that generated the event
    Qt::MouseButton button() const;
    /// Return the button state when the event was generated
    /// @return state of the buttons
    Qt::MouseButtons buttons() const;

    /// Return the keyboard modifiers at the time of the event
    /// @return keyboard modifiers
    Qt::KeyboardModifiers modifiers() const;
  private:
    class D;
    D * m_d;
  };
}

#endif
