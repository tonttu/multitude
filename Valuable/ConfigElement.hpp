/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_ELEMENT_HPP
#define VALUABLE_ELEMENT_HPP

#include <Valuable/Export.hpp>
#include <Valuable/ConfigValue.hpp>

#include <iostream>
#include <vector>
#include <QString>

namespace Valuable
{  

  /// Configuration block in ConfigDocument
  class VALUABLE_API ConfigElement
  {
  public:
    ConfigElement();
    virtual ~ConfigElement();

    /// Gets the value with the given name
    /// @param key name of the value
    /// @return pointer to the value or 0 if the value is not found
    ConfigValue *getConfigValue(const QString & key);
    /// Gets the value with the given name
    /// @param key name of the value
    /// @return copy of the value or an empty value if the given key is not found
    ConfigValue getConfigValueSafe(const QString & key);

    /// Removes all values from the element
    void clear();

    /// Sets the type of the element
    void setType(const QString & type)
    { m_type = type; }

    /// Returns the name of the element
    const QString & elementName() const { return m_elementName; }
    /// Sets the name of the element
    void setElementName(const QString & name)
    { m_elementName = name; }

    /// Adds a sub-element to the element
    void addElement(const ConfigElement & e)
    { m_nodes.push_back(e); }

    /// Adds a value to the element
    void addValue(const ConfigValue & v)
    { m_values.push_back(v); }

    /// Returns the number of values in the element
    size_t valueCount() const { return m_values.size(); }

    /// Returns the ith value
    const ConfigValue & value(size_t n) const
    { return m_values[n]; }

    /// Returns the number of sub-elements
    size_t childCount() const { return m_nodes.size(); }

    /// Returns the ith sub-element
    /// @param n Index of child, starting from 0
    /// @return Child config element
    const ConfigElement & child(size_t n) const
    { return m_nodes[n]; }
    /// @copydoc child
    ConfigElement & child(size_t n)
    { return m_nodes[n]; }

  private:

    friend class ConfigDocument;

    std::vector<ConfigValue>   m_values;
    std::vector<ConfigElement> m_nodes;
    QString                m_type;
    int                        m_depth;
    QString                m_elementName;
  };
}

#endif

