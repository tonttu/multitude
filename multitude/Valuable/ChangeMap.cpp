/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#if 0

#include "ChangeMap.hpp"

#include <Radiant/Trace.hpp>

namespace Valuable
{
  ChangeMap * ChangeMap::instance = 0;

  ChangeMap::ChangeMap()
  {
    if(instance) {
      Radiant::error(
"ChangeMap::ChangeMap # instance already exists, replacing it.");
    } 

    instance = this;
  }

  ChangeMap::~ChangeMap()
  {
    if(instance == this) instance = 0;
  }

  void ChangeMap::addDelete(Attribute * )
  {
    
  }

  void ChangeMap::addChange(Attribute * vo) 
  {
    if(instance) 
      instance->queueChange(vo);
  }

  void ChangeMap::queueChange(Attribute * vo) 
  { 
    m_changes.insert(vo); 
  }

}

#endif
