/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "AttributeStringList.hpp"

#include "StyleValue.hpp"

namespace Valuable
{
  AttributeStringList::AttributeT()
  {
  }

  AttributeStringList::AttributeT(Node * host, const QByteArray & name,
                                  const QStringList & v)
    : Base(host, name, v)
  {
  }

  QString AttributeStringList::asString(bool * const ok, Layer layer) const
  {
    if (ok) *ok = true;
    return value(layer).join(" ");
  }

  bool AttributeStringList::set(const QString & v, Attribute::Layer layer, Attribute::ValueUnit)
  {
    if (v.isEmpty())
      setValue(QStringList(), layer);
    else
      setValue(QStringList(v), layer);
    return true;
  }

  bool AttributeStringList::set(const StyleValue & v, Attribute::Layer layer)
  {
    if (v.isUniform() && v[0].canConvert(StyleValue::TYPE_STRING)) {
      QStringList lst;
      lst.reserve(v.size());
      for (const auto & var : v.components())
        lst << var.asString();
      setValue(lst, layer);
      return true;
    }
    return false;
  }

  QByteArray AttributeStringList::type() const
  {
    return "list:string";
  }
} // namespace Valuable
