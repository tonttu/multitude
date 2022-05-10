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

#include <boost/container/flat_map.hpp>

namespace Valuable {

  /// This struct is used to define the name strings for flags so they can be
  /// referenced from CSS.
  struct FlagName
  {
    /// Name of the flag
    const char * name;
    /// Value of the value
    long value;
    /// Should we create an alias for this flag so that you can write "name: value" in CSS
    bool createAlias;
  };

  struct FlagNames
  {
    FlagNames(std::initializer_list<FlagName> l)
    {
      m_flags.reserve(l.size());
      for (const FlagName & n: l) {
        if (!n.name)
          break;
        FlagData & d = m_flags[QByteArray(n.name).toLower()];
        if (n.createAlias && n.value)
          d.aliasIdx = 0;
        d.flags = n.value;
      }

      std::map<uint64_t, int32_t> aliases;
      std::map<uint64_t, int32_t> bits;
      for (auto & p: m_flags) {
        const QByteArray & name = p.first;
        FlagData & flag = p.second;

        if (flag.aliasIdx == 0) {
          int32_t idx = m_aliases.size();
          m_aliases.emplace_back();
          Alias & alias = m_aliases.back();
          alias.name = name;
          alias.flags = flag.flags;

          flag.aliasIdx = idx;
          flag.linkIdx = idx;
          aliases[flag.flags] = idx;

          std::bitset<64> bitset(flag.flags);
          if (bitset.count() == 1) {
            if (bits.find(flag.flags) == bits.end())
              bits[flag.flags] = idx;
          }
        }
      }

      // mark some of the aliases to be shorthands, for example this would
      // mark "input-translate-xy" to be a shorhand for "input-translate-x" and "...-y"
      for (auto & p: m_flags) {
        FlagData & flag = p.second;

        if (flag.aliasIdx == -1) {
          auto it = aliases.find(flag.flags);
          if (it != aliases.end())
            flag.linkIdx = it->second;
          continue;
        }

        std::bitset<64> bitset(flag.flags);
        Alias & alias = m_aliases[flag.aliasIdx];
        for (std::size_t i = 0; i < bitset.size(); ++i) {
          if (!bitset[i])
            continue;

          const uint64_t t = static_cast<uint64_t>(1) << i;
          auto it = bits.find(t);
          if (it == bits.end() || it->second == flag.aliasIdx) {
            alias.sources.clear();
            break;
          }
          alias.sources.push_back(it->second);
        }
      }
    }

    struct FlagData
    {
      uint64_t flags = 0;
      int32_t aliasIdx = -1;
      int32_t linkIdx = -1;
    };

    struct Alias
    {
      QByteArray name;
      uint64_t flags = 0;
      std::vector<uint32_t> sources;
    };

    boost::container::flat_map<QByteArray, FlagData> m_flags;
    std::vector<Alias> m_aliases;
  };

  class FlagAlias : public Attribute
  {
  public:
    FlagAlias(Node * parent, const QByteArray & name)
      : Attribute(parent, name) {}
  };

  /// This class provides a mechanism to toggle individual flags on and off
  /// using their name from CSS. With this class we can write things like:
  /// @code
  /// Widget {
  ///   input-pass-to-children: true;
  /// }
  /// @endcode
  template <typename T>
  class FlagAliasT : public FlagAlias
  {
  public:
    FlagAliasT(Node * parent, AttributeT<Radiant::FlagsT<T>> & master,
               const FlagNames::Alias & data)
      : FlagAlias(parent, data.name),
        m_master(master),
        m_flags(Radiant::FlagsT<T>::fromInt(data.flags)),
        m_data(data)
    {
    }

    virtual bool set(int v, Layer layer, ValueUnit) OVERRIDE
    {
      m_master.setFlags(m_flags, v != 0, layer);
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
      auto flags = m_master.value(layer) & m_flags;
      if (flags == m_flags)
        e.set("true");
      else if (!flags)
        e.set("false");
      else
        e.set("");
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
      if (p == "")
        return true;
      return false;
    }

    virtual bool handleShorthand(const Valuable::StyleValue & value,
                                 Radiant::ArrayMap<Valuable::Attribute*, Valuable::StyleValue> & expanded) OVERRIDE
    {
      if (m_data.sources.empty())
        return false;

      for (uint32_t idx: m_data.sources)
        expanded[&m_master.m_aliases[idx]] = value;

      return true;
    }

    virtual bool isValueDefinedOnLayer(Layer layer) const OVERRIDE
    {
      return m_master.isFlagDefinedOnLayer(m_flags, layer);
    }

    virtual int asInt(bool * const ok, Layer layer) const OVERRIDE
    {
      if (ok)
        *ok = true;
      return (m_master.value(layer) & m_flags) == m_flags ? 1 : 0;
    }

    virtual QString asString(bool * const ok, Layer layer) const override
    {
      return asInt(ok, layer) ? "true" : "false";
    }

    virtual QByteArray type() const override
    {
      return "flag";
    }

  private:
    friend class AttributeT<Radiant::FlagsT<T>>;

    AttributeT<Radiant::FlagsT<T>> & m_master;
    const Radiant::FlagsT<T> m_flags;
    const FlagNames::Alias & m_data;
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
   * /// In CSS you can write "input-translate-x: true;" or "flags: translate-x translate-y;"
   * Valuable::FlagNames s_flags = {{"input-translate-x", FunnyWidget::INPUT_TRANSLATE_X, true},
   *                                {"input-translate-y", FunnyWidget::INPUT_TRANSLATE_Y, true},
   *                                {"input-translate-xy", FunnyWidget::INPUT_TRANSLATE_XY, true},
   *                                {"translate-x", FunnyWidget::INPUT_TRANSLATE_X, false},
   *                                {"translate-y", FunnyWidget::INPUT_TRANSLATE_Y, false},
   *                                {"translate-xy", FunnyWidget::INPUT_TRANSLATE_XY, false}};
   *
   * FunnyWidget::FunnyWidget()
   *  : m_flags(this, "flags", s_flags, INPUT_TRANSLATE_XY)
   * {}
   * @endcode
   */
  template <typename Flags>
  class AttributeT<Flags, typename std::enable_if<std::is_base_of<Radiant::Flags, Flags>::value>::type>
      : public Attribute
  {
  public:
    typedef typename Flags::Enum T;
    AttributeT(Node * parent, const QByteArray & name, const FlagNames & data,
               Flags v = Flags())
      : Attribute(parent, name)
      , m_data(data)
    {
      m_masks[DEFAULT] = ~Flags();
      m_values[DEFAULT] = v;
      m_cache = v;

      m_aliases.reserve(m_data.m_aliases.size());
      for (const FlagNames::Alias & a: m_data.m_aliases) {
        m_aliases.emplace_back(parent, *this, a);
        if (a.sources.empty())
          m_aliases.back().setOwnerShorthand(this);
      }

      if (!m_aliases.empty())
        setSerializable(false);
    }

    virtual ~AttributeT()
    {
    }

    AttributeT & operator=(const Flags & b) { setValue(b, USER); return *this; }

    bool operator==(const Flags & b) const { return value() == b; }
    bool operator!=(const Flags & b) const { return value() != b; }

    bool operator!() const { return !value(); }
    Flags operator~() const { return ~value(); }

    Flags operator&(const Flags & b) const { return value() & b; }
    Flags operator|(const Flags & b) const { return value() | b; }
    Flags operator^(const Flags & b) const { return value() ^ b; }

    AttributeT & operator&=(const Flags & b) { setValue(value() & b, USER); return *this; }
    AttributeT & operator|=(const Flags & b) { setValue(value() | b, USER); return *this; }
    AttributeT & operator^=(const Flags & b) { setValue(value() ^ b, USER); return *this; }

    operator Flags() const { return m_cache; }

    /// best you can do to emulate c++0x explicit boolean conversion operator
    typedef void (AttributeT<Radiant::FlagsT<T>>::*bool_type)();
    operator bool_type() const { return value() ? &AttributeT<Radiant::FlagsT<T>>::updateCache : 0; }

    Flags value() const
    {
      return m_cache;
    }

    /// For example value(USER) returns bitset that includes bits only from
    /// USER level, not from any lower or higher priority level. This might
    /// be confusing in some situations.
    Flags value(Layer layer) const
    {
      return layer >= CURRENT_LAYER ? value() : value(layer, layer);
    }

    /// Collects bits between layers topLayer to bottomLayer (inclusive) to bitmask
    Flags value(Layer topLayer, Layer bottomLayer) const
    {
      if (topLayer >= CURRENT_LAYER || bottomLayer >= CURRENT_LAYER) {
        return value();
      }

      Flags flags;
      Flags available = ~Flags();

      for(int layer = topLayer; layer >= bottomLayer; --layer) {
        Flags mask = m_masks[layer] & available;
        flags |= mask & m_values[layer];
        available &= ~m_masks[layer];
      }

      return flags;
    }

    void setFlags(Flags f, bool state, Layer layer = USER)
    {
      if (layer >= CURRENT_LAYER) {
        Radiant::warning("AttributeFlag::setFlags # CURRENT_LAYER / CURRENT_VALUE not supported");
        return;
        /// @todo here is one possible way to define this, but most likely
        /// it doesn't really have any real use cases, so it's commented out.
        /*
        Flags mask = f;
        for (Layer l = STYLE_IMPORTANT; l >= DEFAULT && mask; l = Layer(l - 1)) {
          if (state) {
            m_values[l] |= mask & m_masks[l];
          } else {
            m_values[l] &= ~(mask & m_masks[l]);
          }

          mask & ~m_masks[l];
        }
        assert(!mask);*/
      } else {
        if(state) m_values[layer] |= f;
        else m_values[layer] &= ~f;
        m_masks[layer] |= f;
        updateCache();
      }
    }

    void clearFlags(const Flags & f, Layer layer)
    {
      if (layer >= CURRENT_LAYER) {
        Radiant::warning("AttributeFlag::clearFlags # CURRENT_LAYER / CURRENT_VALUE not supported");
        return;
      }
      m_masks[layer] &= ~f;
      updateCache();
    }

    virtual void setValue(const Flags & flags, Layer layer)
    {
      if (layer >= CURRENT_LAYER) {
        Radiant::warning("AttributeFlag::setValue # CURRENT_LAYER / CURRENT_VALUE not supported");
        return;
      }
      m_masks[layer] = ~Flags();
      m_values[layer] = flags;
      updateCache();
    }

    virtual void clearValue(Layer layer) OVERRIDE
    {
      if (layer >= CURRENT_LAYER) {
        Radiant::warning("AttributeFlag::clearValue # CURRENT_LAYER / CURRENT_VALUE not supported");
        return;
        /// See the comment in setFlags
        Flags clearedBits;
        for (Layer l = STYLE_IMPORTANT; l > DEFAULT; l = Layer(l - 1)) {
          Flags mask = m_masks[l];
          m_masks[l] = clearedBits & mask;
          clearedBits |= mask;
        }
        m_values[DEFAULT] &= clearedBits;
      } else {
        m_masks[layer].clear();
      }
      updateCache();
    }

    virtual void setAsDefaults() OVERRIDE
    {
      const Flags mask = m_masks[USER];
      if (!mask)
        return;

      const auto valueUser = value(USER);
      const auto valueDefault = value(DEFAULT);

      m_masks[USER].clear();
      setValue((valueUser & mask) | (valueDefault & ~mask), DEFAULT);
    }

    virtual QString asString(bool * const ok, Layer layer) const override
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
          auto it = m_data.m_flags.find(str.toLatin1().toLower());
          if (it == m_data.m_flags.end()) return false;
          newValue |= Flags::fromInt(it->second.flags);
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
        auto it = m_data.m_flags.find(var.asKeyword().toLower());
        if (it == m_data.m_flags.end()) return false;
        newValue |= Flags::fromInt(it->second.flags);
      }
      setValue(newValue, layer);
      return true;
    }

    virtual QByteArray type() const override
    {
      return "flags";
    }

    virtual bool isValueDefinedOnLayer(Layer layer) const override
    {
      if (layer >= CURRENT_LAYER)
        return true;

      return m_masks[layer];
    }

    bool isFlagDefinedOnLayer(Radiant::FlagsT<T> flags, Layer layer) const
    {
      if (layer >= CURRENT_LAYER) {
        return true;
      }
      return (m_masks[layer] & flags) == flags;
    }

    static inline Flags interpolate(Flags a, Flags b, float m)
    {
      return m >= 0.5f ? b : a;
    }

    // If we have a css rule input-flags: translate-xy; it actually means that
    // we are setting all individual flags to false, except translate-x and
    // translate-y that are set to true. We expand given set of flag names to
    // full set of attributes with either true or false as values.
    virtual bool handleShorthand(const Valuable::StyleValue & value,
                                 Radiant::ArrayMap<Valuable::Attribute*, Valuable::StyleValue> & expanded) OVERRIDE
    {
      // First set everything to false
      // If CSS parser could order components to some order and m_flags would
      // be in the same order, we could iterate them with two iterators
      // without need to have two passes.
      for (auto it = m_data.m_flags.begin(); it != m_data.m_flags.end(); ++it) {
        auto linkIdx = it->second.linkIdx;
        if (linkIdx == -1)
          continue;
        auto & alias = m_data.m_aliases[linkIdx];
        if (!alias.sources.empty())
          continue;

        expanded[&m_aliases[linkIdx]] = 0;
      }

      for (const auto & var: value.components()) {
        auto it = m_data.m_flags.find(var.asKeyword().toLower());
        /// this is not a flag, probably a typo in CSS file
        if (it == m_data.m_flags.end())
          return false;
        const FlagNames::FlagData & d = it->second;
        if (!d.flags)
          continue;
        if (d.linkIdx < 0)
          return false;

        const FlagNames::Alias & a = m_data.m_aliases[d.linkIdx];
        if (a.sources.empty()) {
          expanded[&m_aliases[d.linkIdx]] = 1;
        } else {
          for (uint32_t idx: a.sources)
            expanded[&m_aliases[idx]] = 1;
        }
      }

      return true;
    }

  private:
    friend class FlagAliasT<T>;

    QString stringify(Flags value) const
    {
      // All matching flags sorted by their popcount and their location in the
      // original array to optimize the string representation.
      // For example motion-x and motion-y will become motion-xy, since motion-xy
      // has a higher popcount than motion-x / -y
      std::multimap<int, std::pair<QByteArray, uint64_t>> flags;

      uint64_t v = value.asInt();

      int i = 0;
      for (auto it = m_data.m_flags.begin(); it != m_data.m_flags.end(); ++it, ++i) {
        if ((v & it->second.flags) == it->second.flags) {
          std::bitset<sizeof(T)*8> bitset(it->second.flags);
          flags.insert(std::make_pair(i-1024*int(bitset.count()), std::make_pair(it->first, it->second.flags)));
        }
      }

      QStringList flagLst;
      for (auto p: flags) {
        uint64_t v2 = p.second.second;
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
      Flags before = m_cache;
      m_cache = value(Layer(LAYER_COUNT - 1), Layer(0));
      Flags changedBits = before ^ m_cache;
      if (changedBits) {
        for (auto & alias: m_aliases)
          if (alias.m_flags & changedBits)
            alias.emitChange();
        emitChange();
      }
    }

    Flags m_cache;
    Flags m_values[LAYER_COUNT];
    Flags m_masks[LAYER_COUNT];
    const FlagNames & m_data;
    std::vector<FlagAliasT<T>> m_aliases;
  };
}

#endif // VALUABLE_VALUEFLAGS_HPP
