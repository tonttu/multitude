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

#ifndef VALUABLE_VALUE_INT_IMPL_HPP
#define VALUABLE_VALUE_INT_IMPL_HPP

#include <Valuable/ValueInt.hpp>
#include <Valuable/DOMElement.hpp>

#include <Radiant/StringUtils.hpp>

namespace Valuable
{

  template<class T>
  bool ValueIntT<T>::deserialize(const ArchiveElement & e)
  {
    Base::m_value = Radiant::StringUtils::fromString<T>(e.get().c_str());
    this->emitChange();
    
    return true;
  }
  /*
template<>
  bool ValueIntT<Radiant::TimeStamp>::deserialize(const ArchiveElement & )
  {
    Radiant::error("ValueIntT<Radiant::TimeStamp>::deserialize # not implemented!");
    return false;
  }

  Default implementation is fine also for TimeStamps.
  */
}

#endif
