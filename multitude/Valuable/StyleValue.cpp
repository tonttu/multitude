/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#include "StyleValue.hpp"

#include <Radiant/Trace.hpp>

#include <QtGui/QColor>

#include <typeinfo>

#include <cassert>

namespace Valuable
{
  bool canConvertType(StyleValue::ValueType a, StyleValue::ValueType b)
  {
    if (a == b)
      return true;

    if (a > b)
      std::swap(a, b);

    switch (a) {
    case StyleValue::TYPE_FLOAT:
      return b == StyleValue::TYPE_INT;
    case StyleValue::TYPE_STRING:
      return b == StyleValue::TYPE_KEYWORD;
    default:
      return false;
    }
  }

  StyleValue::Component::Component()
    : m_unit(Attribute::VU_UNKNOWN)
    , m_separator(StyleValue::SEPARATOR_WHITE_SPACE)
  {}

  StyleValue::Component::Component(float f, Attribute::ValueUnit unit)
    : m_data(f)
    , m_unit(unit)
    , m_separator(StyleValue::SEPARATOR_WHITE_SPACE)
  {
  }

  StyleValue::Component::Component(int i)
    : m_data(i)
    , m_unit(Attribute::VU_UNKNOWN)
    , m_separator(StyleValue::SEPARATOR_WHITE_SPACE)
  {
  }

  StyleValue::Component::Component(const Radiant::Color & color)
    : m_data(color)
    , m_unit(Attribute::VU_UNKNOWN)
    , m_separator(StyleValue::SEPARATOR_WHITE_SPACE)
  {
  }

  StyleValue::Component::Component(const Radiant::ColorPMA & color)
    : m_data(color)
    , m_unit(Attribute::VU_UNKNOWN)
    , m_separator(StyleValue::SEPARATOR_WHITE_SPACE)
  {
  }

  StyleValue::Component::Component(const QString & string)
    : m_data(string)
    , m_unit(Attribute::VU_UNKNOWN)
    , m_separator(StyleValue::SEPARATOR_WHITE_SPACE)
  {
  }

  StyleValue::Component::Component(const QByteArray & keyword)
    : m_data(keyword)
    , m_unit(Attribute::VU_UNKNOWN)
    , m_separator(StyleValue::SEPARATOR_WHITE_SPACE)
  {
  }

  StyleValue::Component::Component(const SimpleExpression & expr)
    : m_data(expr)
    , m_unit(Attribute::VU_UNKNOWN)
    , m_separator(StyleValue::SEPARATOR_WHITE_SPACE)
  {
  }

  StyleValue::Component::~Component()
  {
  }

  StyleValue::Component::Component(const Component & component)
    : m_data(component.m_data)
    , m_unit(component.m_unit)
    , m_separator(component.m_separator)
  {
  }

  StyleValue::Component & StyleValue::Component::operator=(const Component & component)
  {
    m_data = component.m_data;
    m_unit = component.m_unit;
    m_separator = component.m_separator;
    return *this;
  }

  StyleValue::Component::Component(Component && component)
    : m_data(std::move(component.m_data))
    , m_unit(component.m_unit)
    , m_separator(component.m_separator)
  {
  }

  StyleValue::Component & StyleValue::Component::operator=(Component && component)
  {
    m_data = std::move(component.m_data);
    m_unit = component.m_unit;
    m_separator = component.m_separator;
    return *this;
  }

  int StyleValue::Component::asInt() const
  {
    switch (type()) {
    case TYPE_INT:
      return std::get<int>(m_data);
    case TYPE_FLOAT:
      return std::get<float>(m_data);
    default:
      Radiant::error("StyleValue::Component::asInt # cannot convert %s to int", typeName());
      return 0;
    }
  }

  float StyleValue::Component::asFloat() const
  {
    switch (type()) {
    case TYPE_FLOAT:
      return std::get<float>(m_data);
    case TYPE_INT:
      return std::get<int>(m_data);
    default:
      Radiant::error("StyleValue::Component::asFloat # cannot convert %s to float", typeName());
      return 0.f;
    }
  }

  QString StyleValue::Component::asString() const
  {
    switch (type()) {
    case TYPE_STRING:
      return std::get<QString>(m_data);
    case TYPE_KEYWORD:
      return QString::fromUtf8(std::get<QByteArray>(m_data));
    default:
      Radiant::error("StyleValue::Component::asString # cannot convert %s to string", typeName());
      return QString();
    }
  }

  QByteArray StyleValue::Component::asKeyword() const
  {
    switch (type()) {
    case TYPE_KEYWORD:
      return std::get<QByteArray>(m_data);
    case TYPE_STRING:
      return std::get<QString>(m_data).toUtf8();
    default:
      Radiant::error("StyleValue::Component::asKeyword # cannot convert %s to keyword", typeName());
      return QByteArray();
    }
  }

  Radiant::Color StyleValue::Component::asColor() const
  {
    switch (type()) {
    case TYPE_COLOR:
      return std::get<Radiant::Color>(m_data);
    case TYPE_COLOR_PMA:
      return std::get<Radiant::ColorPMA>(m_data).toColor();
    case TYPE_KEYWORD:
    {
      Radiant::Color color;
      if (color.set(std::get<QByteArray>(m_data)))
        return color;
      break;
    }
    default:
      break;
    }
    Radiant::error("StyleValue::Component::asColor # cannot convert %s to color", typeName());
    return Radiant::Color();
  }

  Radiant::ColorPMA StyleValue::Component::asColorPMA() const
  {
    switch (type()) {
    case TYPE_COLOR_PMA:
      return std::get<Radiant::ColorPMA>(m_data);
    case TYPE_COLOR:
      return std::get<Radiant::Color>(m_data);
    case TYPE_KEYWORD:
    {
      Radiant::Color color;
      if (color.set(std::get<QByteArray>(m_data))) {
        return color;
      }
    }
    default:
      break;
    }
    Radiant::error("StyleValue::Component::asColorPMA # cannot convert %s to color", typeName());
    return Radiant::Color();
  }

  SimpleExpression StyleValue::Component::asExpr() const
  {
    if (type() == TYPE_EXPR)
      return std::get<SimpleExpression>(m_data);
    Radiant::error("StyleValue::Component::asExpr # cannot convert %s to expr", typeName());
    return SimpleExpression(0.0f);
  }

  bool StyleValue::Component::canConvert(StyleValue::ValueType t) const
  {
    return canConvertType(type(), t);
  }

  bool StyleValue::Component::isNumber() const
  {
    auto t = type();
    return t == TYPE_FLOAT || t == TYPE_INT;
  }

  const char * StyleValue::Component::typeName() const
  {
    switch (type()) {
    case TYPE_FLOAT:
      return "float";
    case TYPE_INT:
      return "int";
    case TYPE_COLOR:
      return "color";
    case TYPE_COLOR_PMA:
      return "color-pma";
    case TYPE_STRING:
      return "string";
    case TYPE_KEYWORD:
      return "keyword";
    case TYPE_EXPR:
      return "expr";
    default:
      return "none";
    }
  }

  bool StyleValue::Component::operator==(const Component & v) const
  {
    return m_data == v.m_data && m_unit == v.m_unit && m_separator == v.m_separator;
  }

  bool StyleValue::Component::operator==(const QByteArray & str) const
  {
    switch (type()) {
    case TYPE_STRING:
      return std::get<QString>(m_data).toUtf8() == str;
    case TYPE_KEYWORD:
      return std::get<QByteArray>(m_data) == str;
    default:
      return false;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  StyleValue::StyleValue(float v, Attribute::ValueUnit unit)
  {
    m_components << Component(v, unit);
  }

  StyleValue::StyleValue(int v)
  {
    m_components << v;
  }

  StyleValue::StyleValue(const Radiant::Color & color)
  {
    m_components << color;
  }

  StyleValue::StyleValue(const Radiant::ColorPMA & color)
  {
    m_components << color;
  }

  StyleValue::StyleValue(const QString & string)
  {
    m_components << string;
  }

  StyleValue::StyleValue(const QByteArray & keyword)
  {
    m_components << keyword;
  }

  StyleValue::StyleValue(const Component & component)
  {
    m_components << component;
  }

  StyleValue::StyleValue(const QMap<QString, QString> & map)
  {
    for (auto it = map.begin(); it != map.end(); ++it) {
      Component key(it.key().toUtf8());
      Component value(it.value());
      if (m_components.size() > 0)
        key.setSeparator(SEPARATOR_COMMA);
      append(key);
      append(value);
    }
  }

  StyleValue::StyleValue(const SimpleExpression & expr)
  {
    m_components << expr;
  }

  StyleValue::~StyleValue()
  {}

  int StyleValue::asInt(int idx) const
  {
    return m_components[idx].asInt();
  }

  float StyleValue::asFloat(int idx) const
  {
    return m_components[idx].asFloat();
  }

  QString StyleValue::asString(int idx) const
  {
    return m_components[idx].asString();
  }

  QByteArray StyleValue::asKeyword(int idx) const
  {
    return m_components[idx].asKeyword();
  }

  Radiant::Color StyleValue::asColor(int idx) const
  {
    return m_components[idx].asColor();
  }

  Radiant::ColorPMA StyleValue::asColorPMA(int idx) const
  {
    return m_components[idx].asColorPMA();
  }

  SimpleExpression StyleValue::asExpr(int idx) const
  {
    return m_components[idx].asExpr();
  }

  StyleValue::ValueType StyleValue::type(int idx) const
  {
    return m_components[idx].type();
  }

  Attribute::ValueUnit StyleValue::unit(int idx) const
  {
    return m_components[idx].unit();
  }

  void StyleValue::append(StyleValue v)
  {
    assert(v.size() >= 1);

    if (m_components.isEmpty()) {
      m_isUniform = v.m_isUniform;
      m_components = std::move(v.m_components);
      return;
    }

    for (const auto & c: v.m_components)
      append(c);
  }

  void StyleValue::append(StyleValue styleValue, StyleValue::Separator separator)
  {
    styleValue.m_components[0].setSeparator(separator);
    append(std::move(styleValue));
  }

  void StyleValue::append(const StyleValue::Component & c)
  {
    if (m_isUniform && m_components.size() > 0) {
      m_isUniform = (m_components.size() == 1 || m_components.back().separator() == c.separator()) &&
          canConvertType(m_components.back().type(), c.type());
    }
    m_components << c;
  }

  void StyleValue::append(const StyleValue::Component & c, StyleValue::Separator separator)
  {
    StyleValue::Component component(c);
    component.setSeparator(separator);
    append(component);
  }

  QString StyleValue::stringify() const
  {
    QString out;

    for(int i = 0, s = m_components.size(); i < s; ++i) {
      const auto & v = m_components[i];
      const int unit = v.unit();
      if (i > 0) {
        const Separator separator = v.separator();
        if (separator == SEPARATOR_COMMA) {
          out += ", ";
        } else if (separator == SEPARATOR_SLASH) {
          out += " / ";
        } else {
          out += " ";
        }
      }


      QString unitstr;
      if(unit == Attribute::VU_PXS)
        unitstr = "px";
      else if(unit == Attribute::VU_EMS)
        unitstr = "em";
      else if(unit == Attribute::VU_EXS)
        unitstr = "ex";

      auto t = v.type();
      if(unit == Attribute::VU_PERCENTAGE) {
        out += QString("%1%").arg(v.asFloat() * 100.0);
      } else if(t == TYPE_INT || t == TYPE_FLOAT) {
        out += QString::number(v.asFloat()) + unitstr;
      } else if(t == TYPE_KEYWORD) {
        out += v.asKeyword();
      } else if(t == TYPE_STRING) {
        out += "\"" + v.asString() + "\"";
      } else if(t == TYPE_COLOR) {
        out += v.asColor().toHex();
      } else if(t == TYPE_COLOR_PMA) {
        out += QString("%1 %2 %3 %4").arg(v.asColorPMA().r).arg(v.asColorPMA().g).
               arg(v.asColorPMA().b).arg(v.asColorPMA().a);
      } else if (t == TYPE_EXPR) {
        out += QString::fromUtf8("calc("+v.asExpr().toString()+")");
      } else {
        Radiant::error("StyleValue::stringify # Unknown component type %d (%s)", t, v.typeName());
        continue;
      }
    }

    return out;
  }

  bool StyleValue::isNumber(int idx) const
  {
    auto t = type(idx);
    return t == TYPE_INT || t == TYPE_FLOAT;
  }

  const StyleValue::Component & StyleValue::operator[](int idx) const
  {
    return m_components[idx];
  }

  bool StyleValue::operator==(const StyleValue & v) const
  {
    return m_isUniform == v.m_isUniform &&
        m_components == v.m_components;
  }

  QList<StyleValue> StyleValue::split(Separator sep) const
  {
    QList<StyleValue> all;
    for (int i = 0; i < m_components.size(); ++i) {
      const Component & c = m_components[i];
      if (i == 0 || c.separator() == sep) {
        all << c;
      } else {
        all.back().append(c);
      }
    }
    return all;
  }

  QMap<QString, QString> StyleValue::asMap() const
  {
    QMap<QString, QString> map;
    for (auto group: split(SEPARATOR_COMMA)) {
      QStringList tmp;
      for (auto v: group.components().mid(1))
        tmp << v.asString();
      map[group[0].asString()] = tmp.join(" ");
    }
    return map;
  }

  QList<Attribute::ValueUnit> StyleValue::units() const
  {
    QList<Attribute::ValueUnit> tmp;
    for (auto & c: m_components)
      tmp << c.unit();
    return tmp;
  }

  std::ostream & operator<<(std::ostream & os, const StyleValue & value)
  {
    return os << value.stringify().toUtf8().data();
  }

}
