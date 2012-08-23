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

  struct FlagNames
  {
    const char * name;
    long value;
  };

  template <typename T> class AttributeFlagsT;

  template <typename T>
  class FlagAliasT : public Attribute
  {
  public:
    FlagAliasT(Node * parent, AttributeFlagsT<T> & master,
               const QString & name, Radiant::FlagsT<T> flags)
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

    const char * type() const OVERRIDE { return "FlagAlias"; }
    ArchiveElement serialize(Archive &) const OVERRIDE { return ArchiveElement(); }
    bool deserialize(const ArchiveElement &) OVERRIDE { return false; }

  private:
    AttributeFlagsT<T> & m_master;
    const Radiant::FlagsT<T> m_flags;
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
   *   enum InputFlags {
   *     INPUT_MOTION_X = 1 << 1,
   *     INPUT_MOTION_Y = 1 << 2,
   *     INPUT_MOTION_XY = INPUT_MOTION_X | INPUT_MOTION_Y
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
   * Valuable::FlagNames s_flags[] = {{"motion-x", FunnyWidget::INPUT_MOTION_X},
   *                                  {"motion-y", FunnyWidget::INPUT_MOTION_Y},
   *                                  {"motion-xy", FunnyWidget::INPUT_MOTION_XY},
   *                                  {"INPUT_MOTION_X", FunnyWidget::INPUT_MOTION_X},
   *                                  {"INPUT_MOTION_Y", FunnyWidget::INPUT_MOTION_Y},
   *                                  {"INPUT_MOTION_XY", FunnyWidget::INPUT_MOTION_XY},
   *                                  {0, 0}};
   *
   * FunnyWidget::FunnyWidget()
   *  : m_flags(this, "flags", s_flags, INPUT_MOTION_XY)
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
  class AttributeFlagsT : public Attribute
  {
  public:
    typedef Radiant::FlagsT<T> Flags;
    AttributeFlagsT(Node * parent, const QString & name, const FlagNames * names,
               Flags v = Flags(), bool transit = false)
      : Attribute(parent, name, transit)
    {
      m_masks[DEFAULT] = ~Flags();
      m_values[DEFAULT] = v;
      m_cache = v;

      if(parent && names) {
        for(const FlagNames * it = names; it->name; ++it) {
          m_aliases << new FlagAliasT<T>(parent, *this, it->name, Flags::fromInt(it->value));
        }
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

    void setFlags(const Flags & f, bool state = true, Layer layer = USER)
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
      *this = Radiant::StringUtils::fromString<T>(element.get().toUtf8().data());
      return true;
    }

    virtual const char * type() const OVERRIDE { return "AttributeFlags"; }

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
      foreach(const ValueUnit & vu, v.units())
        if(vu != VU_UNKNOWN) return false;

      QMap<QByteArray, Flags> all;
      foreach(const FlagAliasT<T>* alias, m_aliases)
        all[alias->name().toUtf8()] = alias->flags();

      Flags newValue;
      foreach(const QVariant & var, v.values()) {
        if(var.type() != QVariant::ByteArray) return false;
        typename QMap<QByteArray, Flags>::iterator it = all.find(var.toByteArray());
        if(it == all.end()) return false;
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
  };
}

#endif // VALUABLE_VALUEFLAGS_HPP
