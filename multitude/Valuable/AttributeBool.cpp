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

#include "AttributeBool.hpp"
#include "DOMElement.hpp"
#include "StyleValue.hpp"

#include <Radiant/StringUtils.hpp>

namespace Valuable
{
  AttributeBool::AttributeBool(Node * host, const QString & name,
                       bool value, bool transit)
    : AttributeT<bool>(host, name, value, transit)
  {}

  AttributeBool::~AttributeBool() {}

  bool AttributeBool::deserialize(const ArchiveElement & e)
  {
    *this = Radiant::StringUtils::fromString<int32_t>(e.get().toUtf8().data()) != 0;
    return true;
  }

  void AttributeBool::processMessage(const QString &, Radiant::BinaryData & data)
  {
    bool ok = true;
    int32_t v = data.readInt32(&ok);
    if(ok) *this = (v != 0);
  }

  int AttributeBool::asInt(bool * const ok) const
  {
    if(ok) *ok = true;
    return value();
  }

  QString AttributeBool::asString(bool * const ok) const
  {
    if(ok) *ok = true;
    return Radiant::StringUtils::stringify((int32_t)value());
  }

  bool AttributeBool::set(int value, Layer layer, ValueUnit)
  {
    setValue(!!value, layer);
    return true;
  }

  bool AttributeBool::set(const StyleValue & v, Layer layer)
  {
    if(v.size() == 1 && v.units()[0] == VU_UNKNOWN && v.values()[0].type() == QVariant::ByteArray) {
      QByteArray ba = v.values()[0].toByteArray().toLower();
      if(ba == "true") setValue(true, layer);
      else if(ba == "false") setValue(false, layer);
      else return false;
      return true;
    }
    return false;
  }
}
