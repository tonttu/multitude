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

#include "AttributeObject.hpp"
#include "StyleValue.hpp"

#include <Radiant/Flags.hpp>
#include <Radiant/StringUtils.hpp>
#include <Radiant/Trace.hpp>

#include <QMap>
#include <QByteArray>

namespace Valuable {

  /// This struct is used to define the name strings for flags so they can be
  /// referenced from CSS.
  struct FlagNames
  {
    /// Name of the flag
    const char * name;
    /// Value of the value
    long value;
  };

  template <typename T> class AttributeFlagsT;

  /// This class provides a mechanism to toggle individual flags on and off
  /// using their name from CSS. With this class we can write things like:
  /// @code
  /// Widget {
  ///   input-pass-to-children: true;
  /// @endcode
  template <typename T>
  class FlagAliasT : public Attribute
  {
  public:
    FlagAliasT(Node * parent, AttributeFlagsT<T> & master,
               const QByteArray & name, Radiant::FlagsT<T> flags)
      : Attribute(parent, name, false),
        m_master(master),
        m_flags(flags)
    {
    }

    virtual bool set(int v, Layer layer, ValueUnit) OVERRIDE
    {
      m_master.setFlags(m_flags, v, layer);
      return true;
    }

    bool set(const StyleValue & v, Layer layer) OVERRIDE
    {
      if(v.size() != 1 || v.units()[0] != VU_UNKNOWN) return false;
      QString p = v.values()[0].toString().toLower();
      bool on = p == "true" || p == "on" || p == "yes";
      if(on || p == "false" || p == "off" || p == "no") {
        m_master.setFlags(m_flags, on, layer);
        return true;
      }
      return false;
    }

    void clearValue(Layer layout) OVERRIDE
    {
      m_master.clearFlags(m_flags, layout);
    }

    Radiant::FlagsT<T> flags() const { return m_flags; }

    ArchiveElement serialize(Archive &) const OVERRIDE { return ArchiveElement(); }
    bool deserialize(const ArchiveElement &) OVERRIDE { return false; }

  private:
    AttributeFlagsT<T> & m_master;
    const Radiant::FlagsT<T> m_flags;
  };

  /**
   * Attribute containing flags, bitmask of enum values.
   *
   * @code
   * /// FunnyWidget.hpp
   *
   * struct FunnyWidget {
   *   FunnyWidget();
   *
   *   enum InputFlags {
   *     INPUT_TRANSLATE_X = 1 << 1,
   *     INPUT_TRANSLATE_Y = 1 << 2,
   *     INPUT_TRANSLATE_XY = INPUT_TRANSLATE_X | INPUT_TRANSLATE_Y
   *   };
   *
   *   /// m_flags is bitwise or of InputFlags values
   *   AttributeFlags<InputFlags> m_flags;
   * };
   *
   * //////////////////////////////////////////////////////////////////////////
   *
   * /// FunnyWidget.cpp
   *
   * /// In CSS/Script you can write "motion-x: true;" or "flags: motion-x motion-y;"
   * Valuable::FlagNames s_flags[] = {{"translate-x", FunnyWidget::INPUT_TRANSLATE_X},
   *                                  {"translate-y", FunnyWidget::INPUT_TRANSLATE_Y},
   *                                  {"translate-xy", FunnyWidget::INPUT_TRANSLATE_XY},
   *                                  {"INPUT_TRANSLATE_X", FunnyWidget::INPUT_TRANSLATE_X},
   *                                  {"INPUT_TRANSLATE_Y", FunnyWidget::INPUT_TRANSLATE_Y},
   *                                  {"INPUT_TRANSLATE_XY", FunnyWidget::INPUT_TRANSLATE_XY},
   *                                  {0, 0}};
   *
   * FunnyWidget::FunnyWidget()
   *  : m_flags(this, "flags", s_flags, INPUT_TRANSLATE_XY)
   * {
   *   m_flags.mask("input-translate")
   *     ("xy", INPUT_TRANSLATE_XY)
   *     ("x", INPUT_TRANSLATE_X)
   *     ("y", INPUT_TRANSLATE_Y);
   *   m_flags.mask("fixed")
   *     (true, INPUT_TRANSLATE_XY)
   *     (false, 0);
   *
   * @endcode
   */
  template <typename T>
  class AttributeFlagsT : public Attribute
  {
  public:
    typedef Radiant::FlagsT<T> Flags;
    AttributeFlagsT(Node * parent, const QByteArray & name, const FlagNames * names,
               Flags v = Flags(), bool createAliases = true, bool transit = false)
      : Attribute(parent, name, transit)
    {
      m_masks[DEFAULT] = ~Flags();
      m_values[DEFAULT] = v;
      m_cache = v;

      assert(names);

      for (const FlagNames * it = names; it->name; ++it) {
        m_flags[QByteArray(it->name).toLower()] = Flags::fromInt(it->value);
        if (parent && createAliases)
          m_aliases << new FlagAliasT<T>(parent, *this, it->name, Flags::fromInt(it->value));
      }
    }

    AttributeFlagsT & operator=(const Flags & b) { setValue(b, USER); return *this; }

    bool operator==(const Flags & b) const { return value() == b; }
    bool operator!=(const Flags & b) const { return value() != b; }

    bool operator!() const { return !value(); }
    Flags operator~() const { return ~value(); }

    Flags operator&(const Flags & b) const { return value() & b; }
    Flags operator|(const Flags & b) const { return value() | b; }
    Flags operator^(const Flags & b) const { return value() ^ b; }

    AttributeFlagsT & operator&=(const Flags & b) { setValue(value() & b, USER); return *this; }
    AttributeFlagsT & operator|=(const Flags & b) { setValue(value() & b, USER); return *this; }
    AttributeFlagsT & operator^=(const Flags & b) { setValue(value() & b, USER); return *this; }

    operator Flags() const { return m_cache; }

    /// best you can do to emulate c++0x explicit boolean conversion operator
    typedef void (AttributeFlagsT<T>::*bool_type)();
    operator bool_type() const { return value() ? &AttributeFlagsT<T>::updateCache : 0; }

    Flags value() const
    {
      return m_cache;
    }

    void setFlags(const Flags & f, bool state, Layer layer = USER)
    {
      if(state) m_values[layer] |= f;
      else m_values[layer] &= ~f;
      m_masks[layer] |= f;
      updateCache();
    }

    void clearFlags(const Flags & f, Layer layer)
    {
      m_masks[layer] &= ~f;
      updateCache();
    }

    virtual void setValue(const Flags & flags, Layer layer)
    {
      m_masks[layer] = ~Flags();
      m_values[layer] = flags;
      updateCache();
    }

    virtual void clearValue(Layer layout) OVERRIDE
    {
      m_masks[layout].clear();
      updateCache();
    }

    virtual QString asString(bool * const ok) const OVERRIDE
    {
      /// @todo should serialize using flag names, not ints
      return QString::number(asInt(ok));
    }

    virtual bool deserialize(const ArchiveElement & element) OVERRIDE
    {
      /// @todo Should we serialize all layers?
      *this = Radiant::StringUtils::fromString<T>(element.get().toUtf8());
      return true;
    }

    virtual int asInt(bool * const ok = 0) const OVERRIDE
    {
      if(ok) *ok = true;
      return value().asInt();
    }

    virtual void processMessage(const QByteArray &, Radiant::BinaryData & data) OVERRIDE
    {
      bool ok = true;
      uint32_t v = uint32_t(data.readInt32(&ok));

      if(ok) setValue(Flags::fromInt(v), USER);
    }

    virtual bool set(int v, Layer layer, ValueUnit /*unit*/ = VU_UNKNOWN) OVERRIDE
    {
      Radiant::warning("AttributeFlagsT::set # using deprecated functionality, do not set flags with numbers");
      setValue(Flags::fromInt(v), layer);
      return true;
    }

    virtual bool set(const StyleValue & v, Layer layer = USER) OVERRIDE
    {
      for(const ValueUnit & vu : v.units())
        if(vu != VU_UNKNOWN) return false;

      Flags newValue;
      for(const QVariant & var : v.values()) {
        if(var.type() != QVariant::ByteArray) return false;
        auto it = m_flags.find(var.toByteArray().toLower());
        if (it == m_flags.end()) return false;
        newValue |= *it;
      }
      setValue(newValue, layer);
      return true;
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
    QList<FlagAliasT<T>*> m_aliases;
    QMap<QByteArray, Flags> m_flags;
  };
}

#endif // VALUABLE_VALUEFLAGS_HPP
