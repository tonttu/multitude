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

#ifndef VALUABLE_CONFIG_ATTRIBUTE_HPP
#define VALUABLE_CONFIG_ATTRIBUTE_HPP

#include <Valuable/Export.hpp>

#include <string>

namespace Valuable
{  
  class VALUABLE_API ConfigValue
  {
  public:
    ConfigValue(void);
    ConfigValue(std::string k, std::string v);
    ~ConfigValue(void);
    
    const std::string & key() const { return m_key; }
    const std::string & value() const { return m_value; }

    int m_depth;
    std::string m_key, m_value;
  };
}


#endif
