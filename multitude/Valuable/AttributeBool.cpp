/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "AttributeBool.hpp"
#include "DOMElement.hpp"
#include "StyleValue.hpp"

#include <Radiant/StringUtils.hpp>

namespace Valuable
{
  AttributeBool::AttributeT(Node * host, const QByteArray & name,
                            bool value)
    : AttributeBaseT<bool>(host, name, value)
  {}

  AttributeBool::~AttributeT() {}

  void AttributeBool::eventProcess(const QByteArray &, Radiant::BinaryData & data)
  {
    bool ok = true;
    int32_t v = data.readInt32(&ok);
    if(ok) *this = (v != 0);
  }

  int AttributeBool::asInt(bool * const ok, Layer layer) const
  {
    if(ok) *ok = true;
    return value(layer);
  }

  QString AttributeBool::asString(bool * const ok, Layer layer) const
  {
    if(ok) *ok = true;
    return Radiant::StringUtils::toString(value(layer));
  }

  bool AttributeBool::set(int value, Layer layer, ValueUnit)
  {
    setValue(!!value, layer);
    return true;
  }

  bool AttributeBool::set(float v, Attribute::Layer layer, Attribute::ValueUnit)
  {
    setValue(!!v, layer);
    return true;
  }

  bool AttributeBool::set(const StyleValue & v, Layer layer)
  {
    if(v.size() == 1 && v.type() == StyleValue::TYPE_KEYWORD) {
      QByteArray ba = v.asKeyword().toLower();
      if(ba == "true") setValue(true, layer);
      else if(ba == "false") setValue(false, layer);
      else return false;
      return true;
    }
    return false;
  }

  float AttributeBool::asFloat(bool * const ok, Attribute::Layer layer) const
  {
    if(ok) *ok = true;
    return value(layer);
  }
}
