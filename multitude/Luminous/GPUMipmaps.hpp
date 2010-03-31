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

#ifndef LUMINOUS_GPU_MIPMAPS_HPP
#define LUMINOUS_GPU_MIPMAPS_HPP

#include <Luminous/Export.hpp>
#include <Luminous/GLResource.hpp>
#include <Luminous/Texture.hpp>

#include <Nimble/Matrix3.hpp>

#include <Luminous/CPUMipmaps.hpp>

namespace Luminous {

  /** A set of image mipmaps in the GPU memory. This class is used to
      load images from the CPUMipmaps object to the GPU as
      required and available. */
  class LUMINOUS_API GPUMipmaps : public GLResource
  {
  public:
    GPUMipmaps(CPUMipmaps *, GLResources * resources = 0);
    virtual ~GPUMipmaps();

    /** Binds the optimal mipmap to the OpenGL context. 
	
	@param pixelsize The size in which the image would be
	used. This size should be actual screen pixels, so that the
	underlying system can select the best mipmap.
     */
    bool bind(Nimble::Vector2 pixelsize);

    /** Binds the optimal mipmap to the OpenGL context. 
	
	@param transform The transformation matrix that would be used
	before rendering the content using the texture.

	@param pixelsize The size in which the image would be
	used, after multiplication with the given transformation matrix.

	
     */
    bool bind(const Nimble::Matrix3 & transform, 
	      Nimble::Vector2 pixelsize);

  private:

    CPUMipmaps * m_cpumaps;

    Collectable m_keys[CPUMipmaps::MAX_MAPS];

  };

}

#endif
