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

#ifndef HASVALUES_HASVALUES_IMPL_HPP
#define HASVALUES_HASVALUES_IMPL_HPP

#include <Valuable/HasValues.hpp>

#include <Radiant/Trace.hpp>

namespace Valuable
{

  template<class Type>
      bool HasValues::setValue(const std::string & name, const Type & v)
  {
    size_t cut = name.find("/");
    std::string next = name.substr(0, cut);
    std::string rest = name.substr(cut + 1);

    if(next == std::string("..")) {
      if(!m_parent) {
        Radiant::error(
            "HasValues::setValue # node '%s' has no parent", m_name.c_str());
        return false;
      }

      return m_parent->setValue(rest, v);
    }

    container::iterator it = m_children.find(next);
    if(it == m_children.end()) {
      Radiant::error(
          "HasValues::setValue # property '%s' not found", next.c_str());
      return false;
    }

    HasValues * hv = dynamic_cast<HasValues *> (it->second);
    if(hv)
      return hv->setValue(rest, v);
    else {
      ValueObject * vo = it->second;
      return vo->set(v);
    }
  }

}

#endif
