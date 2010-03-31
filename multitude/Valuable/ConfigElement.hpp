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

#ifndef VALUABLE_ELEMENT_HPP
#define VALUABLE_ELEMENT_HPP

#include <Valuable/Export.hpp>
#include <Valuable/ConfigValue.hpp>

#include <iostream>
#include <vector>
#include <string>

namespace Valuable
{  

  class VALUABLE_API ConfigElement
  {
  public:
    ConfigElement();
    virtual ~ConfigElement();

    ConfigValue *getConfigValue(const std::string & key);
    ConfigValue getConfigValueSafe(const std::string & key);

    void clear();

    void setType(const std::string & type)
    { m_type = type; }

    const std::string & elementName() const { return m_elementName; }
    void setElementName(const std::string & name)
    { m_elementName = name; }

    void addElement(const ConfigElement & e)
    { m_nodes.push_back(e); }

    void addValue(const ConfigValue & v)
    { m_values.push_back(v); }

    unsigned valueCount() const { return (unsigned) m_values.size(); }

    const ConfigValue & value(unsigned n) const
    { return m_values[n]; }

    unsigned childCount() const { return (unsigned) m_nodes.size(); }

    const ConfigElement & child(unsigned n) const
    { return m_nodes[n]; }

    ConfigElement & child(unsigned n)
    { return m_nodes[n]; }

  private:

    friend class ConfigDocument;

    std::vector<ConfigValue>   m_values;
    std::vector<ConfigElement> m_nodes;
    std::string                m_type;
    int                        m_depth;
    std::string                m_elementName;
  };
}

#endif

