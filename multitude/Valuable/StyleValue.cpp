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

  StyleValue::StyleValue(QVariant v, Attribute::ValueUnit unit) : m_uniform(true)
  {
    m_values << v;
    m_units << unit;
    m_separators << WhiteSpace;
  }

  StyleValue::StyleValue(const QMap<QString, QString> & map)
  {
    for (auto it = map.begin(); it != map.end(); ++it) {
      if (m_values.size() == 0)
        m_separators << WhiteSpace << WhiteSpace;
      else
        m_separators << Comma << WhiteSpace;
      m_values << it.key() << it.value();
      m_units << Attribute::VU_UNKNOWN << Attribute::VU_UNKNOWN;
    }
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
      return Radiant::Color(color.redF(), color.greenF(), color.blueF(), color.alphaF());
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
    assert(v.size() >= 1);

    if (m_values.isEmpty()) {
      m_uniform = v.m_uniform;
      m_values = v.m_values;
      m_units = v.m_units;
      m_separators = v.m_separators;
      m_separators[0] = sep;
      return;
    }

    const QVariant & v1 = v.m_values[0];
    if(m_uniform && v.m_uniform) {
      const QVariant & v2 = m_values.last();
      m_uniform = v2.canConvert(v1.type()) && (isNumber(m_values.size() - 1) == v.isNumber());
    } else {
      m_uniform = false;
    }
    for (int i = 0; i < v.m_values.size(); ++i) {
      m_values << v.m_values[i];
      m_units << v.m_units[i];
      m_separators << (i == 0 ? sep : v.m_separators[i]);
    }
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
      const QVariant v = m_values[i];
      const int unit = m_units[i];
      const Separator separator = m_separators[i];

      QString unitstr;
      if(unit == Attribute::VU_PXS)
        unitstr = "px";
      else if(unit == Attribute::VU_EMS)
        unitstr = "em";
      else if(unit == Attribute::VU_EXS)
        unitstr = "ex";

      QVariant::Type t = v.type();
      if(unit == Attribute::VU_PERCENTAGE) {
        out << QString::number(v.toDouble() * 100.0) + "%";
      } else if(t == QVariant::Bool || t == QVariant::Int || t == QVariant::Double || int(t) == QMetaType::Float) {
        out << v.toString() + unitstr;
      } else if(t == QVariant::ByteArray) {
        out << v.toByteArray();
      } else if(t == QVariant::String) {
        out << "\"" + v.toString() + "\"";
      } else if(t == QVariant::Color) {
        QColor c = v.value<QColor>();
        out << QString("#%1%2%3%4").arg(c.red(), 2, 16, QLatin1Char('0')).
               arg(c.green(), 2, 16, QLatin1Char('0')).
               arg(c.blue(), 2, 16, QLatin1Char('0')).
               arg(c.alpha(), 2, 16, QLatin1Char('0'));
      } else {
        Radiant::error("StyleValue::stringify # Unknown variant type %d (%s)", t, v.typeName());
        continue;
      }
      if (separator == Comma) {
        out << ",";
      } else if (separator == Slash) {
        out << "/";
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
        g.separators << m_separators[i];
        all << g;
      } else {
        all.back().units << m_units[i];
        all.back().values << m_values[i];
        all.back().separators << m_separators[i];
      }
    }
    return all;
  }

  QMap<QString, QString> StyleValue::asMap() const
  {
    QMap<QString, QString> map;
    for (auto group: groups(Comma)) {
      QStringList tmp;
      for (auto v: group.values.mid(1))
        tmp << v.toString();
      map[group.values[0].toString()] = tmp.join(" ");
    }
    return map;
  }

  std::ostream & operator<<(std::ostream & os, const StyleValue & value)
  {
    return os << value.stringify().toUtf8().data();
  }
}
