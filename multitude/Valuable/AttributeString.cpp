/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "AttributeString.hpp"

#include "StyleValue.hpp"

namespace Valuable
{
  AttributeString::AttributeT() {}
  AttributeString::AttributeT(Node * parent, const QByteArray & name,
                              const QString & v)
    : Base(parent, name, v)
  {}

  void AttributeString::eventProcess(const QByteArray & /*id*/, Radiant::BinaryData & data)
  {
    bool ok = true;
    QString tmp = data.read<QString>(&ok);
    if(ok)
      *this = tmp;
  }

  float AttributeString::asFloat(bool * const ok, Layer layer) const
  {
    return value(layer).toFloat(ok);
  }

  int AttributeString::asInt(bool * const ok, Layer layer) const
  {
    return value(layer).toInt(ok, 0);
  }

  QString AttributeString::asString(bool * const ok, Layer layer) const
  {
    if(ok) *ok = true;
    return value(layer);
  }

  bool AttributeString::set(const QString & v, Layer layer, ValueUnit)
  {
    setValue(v, layer);
    return true;
  }

  bool AttributeString::set(const StyleValue & value, Layer layer)
  {
    if (value.size() == 0) {
      setValue(QString(), layer);
      return true;
    } else if (value.size() > 1 || !value[0].canConvert(StyleValue::TYPE_STRING)) {
      return false;
    }
    setValue(value.asString(), layer);
    return true;
  }

  QString AttributeString::operator+(const AttributeString & i) const
  {
    return value() + i.value();
  }

  QString AttributeString::operator+(const QString & i) const
  {
    return value() + i;
  }

  QString AttributeString::operator+(const char * utf8) const
  {
    return value() + QString::fromUtf8(utf8);
  }

  bool AttributeString::operator == (const QString & that) const
  {
    return value() == that;
  }

  bool AttributeString::operator != (const QString & that) const
  {
    return value() != that;
  }

  void AttributeString::clear()
  {
    *this = "";
  }

  unsigned AttributeString::size() const
  {
    return (unsigned) value().size();
  }
}

QString operator + (const QString & a, const Valuable::AttributeString & b)
{
  return a + *b;
}
