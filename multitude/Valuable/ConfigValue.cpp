/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ConfigValue.hpp"
namespace Valuable
{  
ConfigValue::ConfigValue(void)
{
  m_depth=0;
}
ConfigValue::ConfigValue(QString k, QString v)
{

  m_depth=0;
  m_key=k;
  m_value=v;
}

ConfigValue::~ConfigValue(void)
{
}
}
