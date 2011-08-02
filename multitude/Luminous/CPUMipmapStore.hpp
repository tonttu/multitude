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
#include <Radiant/Defines.hpp>

namespace Luminous {


  /** Shared CPUMipmap storage. This class is used as an access point
      to load CPUMipmap objects from the hard-disk. */
  class LUMINOUS_API CPUMipmapStore
  {
  public:
    /** Gets a shared pointer to an image file CPU-side mipmap. @sa Luminous::CPUMipmaps::startLoading

        @param filename The name of the image file

        @param compressed_mipmaps control whether compressed mipmaps should be used

        @return If the file is already open, then a shared pointer is
        returned. Otherwise CPUMipmapStore will create a new
        #Luminous::CPUMipmaps object, and return a shared pointer to that (if opened successfully).
     */
    static std::shared_ptr<CPUMipmaps> acquire(const std::string & filename, bool compressed_mipmaps);

    /** Release a #Luminous::CPUMipmaps object. If there are no references to
        the object, then its memory is freed.

        @deprecated no longer need to call release. Will be removed in 2.0.
     */
    MULTI_ATTR_DEPRECATED(static void release(std::shared_ptr<CPUMipmaps>));

    /** @deprecated copy shared pointer instead. Will be removed in 2.0. */
    MULTI_ATTR_DEPRECATED(static std::shared_ptr<CPUMipmaps> copy(std::shared_ptr<CPUMipmaps>));
  };
}

#endif
