/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_COLLECTABLE_HPP
#define LUMINOUS_COLLECTABLE_HPP

#include "Export.hpp"
#include "GarbageCollector.hpp"

#include <Radiant/MemCheck.hpp>

namespace Luminous
{
  /// A utility class to make the work of GarbageCollector easier.
  class Collectable
  {
    MEMCHECKED
  public:
      Collectable() {}
    virtual ~Collectable() {GarbageCollector::objectDeleted(this);}
    /** Instructs the resource manager(s) to free linked resources.*/
    inline void freeLinkedResources() {GarbageCollector::objectDeleted(this);}
  };

}

#endif
