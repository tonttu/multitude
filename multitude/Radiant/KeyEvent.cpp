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

  VirtualKeyCode KeyEvent::convertToVirtualKeyCode(int key)
  {
    switch (key) {
      case Qt::Key_Escape: return VKEY_ESCAPE;
      case Qt::Key_Tab: return VKEY_TAB;
      case Qt::Key_Backtab: return VKEY_BACKTAB;
      case Qt::Key_Backspace: return VKEY_BACK;
      case Qt::Key_Return: return VKEY_RETURN;
      case Qt::Key_Insert: return VKEY_INSERT;
      case Qt::Key_Delete: return VKEY_DELETE;
      case Qt::Key_Pause: return VKEY_PAUSE;
      case Qt::Key_Print: return VKEY_PRINT;
      case Qt::Key_Clear: return VKEY_CLEAR;
      case Qt::Key_Home: return VKEY_HOME;
      case Qt::Key_End: return VKEY_END;
      case Qt::Key_Left: return VKEY_LEFT;
      case Qt::Key_Up: return VKEY_UP;
      case Qt::Key_Right: return VKEY_RIGHT;
      case Qt::Key_Down: return VKEY_DOWN;
      case Qt::Key_Shift: return VKEY_SHIFT;
      case Qt::Key_Control: return VKEY_CONTROL;
      case Qt::Key_NumLock: return VKEY_NUMLOCK;
      case Qt::Key_F1: return VKEY_F1;
      case Qt::Key_F2: return VKEY_F2;
      case Qt::Key_F3: return VKEY_F3;
      case Qt::Key_F4: return VKEY_F4;
      case Qt::Key_F5: return VKEY_F5;
      case Qt::Key_F6: return VKEY_F6;
      case Qt::Key_F7: return VKEY_F7;
      case Qt::Key_F8: return VKEY_F8;
      case Qt::Key_F9: return VKEY_F9;
      case Qt::Key_F10: return VKEY_F10;
      case Qt::Key_F11: return VKEY_F11;
      case Qt::Key_F12: return VKEY_F12;
      case Qt::Key_F13: return VKEY_F13;
      case Qt::Key_F14: return VKEY_F14;
      case Qt::Key_F15: return VKEY_F15;
      case Qt::Key_F16: return VKEY_F16;
      case Qt::Key_F17: return VKEY_F17;
      case Qt::Key_F18: return VKEY_F18;
      case Qt::Key_F19: return VKEY_F19;
      case Qt::Key_F20: return VKEY_F20;
      case Qt::Key_F21: return VKEY_F21;
      case Qt::Key_F22: return VKEY_F22;
      case Qt::Key_F23: return VKEY_F23;
      case Qt::Key_F24: return VKEY_F24;

      case Qt::Key_Menu: return VKEY_MENU;
      case Qt::Key_Help: return VKEY_HELP;
      case Qt::Key_Space: return VKEY_SPACE;

      case Qt::Key_Asterisk: return VKEY_MULTIPLY;
      case Qt::Key_Plus: return VKEY_ADD;
      case Qt::Key_Comma: return VKEY_SEPARATOR;
      case Qt::Key_Minus: return VKEY_SUBTRACT;
      case Qt::Key_Period: return VKEY_DECIMAL;
      case Qt::Key_Slash: return VKEY_DIVIDE;
      case Qt::Key_0: return VKEY_0;
      case Qt::Key_1: return VKEY_1;
      case Qt::Key_2: return VKEY_2;
      case Qt::Key_3: return VKEY_3;
      case Qt::Key_4: return VKEY_4;
      case Qt::Key_5: return VKEY_5;
      case Qt::Key_6: return VKEY_6;
      case Qt::Key_7: return VKEY_7;
      case Qt::Key_8: return VKEY_8;
      case Qt::Key_9: return VKEY_9;

      case Qt::Key_A: return VKEY_A;
      case Qt::Key_B: return VKEY_B;
      case Qt::Key_C: return VKEY_C;
      case Qt::Key_D: return VKEY_D;
      case Qt::Key_E: return VKEY_E;
      case Qt::Key_F: return VKEY_F;
      case Qt::Key_G: return VKEY_G;
      case Qt::Key_H: return VKEY_H;
      case Qt::Key_I: return VKEY_I;
      case Qt::Key_J: return VKEY_J;
      case Qt::Key_K: return VKEY_K;
      case Qt::Key_L: return VKEY_L;
      case Qt::Key_M: return VKEY_M;
      case Qt::Key_N: return VKEY_N;
      case Qt::Key_O: return VKEY_O;
      case Qt::Key_P: return VKEY_P;
      case Qt::Key_Q: return VKEY_Q;
      case Qt::Key_R: return VKEY_R;
      case Qt::Key_S: return VKEY_S;
      case Qt::Key_T: return VKEY_T;
      case Qt::Key_U: return VKEY_U;
      case Qt::Key_V: return VKEY_V;
      case Qt::Key_W: return VKEY_W;
      case Qt::Key_X: return VKEY_X;
      case Qt::Key_Y: return VKEY_Y;
      case Qt::Key_Z: return VKEY_Z;

      case Qt::Key_Enter: return VKEY_RETURN;

      case Qt::Key_SysReq: return VKEY_UNKNOWN;
      case Qt::Key_PageUp: return VKEY_UNKNOWN;
      case Qt::Key_PageDown: return VKEY_UNKNOWN;
      case Qt::Key_Meta: return VKEY_UNKNOWN;
      case Qt::Key_Alt: return VKEY_UNKNOWN;
      case Qt::Key_CapsLock: return VKEY_UNKNOWN;
      case Qt::Key_ScrollLock: return VKEY_UNKNOWN;
      case Qt::Key_F25: return VKEY_UNKNOWN;
      case Qt::Key_F26: return VKEY_UNKNOWN;
      case Qt::Key_F27: return VKEY_UNKNOWN;
      case Qt::Key_F28: return VKEY_UNKNOWN;
      case Qt::Key_F29: return VKEY_UNKNOWN;
      case Qt::Key_F30: return VKEY_UNKNOWN;
      case Qt::Key_F31: return VKEY_UNKNOWN;
      case Qt::Key_F32: return VKEY_UNKNOWN;
      case Qt::Key_F33: return VKEY_UNKNOWN;
      case Qt::Key_F34: return VKEY_UNKNOWN;
      case Qt::Key_F35: return VKEY_UNKNOWN;
      case Qt::Key_Super_L: return VKEY_UNKNOWN;
      case Qt::Key_Super_R: return VKEY_UNKNOWN;
      case Qt::Key_Hyper_L: return VKEY_UNKNOWN;
      case Qt::Key_Hyper_R: return VKEY_UNKNOWN;
      case Qt::Key_Direction_L: return VKEY_UNKNOWN;
      case Qt::Key_Direction_R: return VKEY_UNKNOWN;

      case Qt::Key_Exclam: return VKEY_UNKNOWN;
      case Qt::Key_QuoteDbl: return VKEY_UNKNOWN;
      case Qt::Key_NumberSign: return VKEY_UNKNOWN;
      case Qt::Key_Dollar: return VKEY_UNKNOWN;
      case Qt::Key_Percent: return VKEY_UNKNOWN;
      case Qt::Key_Ampersand: return VKEY_UNKNOWN;
      case Qt::Key_Apostrophe: return VKEY_UNKNOWN;
      case Qt::Key_ParenLeft: return VKEY_UNKNOWN;
      case Qt::Key_ParenRight: return VKEY_UNKNOWN;
      case Qt::Key_Colon: return VKEY_UNKNOWN;
      case Qt::Key_Semicolon: return VKEY_UNKNOWN;
      case Qt::Key_Less: return VKEY_UNKNOWN;
      case Qt::Key_Equal: return VKEY_UNKNOWN;
      case Qt::Key_Greater: return VKEY_UNKNOWN;
      case Qt::Key_Question: return VKEY_UNKNOWN;
      case Qt::Key_At: return VKEY_UNKNOWN;
      case Qt::Key_BracketLeft: return VKEY_UNKNOWN;
      case Qt::Key_Backslash: return VKEY_UNKNOWN;
      case Qt::Key_BracketRight: return VKEY_UNKNOWN;
      case Qt::Key_AsciiCircum: return VKEY_UNKNOWN;
      case Qt::Key_Underscore: return VKEY_UNKNOWN;
      case Qt::Key_QuoteLeft: return VKEY_UNKNOWN;
      case Qt::Key_BraceLeft: return VKEY_UNKNOWN;
      case Qt::Key_Bar: return VKEY_UNKNOWN;
      case Qt::Key_BraceRight: return VKEY_UNKNOWN;
      case Qt::Key_AsciiTilde: return VKEY_UNKNOWN;
    };
    return VKEY_UNKNOWN;

  #if 0
      // Still missing:
      VKEY_CAPITAL = 0x14,
      VKEY_KANA = 0x15,
      VKEY_HANGUL = 0x15,
      VKEY_JUNJA = 0x17,
      VKEY_FINAL = 0x18,
      VKEY_HANJA = 0x19,
      VKEY_KANJI = 0x19,

      VKEY_CONVERT = 0x1C,
      VKEY_NONCONVERT = 0x1D,
      VKEY_ACCEPT = 0x1E,
      VKEY_MODECHANGE = 0x1F,
      VKEY_PRIOR = 0x21,
      VKEY_NEXT = 0x22,
      VKEY_SELECT = 0x29,
      VKEY_EXECUTE = 0x2B,
      VKEY_SNAPSHOT = 0x2C,

      VKEY_LWIN = 0x5B,
      VKEY_COMMAND = VKEY_LWIN,  // Provide the Mac name for convenience.
      VKEY_RWIN = 0x5C,
      VKEY_APPS = 0x5D,
      VKEY_SLEEP = 0x5F,
      VKEY_NUMPAD0 = 0x60,
      VKEY_NUMPAD1 = 0x61,
      VKEY_NUMPAD2 = 0x62,
      VKEY_NUMPAD3 = 0x63,
      VKEY_NUMPAD4 = 0x64,
      VKEY_NUMPAD5 = 0x65,
      VKEY_NUMPAD6 = 0x66,
      VKEY_NUMPAD7 = 0x67,
      VKEY_NUMPAD8 = 0x68,
      VKEY_NUMPAD9 = 0x69,

      VKEY_SCROLL = 0x91,
      VKEY_LSHIFT = 0xA0,
      VKEY_RSHIFT = 0xA1,
      VKEY_LCONTROL = 0xA2,
      VKEY_RCONTROL = 0xA3,
      VKEY_LMENU = 0xA4,
      VKEY_RMENU = 0xA5,
      VKEY_BROWSER_BACK = 0xA6,
      VKEY_BROWSER_FORWARD = 0xA7,
      VKEY_BROWSER_REFRESH = 0xA8,
      VKEY_BROWSER_STOP = 0xA9,
      VKEY_BROWSER_SEARCH = 0xAA,
      VKEY_BROWSER_FAVORITES = 0xAB,
      VKEY_BROWSER_HOME = 0xAC,
      VKEY_VOLUME_MUTE = 0xAD,
      VKEY_VOLUME_DOWN = 0xAE,
      VKEY_VOLUME_UP = 0xAF,
      VKEY_MEDIA_NEXT_TRACK = 0xB0,
      VKEY_MEDIA_PREV_TRACK = 0xB1,
      VKEY_MEDIA_STOP = 0xB2,
      VKEY_MEDIA_PLAY_PAUSE = 0xB3,
      VKEY_MEDIA_LAUNCH_MAIL = 0xB4,
      VKEY_MEDIA_LAUNCH_MEDIA_SELECT = 0xB5,
      VKEY_MEDIA_LAUNCH_APP1 = 0xB6,
      VKEY_MEDIA_LAUNCH_APP2 = 0xB7,
      VKEY_OEM_1 = 0xBA,
      VKEY_OEM_PLUS = 0xBB,
      VKEY_OEM_COMMA = 0xBC,
      VKEY_OEM_MINUS = 0xBD,
      VKEY_OEM_PERIOD = 0xBE,
      VKEY_OEM_2 = 0xBF,
      VKEY_OEM_3 = 0xC0,
      VKEY_OEM_4 = 0xDB,
      VKEY_OEM_5 = 0xDC,
      VKEY_OEM_6 = 0xDD,
      VKEY_OEM_7 = 0xDE,
      VKEY_OEM_8 = 0xDF,
      VKEY_OEM_102 = 0xE2,
      VKEY_OEM_103 = 0xE3,  // GTV KEYCODE_MEDIA_REWIND
      VKEY_OEM_104 = 0xE4,  // GTV KEYCODE_MEDIA_FAST_FORWARD
      VKEY_PROCESSKEY = 0xE5,
      VKEY_PACKET = 0xE7,
      VKEY_DBE_SBCSCHAR = 0xF3,
      VKEY_DBE_DBCSCHAR = 0xF4,
      VKEY_ATTN = 0xF6,
      VKEY_CRSEL = 0xF7,
      VKEY_EXSEL = 0xF8,
      VKEY_EREOF = 0xF9,
      VKEY_PLAY = 0xFA,
      VKEY_ZOOM = 0xFB,
      VKEY_NONAME = 0xFC,
      VKEY_PA1 = 0xFD,
      VKEY_OEM_CLEAR = 0xFE,

      // POSIX specific VKEYs. Note that as of Windows SDK 7.1, 0x97-9F, 0xD8-DA,
      // and 0xE8 are unassigned.
      VKEY_WLAN = 0x97,
      VKEY_POWER = 0x98,
      VKEY_BRIGHTNESS_DOWN = 0xD8,
      VKEY_BRIGHTNESS_UP = 0xD9,
      VKEY_KBD_BRIGHTNESS_DOWN = 0xDA,
      VKEY_KBD_BRIGHTNESS_UP = 0xE8,

      // Windows does not have a specific key code for AltGr. We use the unused 0xE1
      // (VK_OEM_AX) code to represent AltGr, matching the behaviour of Firefox on
      // Linux.
      VKEY_ALTGR = 0xE1,
      // Windows does not have a specific key code for Compose. We use the unused
      // 0xE6 (VK_ICO_CLEAR) code to represent Compose.
      VKEY_COMPOSE = 0xE6,
  #endif
  }

  int KeyEvent::key() const
  {
    return m_d->m_event->key();
  }

  VirtualKeyCode KeyEvent::virtualKeyCode() const
  {
    return convertToVirtualKeyCode(key());
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
    m_d->location = Nimble::Vector2f(event.localPos().x(), event.localPos().y());
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
