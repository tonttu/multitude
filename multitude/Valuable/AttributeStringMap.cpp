/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "AttributeStringMap.hpp"

#include "StyleValue.hpp"

namespace Valuable
{
  AttributeStringMap::AttributeT()
  {
  }

  AttributeStringMap::AttributeT(Node * host, const QByteArray & name,
                                 const StringMap & v)
    : Base(host, name, v)
  {
  }

  QString AttributeStringMap::asString(bool * const ok, Layer layer) const
  {
    if (ok) *ok = true;
    const QMap<QString, QString> &map = value(layer);
    QStringList ret;
    for (auto it = map.begin(); it != map.end(); ++it)
      ret << QString("\"%1\" \"%2\"").arg(it.key(), it.value());
    return ret.join(",");
  }

  bool AttributeStringMap::set(const QString & v, Attribute::Layer layer, Attribute::ValueUnit)
  {
    if (v.isEmpty()) {
      setValue(StringMap(), layer);
    } else {
      QStringList strs = v.split(",", QString::SkipEmptyParts);
      StringMap map;
      for(auto it = strs.constBegin(); it != strs.constEnd(); ++it) {
        QString trimmed = it->trimmed();
        int keyLen = trimmed.indexOf(" ");
        int valueBeg = keyLen;

        while(valueBeg < trimmed.length() && trimmed[valueBeg].isSpace())
          ++valueBeg;

        if(keyLen == 0)
          continue;
        else {
          map[trimmed.mid(0, keyLen)] = trimmed.mid(valueBeg);
        }
      }
      setValue(map, layer);
    }
    return true;
  }

  bool AttributeStringMap::set(const StyleValue & v, Attribute::Layer layer)
  {
    setValue(v.asMap(), layer);
    return true;
  }

  QByteArray AttributeStringMap::type() const
  {
    return "stringmap";
  }
} // namespace Valuable
