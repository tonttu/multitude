/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef LUMINOUS_COLLECTABLE_HPP
#define LUMINOUS_COLLECTABLE_HPP

#include <Luminous/Export.hpp>

namespace Luminous
{
  /// A utility class to make the work of GarbageCollector easier.
  class LUMINOUS_API Collectable
  {
  public:
    Collectable();
    virtual ~Collectable();
    /** Instructs the resource manager(s) to free linked resources.*/
    void freeLinkedResources();
  };

}

#endif
