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

#include "ConfigElement.hpp"

namespace Valuable
{

  ConfigElement::ConfigElement(void)
  {
    m_depth = 0;
    m_type = "";
  }

  ConfigElement::~ConfigElement(void)
  {
  }

  ConfigValue * ConfigElement::getConfigValue(const std::string & key)
  {
    for(int i = 0; i < (int) m_values.size(); i++) {

      if(m_values[i].m_key == key) {
	return & m_values[i];
	break;
      }
    }

    return 0;
  }

  ConfigValue ConfigElement::getConfigValueSafe(const std::string & key)
  {
    ConfigValue * v = getConfigValue(key);
    if(v)
      return * v;
    return ConfigValue();
  }

  void ConfigElement::clear()
  {
    *this = ConfigElement();
  }

}
