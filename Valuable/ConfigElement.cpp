/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
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

  ConfigValue * ConfigElement::getConfigValue(const QString & key)
  {
    for(int i = 0; i < (int) m_values.size(); i++) {

      if(m_values[i].key() == key) {
        return & m_values[i];
        break;
      }
    }

    return 0;
  }

  ConfigValue ConfigElement::getConfigValueSafe(const QString & key)
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
