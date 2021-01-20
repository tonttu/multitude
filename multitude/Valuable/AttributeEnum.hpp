/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_ENUM_HPP
#define VALUABLE_VALUE_ENUM_HPP

#include "AttributeNumeric.hpp"
#include "AttributeFlags.hpp"

#include <boost/container/flat_map.hpp>

namespace Valuable
{
  struct EnumName
  {
    /// Name of the flag
    const char * name;
    /// Value of the value
    int value;
  };

  /// This struct is used to define the name strings for enum values so they
  /// can be referenced from CSS.
  struct EnumNames
  {
    EnumNames() = default;
    EnumNames(std::initializer_list<EnumName> l)
    {
      values.reserve(l.size());
      for (const EnumName & n: l) {
        if (!n.name)
          break;
        values[QByteArray(n.name).toLower()] = n.value;
      }
    }

    boost::container::flat_map<QByteArray, int> values;
  };

  class AttributeEnum
  {
  public:
    AttributeEnum(const EnumNames & names)
      : m_enumValues(names) {}
    ~AttributeEnum() {}

    void setAllowIntegers(bool allow)
    {
      m_allowIntegers = allow;
    }

    bool allowIntegers() const
    {
      return m_allowIntegers;
    }

    const EnumNames & enumValues() const { return m_enumValues; }

  protected:
    const EnumNames & m_enumValues;
    bool m_allowIntegers = false;
  };

  /**
   * Valuable enum. Similar to AttributeFlags, but only one value can be enabled
   * at a time.
   *
   * This class also supports pure integer values in addition to enum values.
   * In practice this means that you could write in CSS: "priority: low" or
   * "priority: 15".
   *
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
   *   enum {
   *     PriorityLow = 10,
   *     PriorityMedium = 50,
   *     PriorityHigh = 90
   *   };
   *
   *   /// m_mode is either ON or OFF
   *   AttributeT<Mode> m_mode;
   *
   *   /// m_priority is an integer, but has shortcuts low/medium/high
   *   AttributeT<int> m_priority;
   * };
   *
   * //////////////////////////////////////////////////////////////////////////
   *
   * /// FunnyWidget.cpp
   *
   * /// In CSS/Script you can use keywords "on" or "enabled / "off" or "disabled"
   * Valuable::EnumNames s_modes[] = {{"on", FunnyWidget::ON, "enabled", FunnyWidget::ON},
   *                                  {"off", FunnyWidget::OFF, "disabled", FunnyWidget::OFF},
   *                                  {0, 0}};
   *
   * Valuable::EnumNames s_priorities[] = {{"low", FunnyWidget::PriorityLow},
   *                                       {"medium", FunnyWidget::PriorityMedium},
   *                                       {"high", FunnyWidget::PriorityHigh},
   *                                       {0, 0}};
   *
   * FunnyWidget::FunnyWidget()
   *  : m_mode(this, "mode", s_modes, ON)
   *  , m_priority(this, "priority", s_priorities, PriorityMedium)
   * {
   *   m_priority.setAllowIntegers(true);
   * }
   *
   * @endcode
   */
  template <typename T>
  class AttributeT<T, typename std::enable_if<std::is_enum<T>::value>::type>
      : public AttributeBaseT<T>, public AttributeEnum
  {
    typedef AttributeBaseT<T> Base;

  public:
    using Base::operator =;
    using Base::value;

    AttributeT(Node * host, const QByteArray & name, const EnumNames & names,
                  const T & v)
      : Base(host, name, v),
        AttributeEnum(names)
    {
    }

    /// Converts the enum value to integer if integers are allowed
    virtual int asInt(bool * const ok = nullptr, Attribute::Layer layer = Attribute::CURRENT_VALUE) const OVERRIDE
    {
      if(ok) *ok = m_allowIntegers;
      return static_cast<int> (value(layer));
    }

    virtual bool set(int v, Attribute::Layer layer = Attribute::USER,
                            Attribute::ValueUnit = Attribute::VU_UNKNOWN) OVERRIDE
    {
      if (m_allowIntegers) {
        this->setValue(T(v), layer);
        return true;
      } else {
        for (auto & p: m_enumValues.values) {
          if (p.second == v) {
            this->setValue(T(v), layer);
            return true;
          }
        }
      }
      return false;
    }

    virtual bool set(const QString & v, Attribute::Layer layer = Attribute::USER,
                     Attribute::ValueUnit unit = Attribute::VU_UNKNOWN) override
    {
      if (unit != Attribute::VU_UNKNOWN)
        return false;

      auto it = m_enumValues.values.find(v.toLatin1().toLower());
      if (it == m_enumValues.values.end())
        return false;

      this->setValue(T(it->second), layer);
      return true;
    }

    virtual bool set(const StyleValue & v, Attribute::Layer layer = Attribute::USER) OVERRIDE
    {
      if (v.size() != 1)
        return false;

      if (v.unit(0) != Attribute::VU_UNKNOWN)
        return false;

      auto it = m_enumValues.values.find(v.asKeyword().toLower());
      if (it == m_enumValues.values.end())
        return false;

      this->setValue(T(it->second), layer);
      return true;
    }

    virtual void eventProcess(const QByteArray &, Radiant::BinaryData & data) OVERRIDE
    {
      QByteArray str;
      if (data.readString(str)) {
        auto it = m_enumValues.values.find(str.toLower());
        if (it != m_enumValues.values.end())
          this->setValue(T(it->second));
      }
    }

    virtual QString asString(bool * const ok = nullptr, Valuable::Attribute::Layer layer = Base::CURRENT_VALUE) const OVERRIDE
    {
      if (ok)
        *ok = true;

      T v = value(layer);
      for (auto & p: m_enumValues.values)
        if (p.second == (int)v)
          return QString::fromLatin1(p.first);
      return QString::number((int)v);
    }

    virtual bool deserialize(const ArchiveElement & element) override
    {
      QString tmp = element.get();
      bool ok = false;
      int num = tmp.toInt(&ok);
      if (ok) {
        return set(num);
      } else {
        return set(tmp);
      }
    }

    virtual QByteArray type() const override
    {
      return "enum:" + Radiant::StringUtils::type<T>();
    }

    static inline T interpolate(T a, T b, float m)
    {
      return m >= 0.5f ? b : a;
    }
  };

  /// @todo is this really needed? if so, also implement with flipped parameters
  template <typename T>
  inline bool operator==(const AttributeT<T, typename std::enable_if<std::is_enum<T>::value>::type> & a, T e)
  {
    return *a == e;
  }
}

#endif // VALUEENUM_HPP
