/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUEFLAGS_HPP
#define VALUABLE_VALUEFLAGS_HPP

#include "Attribute.hpp"
#include "StyleValue.hpp"

#include <Radiant/Flags.hpp>
#include <Radiant/StringUtils.hpp>
#include <Radiant/Trace.hpp>

#include <QMap>
#include <QByteArray>

#include <bitset>

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
      if(v.size() != 1 || v.unit() != VU_UNKNOWN) return false;
      QByteArray p = v.asKeyword().toLower();
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

    ArchiveElement serialize(Archive & archive) const OVERRIDE
    {
      Layer layer;
      if (!layerForSerialization(archive, layer))
        return ArchiveElement();
      ArchiveElement e = archive.createElement(name());
      e.set(m_master.value(layer) ? "true" : "false");
      return e;
    }

    bool deserialize(const ArchiveElement & e) OVERRIDE
    {
      const QByteArray p = e.get().toUtf8().trimmed();
      bool on = p == "true" || p == "on" || p == "yes";
      if (on || p == "false" || p == "off" || p == "no") {
        m_master.setFlags(m_flags, on);
        return true;
      }
      return false;
    }

    virtual bool handleShorthand(const Valuable::StyleValue & value,
                                 QMap<Valuable::Attribute*, Valuable::StyleValue> & expanded) OVERRIDE
    {
      if (m_sources.empty())
        return false;

      for (auto source: m_sources)
        expanded[source] = value;

      return true;
    }

    virtual bool isValueDefinedOnLayer(Layer layer) const OVERRIDE
    {
      return m_master.isFlagDefinedOnLayer(m_flags, layer);
    }

    void setSources(std::vector<FlagAliasT<T>*> sources)
    {
      m_sources = sources;
      setSerializable(sources.empty());
    }

  private:
    AttributeFlagsT<T> & m_master;
    const Radiant::FlagsT<T> m_flags;
    std::vector<FlagAliasT<T>*> m_sources;
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

      std::map<T, FlagAliasT<T>*> bits;
      for (const FlagNames * it = names; it->name; ++it) {
        auto flag = Flags::fromInt(it->value);
        m_flags[QByteArray(it->name).toLower()] = flag;
        if (parent && createAliases && it->value) {
          auto alias = new FlagAliasT<T>(parent, *this, it->name, flag);
          m_aliases << alias;

          std::bitset<sizeof(T)*8> bitset(it->value);
          if (bitset.count() == 1) {
            const T t = T(it->value);
            if (bits.find(t) == bits.end())
              bits[t] = alias;
          }
        }
      }

      // mark some of the aliases to be shorthands, for example this would
      // mark "input-translate-xy" to be a shorhand for "input-translate-x" and "...-y"
      for (auto alias: m_aliases) {
        std::bitset<sizeof(T)*8> bitset(alias->flags().asInt());
        bool found = true;
        std::vector<FlagAliasT<T>*> sources;
        for (std::size_t i = 0; i < bitset.size(); ++i) {
          if (!bitset[i])
            continue;

          const T t = T(1 << i);
          auto it = bits.find(t);
          if (it == bits.end() || it->second == alias) {
            found = false;
            break;
          }
          sources.push_back(it->second);
        }

        if (found)
          alias->setSources(sources);

        if (sources.empty())
          alias->setOwnerShorthand(this);
      }

      if (createAliases)
        setSerializable(false);
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
    AttributeFlagsT & operator|=(const Flags & b) { setValue(value() | b, USER); return *this; }
    AttributeFlagsT & operator^=(const Flags & b) { setValue(value() ^ b, USER); return *this; }

    operator Flags() const { return m_cache; }

    /// best you can do to emulate c++0x explicit boolean conversion operator
    typedef void (AttributeFlagsT<T>::*bool_type)();
    operator bool_type() const { return value() ? &AttributeFlagsT<T>::updateCache : 0; }

    Flags value() const
    {
      return m_cache;
    }

    /// For example value(USER) returns bitset that includes bits only from
    /// USER level, not from any lower or higher priority level. This might
    /// be confusing in some situations.
    Flags value(Layer layer) const
    {
      return layer == LAYER_CURRENT ? value() : value(layer, layer);
    }

    /// Collects bits between layers topLayer to bottomLayer (inclusive) to bitmask
    Flags value(Layer topLayer, Layer bottomLayer) const
    {
      Flags flags;
      Flags available = ~Flags();

      for(int layer = topLayer; layer >= bottomLayer; --layer) {
        Flags mask = m_masks[layer] & available;
        flags |= mask & m_values[layer];
        available &= ~m_masks[layer];
      }

      return flags;
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

    QString asString(bool * const ok, Layer layer) const
    {
      if (ok)
        *ok = true;
      return stringify(value(layer));
    }

    virtual bool deserialize(const ArchiveElement & element) OVERRIDE
    {
      QString tmp = element.get();
      bool ok = false;
      int num = tmp.toInt(&ok);
      if (ok) {
        *this = Flags::fromInt(num);
      } else {
        Flags newValue;
        for (auto str: tmp.split(QRegExp("\\s+"), QString::SkipEmptyParts)) {
          auto it = m_flags.find(str.toUtf8().toLower());
          if (it == m_flags.end()) return false;
          newValue |= *it;
        }
        *this = newValue;
      }
      return true;
    }

    virtual int asInt(bool * const ok, Layer layer) const OVERRIDE
    {
      if(ok) *ok = true;
      return value(layer).asInt();
    }

    virtual void eventProcess(const QByteArray &, Radiant::BinaryData & data) OVERRIDE
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
      if (!v.isUniform() || !v[0].canConvert(StyleValue::TYPE_KEYWORD))
        return false;

      Flags newValue;
      for(const auto & var : v.components()) {
        auto it = m_flags.find(var.asKeyword().toLower());
        if (it == m_flags.end()) return false;
        newValue |= *it;
      }
      setValue(newValue, layer);
      return true;
    }

    bool isFlagDefinedOnLayer(Radiant::FlagsT<T> flags, Layer layer) const
    {
      return (m_masks[layer] & flags) == flags;
    }

  private:

    QString stringify(Flags v) const
    {
      // All matching flags sorted by their popcount and their location in the
      // original array to optimize the string representation.
      // For example motion-x and motion-y will become motion-xy, since motion-xy
      // has a higher popcount than motion-x / -y
      std::multimap<int, std::pair<QByteArray, Flags> > flags;

      int i = 0;
      for (auto it = m_flags.begin(); it != m_flags.end(); ++it, ++i) {
        if ((v & *it) == *it) {
          std::bitset<sizeof(T)*8> bitset(it->asInt());
          flags.insert(std::make_pair(i-1024*int(bitset.count()), std::make_pair(it.key(), *it)));
        }
      }

      QStringList flagLst;
      for (auto p: flags) {
        Flags v2 = p.second.second;
        if ((v2 & v) == v2) {
          // If we have bits on that aren't actual enums (or the string version
          // is missing), v might still have some bits enabled, but v2 might be
          // an enum with a value of zero. Skip this case, since otherwise we
          // might have something like "lock-depth flags-none" that makes no
          // sense.
          if (v2 || flagLst.isEmpty())
            flagLst << p.second.first;
          // these bits are now consumed by this flag, remove those from the value
          v &= ~v2;
          if (!v)
            break;
        }
      }

      return flagLst.join(" ");
    }

    void updateCache()
    {
      Flags tmp = m_cache;
      m_cache = value(Layer(LAYER_COUNT - 1), Layer(0));
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
