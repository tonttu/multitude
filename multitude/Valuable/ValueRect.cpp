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

#include "ValueRect.hpp"
#include "DOMElement.hpp"

#include <Radiant/StringUtils.hpp>

namespace Valuable
{

  ValueRect::ValueRect(HasValues * parent, const QString & name, const Nimble::Rect & r, bool transit)
    : Base(parent, name, r, transit)
  {}

  bool ValueRect::deserialize(ArchiveElement & element) {
    std::stringstream in(element.get().toUtf8().data());

    Nimble::Vector2f lo, hi;

    in >> lo[0];
    in >> lo[1];
    in >> hi[0];
    in >> hi[1];

    *this = Nimble::Rect(lo, hi);
    return true;
  }

  QString ValueRect::asString(bool * const ok) const {
    if(ok) *ok = true;

    const Nimble::Rect & rect = value();
    const Nimble::Vector2f & lo = rect.low();
    const Nimble::Vector2f & hi = rect.high();

    QString r = Radiant::StringUtils::stringify(lo[0]);
    r += QString(" ") + Radiant::StringUtils::stringify(lo[1]);
    r += QString(" ") + Radiant::StringUtils::stringify(hi[0]);
    r += QString(" ") + Radiant::StringUtils::stringify(hi[1]);

    return r;
  }
/*
  bool ValueRect::set(const Nimble::Vector4f & v)
  {
    m_color = v;
    return true;
  }
*/
}

