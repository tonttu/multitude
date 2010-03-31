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

#include "ValueColor.hpp"
#include "DOMElement.hpp"

#include <Radiant/StringUtils.hpp>

namespace Valuable
{

  ValueColor::ValueColor(HasValues * parent, const std::string & name, const Radiant::Color & c, bool transit)
    : ValueVector<Nimble::Vector4f,float,4>(parent, name, c, transit)
  {}

  ValueColor::~ValueColor()
  {}
}

