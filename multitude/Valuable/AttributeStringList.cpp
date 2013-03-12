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
  AttributeStringList::AttributeStringList()
  {
  }

  AttributeStringList::AttributeStringList(Node * host, const QByteArray & name,
                                           const QStringList & v, bool transit)
    : Base(host, name, v, transit)
  {
  }

  QString AttributeStringList::asString(bool * const ok) const
  {
    if (ok) *ok = true;
    return value().join(" ");
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
    for (auto u : v.units())
      if (u != VU_UNKNOWN)
        return false;

    QStringList lst;
    lst.reserve(v.size());
    for (const QVariant & var : v.values())
      lst << var.toString();
    setValue(lst, layer);
    return true;
  }
} // namespace Valuable
