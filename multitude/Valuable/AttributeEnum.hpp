/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_ENUM_HPP
#define VALUABLE_VALUE_ENUM_HPP

#include "AttributeNumeric.hpp"
#include "AttributeFlags.hpp"

namespace Valuable
{
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
   *   AttributeEnumT<Mode> m_mode;
   *
   *   /// m_priority is an integer, but has shortcuts low/medium/high
   *   AttributeEnumT<int> m_priority;
   * };
   *
   * //////////////////////////////////////////////////////////////////////////
   *
   * /// FunnyWidget.cpp
   *
   * /// In CSS/Script you can use keywords "on" or "enabled / "off" or "disabled"
   * Valuable::Flags s_modes[] = {"on", FunnyWidget::ON, "enabled", FunnyWidget::ON,
   *                              "off", FunnyWidget::OFF, "disabled", FunnyWidget::OFF,
   *                              0, 0};
   *
   * Valuable::Flags s_priorities[] = {"low", FunnyWidget::PriorityLow,
   *                                   "medium", FunnyWidget::PriorityMedium,
   *                                   "high", FunnyWidget::PriorityHigh,
   *                                    0, 0};
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
  // We can't inherit from AttributeIntT, since T might be an enum
  template <typename T>
  class AttributeEnumT : public AttributeNumeric<T>
  {
    typedef AttributeNumeric<T> Base;

  public:
    using Base::operator =;

    AttributeEnumT(Node * host, const QByteArray & name, const FlagNames * names,
                  const T & v, bool transit = false)
      : AttributeNumeric<T>(host, name, v, transit)
      , m_allowIntegers(false)
    {
      for (const FlagNames * it = names; it->name; ++it) {
        m_enumValues[QString(it->name).toLower()] = T(it->value);
      }
    }

    virtual bool set(int v, Attribute::Layer layer = Attribute::USER,
                            Attribute::ValueUnit = Attribute::VU_UNKNOWN) OVERRIDE
    {
      if (m_allowIntegers)
        this->setValue(T(v), layer);
      return m_allowIntegers;
    }

    virtual bool set(const StyleValue & v, Attribute::Layer layer = Attribute::USER) OVERRIDE
    {
      if (v.size() != 1)
        return false;

      if (v.unit(0) != Attribute::VU_UNKNOWN)
        return false;

      auto it = m_enumValues.find(v.asString().toLower());
      if (it == m_enumValues.end())
        return false;

      this->setValue(*it, layer);
      return true;
    }

    virtual void processMessage(const QByteArray &, Radiant::BinaryData & data) OVERRIDE
    {
      QString str;
      if (data.readString(str)) {
        auto it = m_enumValues.find(str.toLower());
        if (it != m_enumValues.end())
          this->setValue(*it);
      }
    }

    void setAllowIntegers(bool allow)
    {
      m_allowIntegers = allow;
    }

  private:
    QMap<QString, T> m_enumValues;
    bool m_allowIntegers;
  };

}

#endif // VALUEENUM_HPP
