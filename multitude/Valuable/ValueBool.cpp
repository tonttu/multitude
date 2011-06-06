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

#include "ValueBool.hpp"
#include "DOMElement.hpp"
#include "Radiant/StringUtils.hpp"

namespace Valuable
{
  ValueBool::ValueBool(HasValues * parent, const QString & name,
                       bool value, bool transit)
    : ValueObjectT<bool>(parent, name, value, transit)
  {}

  ValueBool::~ValueBool() {}

  bool ValueBool::deserialize(ArchiveElement & e)
  {
    *this = Radiant::StringUtils::fromString<int32_t>(e.get().toUtf8().data()) != 0;
    return true;
  }

  void ValueBool::processMessage(const char *, Radiant::BinaryData & data)
  {
    bool ok = true;
    int32_t v = data.readInt32(&ok);
    if(ok) *this = (v != 0);
  }

  QString ValueBool::asString(bool * const ok) const
  {
    if(ok) *ok = true;
    return Radiant::StringUtils::stringify((int32_t)value());
  }

  bool ValueBool::set(int value, Layer layer)
  {
    setValue(!!value, layer);
    return true;
  }
}
