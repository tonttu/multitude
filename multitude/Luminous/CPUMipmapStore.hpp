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

#ifndef LUMINOUS_CPU_MIPMAP_STORE_HPP
#define LUMINOUS_CPU_MIPMAP_STORE_HPP

#include <Luminous/CPUMipmaps.hpp>

namespace Luminous {


  /** Shared CPUMipmap storage. This class is used as an access point
      to load CPUMipmap objects from the hard-disk. */
  class LUMINOUS_API CPUMipmapStore
  {
  public:
    /** Gets a pointer to an image file CPU-side mipmap.

    @return If the file already open, then an old pointer is
    returned. Otherwise CPUMipmapStore will create a new
        #Luminous::CPUMipmaps object, and return that (if opened successfully).
     */
    static CPUMipmaps * acquire(const std::string & filename, bool immediate = true);
    static CPUMipmaps * acquire(const char * filename, bool immediate = true);

    /** Release a #Luminous::CPUMipmaps object. If there are no references to
    the object, then its memory is freed. */
    static void release(Luminous::CPUMipmaps *);
  };
}

#endif
