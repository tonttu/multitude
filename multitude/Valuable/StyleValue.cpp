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
#include "StyleValue.hpp"

#include <Radiant/Trace.hpp>

#include <QtGui/QColor>

#include <typeinfo>

#include <cassert>

namespace Valuable
{

  StyleValue::StyleValue() : m_uniform(true)
  {}

  StyleValue::StyleValue(float v, Attribute::ValueUnit unit) : m_uniform(true)
  {
    m_values << v;
    m_units << unit;
    m_separators << WhiteSpace;
  }

  StyleValue::StyleValue(int v) : m_uniform(true)
  {
    m_values << v;
    m_units << Attribute::VU_UNKNOWN;
    m_separators << WhiteSpace;
  }

  StyleValue::StyleValue(QVariant v) : m_uniform(true)
  {
    m_values << v;
    m_units << Attribute::VU_UNKNOWN;
    m_separators << WhiteSpace;
  }

  StyleValue::~StyleValue()
  {}

  int StyleValue::asInt(int idx) const
  {
    bool ok;
    assert(m_values.size() > idx && idx >= 0);
    int ret = m_values[idx].toInt(&ok);
    /// @todo should not allow implicit type casting between numeric and string values
    if(ok) return ret;

    Radiant::error("StyleValue::asInt # cannot convert %s to int", m_values[idx].typeName());
    return 0;
  }

  float StyleValue::asFloat(int idx) const
  {
    bool ok;
    assert(m_values.size() > idx && idx >= 0);
    float ret = m_values[idx].toFloat(&ok);
    if(ok) return ret;

    Radiant::error("StyleValue::asFloat # cannot convert %s to float", m_values[idx].typeName());
    return 0.f;
  }

  QString StyleValue::asString(int idx) const
  {
    assert(m_values.size() > idx && idx >= 0);
    return m_values[idx].toString();

    /// @todo
    //Radiant::error("StyleValue::asString # cannot convert %s to string", m_values[idx].typeName());
    //return "";
  }

  Radiant::Color StyleValue::asColor(int idx) const
  {
    assert(m_values.size() > idx && idx >= 0);
    if(m_values[idx].type() == QVariant::Color) {
      QColor color = m_values[idx].value<QColor>();
      return Radiant::Color(color.red(), color.green(), color.blue(), color.alpha());
    }

    Radiant::error("StyleValue::asColor # cannot convert %s to color", m_values[idx].typeName());
    return Radiant::Color(0, 0, 0, 0);
  }

  int StyleValue::type(int idx) const
  {
    assert(m_values.size() > idx && idx >= 0);
    return m_values[idx].type();
  }

  Attribute::ValueUnit StyleValue::unit(int idx) const
  {
    assert(m_values.size() > idx && idx >= 0);
    return m_units[idx];
  }

  void StyleValue::append(const StyleValue & v, Separator sep)
  {
    assert(v.size() == 1);
    const QVariant & v1 = v.m_values[0];
    if(m_uniform) {
      const QVariant & v2 = m_values.last();
      m_uniform = v2.canConvert(v1.type()) && (isNumber(m_values.size() - 1) == v.isNumber());
    }
    m_values << v1;
    m_units << v.m_units[0];
    m_separators << sep;
  }

  int StyleValue::size() const
  {
    return m_values.size();
  }

  bool StyleValue::uniform() const
  {
    return m_uniform;
  }

  QString StyleValue::stringify() const
  {
    QStringList out;

    for(int i = 0, s = m_values.size(); i < s; ++i) {
      QVariant v = m_values[i];
      int unit = m_units[i];
      QString unitstr;
      if(unit == Attribute::VU_PXS)
        unitstr = "px";
      else if(unit == Attribute::VU_EMS)
        unitstr = "em";
      else if(unit == Attribute::VU_EXS)
        unitstr = "ex";

      QVariant::Type t = v.type();
      if(unit == Attribute::VU_PERCENTAGE) {
        out << QString::number(v.toDouble() * 0.01) + "%";
      } else if(t == QVariant::Bool || t == QVariant::Int || t == QVariant::Double) {
        out << v.toString() + unitstr;
      } else if(t == QVariant::ByteArray) {
        out << v.toByteArray();
      } else if(t == QVariant::String) {
        out << "\"" + v.toString() + "\"";
      } else if(t == QVariant::Color) {
        QColor c = v.value<QColor>();
        out << QString("#%1%2%3%4").arg(c.red(), 2, 10, QLatin1Char('0')).
               arg(c.green(), 2, 10, QLatin1Char('0')).
               arg(c.blue(), 2, 10, QLatin1Char('0')).
               arg(c.alpha(), 2, 10, QLatin1Char('0'));
      } else {
        Radiant::error("StyleValue::stringify # Unknown variant type %d (%s)", t, v.typeName());
      }
    }

    return out.join(" ");
  }

  bool StyleValue::isNumber(int idx) const
  {
    int t = type(idx);
    return t == QVariant::Int || t == QMetaType::Float;
  }

  StyleValue StyleValue::operator[](int idx) const
  {
    assert(m_values.size() > idx && idx >= 0);
    StyleValue v(m_values[idx]);
    v.m_units[0] = m_units[idx];
    return v;
  }

  bool StyleValue::operator==(const StyleValue & v) const
  {
    return m_uniform == v.m_uniform &&
        m_values == v.m_values &&
        m_units == v.m_units &&
        m_separators == v.m_separators;
  }

  QList<StyleValue::Group> StyleValue::groups(Separator sep) const
  {
    QList<Group> all;
    for (int i = 0; i < m_values.size(); ++i) {
      if (i == 0 || m_separators[i] == sep) {
        Group g;
        g.units << m_units[i];
        g.values << m_values[i];
        all << g;
      } else {
        all.back().units << m_units[i];
        all.back().values << m_values[i];
      }
    }
    return all;
  }
}
