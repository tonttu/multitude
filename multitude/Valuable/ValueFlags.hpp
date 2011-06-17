/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef VALUABLE_VALUEFLAGS_HPP
#define VALUABLE_VALUEFLAGS_HPP

#include "ValueObject.hpp"

#include <Radiant/Flags.hpp>

namespace Valuable {

  struct FlagNames
  {
    const char * name;
    int value;
  };

  /**
   * Valuable Flags, bitmask of enum values.
   *
   * @example
   * @code
   * /// FunnyWidget.hpp
   *
   * struct FunnyWidget {
   *   FunnyWidget();
   *
   *   enum Mode {
   *     ON = 1,
   *     OFF = 0
   *   };
   *
   *   enum InputFlags {
   *     INPUT_MOTION_X = 1 << 1,
   *     INPUT_MOTION_Y = 1 << 2,
   *     INPUT_MOTION_XY = INPUT_MOTION_X | INPUT_MOTION_Y
   *   };
   *
   *   /// m_mode is either ON or OFF
   *   ValueEnum<Mode> m_mode;
   *   /// m_flags is bitwise or of InputFlags values
   *   ValueFlags<InputFlags> m_flags;
   * };
   *
   * //////////////////////////////////////////////////////////////////////////
   *
   * /// FunnyWidget.cpp
   *
   * /// In CSS/Script you can use keywords "on" or "enabled / "off" or "disabled"
   * Valuable::Flags s_modes[] = {"on", Widget::ON, "enabled", Widget::ON,
   *                              "off", Widget::OFF, "disabled", Widget::OFF,
   *                              0, 0};
   *
   * /// In CSS/Script you can write "motion-x: true;" or "flags: motion-x motion-y;"
   * Valuable::Flags s_flags[] = {"motion-x", Widget::INPUT_MOTION_X,
   *                              "motion-y", Widget::INPUT_MOTION_Y,
   *                              "motion-xy", Widget::INPUT_MOTION_XY,
   *                              "INPUT_MOTION_X", Widget::INPUT_MOTION_X,
   *                              "INPUT_MOTION_Y", Widget::INPUT_MOTION_Y,
   *                              "INPUT_MOTION_XY", Widget::INPUT_MOTION_XY,
   *                              0, 0};
   *
   * FunnyWidget::FunnyWidget()
   *  : m_mode(this, "mode", s_modes, ON),
   *    m_flags(this, "flags", s_flags, INPUT_MOTION_XY)
   * {
   *   m_flags.mask("input-motion")
   *     ("xy", INPUT_MOTION_XY)
   *     ("x", INPUT_MOTION_X)
   *     ("y", INPUT_MOTION_Y);
   *   m_flags.mask("fixed")
   *     (true, INPUT_MOTION_XY)
   *     (false, 0);
   *
   * @endcode
   */
  template <typename T>
  class ValueFlagsT : ValueObject
  {
  public:
    typedef Radiant::FlagsT<T> Flags;
    ValueFlagsT(HasValues * parent, const QString & name, FlagNames * names,
               Flags v = Flags(), bool transit = false)
      : ValueObject(parent, name, transit)
    {
    }

    ValueFlagsT & operator=(const Flags & b) { setValue(b, OVERRIDE); return *this; }

    bool operator==(const Flags & b) const { return value() == b; }
    bool operator!=(const Flags & b) const { return value() != b; }

    bool operator!() const { return !value(); }
    Flags operator~() const { return ~value(); }

    Flags operator&(const Flags & b) const { return value() & b; }
    Flags operator|(const Flags & b) const { return value() | b; }
    Flags operator^(const Flags & b) const { return value() ^ b; }

    ValueFlagsT & operator&=(const Flags & b) { setValue(value() & b, OVERRIDE); return *this; }
    ValueFlagsT & operator|=(const Flags & b) { setValue(value() & b, OVERRIDE); return *this; }
    ValueFlagsT & operator^=(const Flags & b) { setValue(value() & b, OVERRIDE); return *this; }

    operator Flags() const { return m_cache; }

    /// best you can do to emulate c++0x explicit boolean conversion operator
    typedef void (ValueFlagsT<T>::*bool_type)();
    operator bool_type() const { return value() ? &ValueFlagsT<T>::updateCache : 0; }

    Flags value() const
    {
      return m_cache;
    }

    void setFlag(const Flags & f, bool state, Layer layer)
    {
      if(state) m_values[layer] |= f;
      else m_values[layer] &= ~f;
      m_masks[layer] |= f;
    }

    void clearFlag(const Flags & f, Layer layer)
    {
      m_masks[layer] &= ~f;
    }

    void setValue(Flags flags, Layer layer)
    {
      m_masks[layer] = ~Flags();
      m_values[layer] = flags;
      updateCache();
    }

    void clearValue(Layer layout)
    {
      m_masks[layout].clear();
      updateCache();
    }

    virtual bool deserialize(ArchiveElement & element)
    {
      /// @todo Should we serialize all layers?
      *this = Radiant::StringUtils::fromString<T>(element.get().toUtf8().data());
      return true;
    }

    virtual const char * type() const { return "ValueFlags"; }

    int asInt(bool * const ok = 0) const
    {
      if(ok) *ok = true;
      return value().asInt();
    }

    void processMessage(const char *, Radiant::BinaryData & data)
    {
      bool ok = true;
      uint32_t v = uint32_t(data.readInt32(&ok));

      if(ok) setValue(Flags::fromInt(v), OVERRIDE);
    }

  private:

    void updateCache()
    {
      Flags tmp = m_cache;
      m_cache.clear();
      Flags available = ~Flags();

      for(int layer = LAYER_COUNT - 1; layer >= 0; --layer) {
        Flags mask = m_masks[layer] & available;
        m_cache |= mask & m_values[layer];
        available &= ~m_masks[layer];
      }

      if(tmp != m_cache) emitChange();
    }

    Flags m_cache;
    Flags m_values[LAYER_COUNT];
    Flags m_masks[LAYER_COUNT];
  };
}

#endif // VALUABLE_VALUEFLAGS_HPP
