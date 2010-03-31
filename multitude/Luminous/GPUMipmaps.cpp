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

#include "CPUMipmaps.hpp"

#include "GPUMipmaps.hpp"

#include <Luminous/GLResources.hpp>

#include <Radiant/Trace.hpp>

#include <assert.h>

namespace Luminous {

  using namespace Nimble;
  using namespace Radiant;

  GPUMipmaps::GPUMipmaps(CPUMipmaps * cpumaps, GLResources * resources)
    : GLResource(resources),
      m_cpumaps(cpumaps)
  {
    /* for(int i = 0; i < CPUMipmaps::MAX_MAPS; i++) {
      m_textures[i].setResources(resources);
    }
    */
  }

  GPUMipmaps::~GPUMipmaps()
  {
    if(!resources()) {
      error("GPUMipmaps::~GPUMipmaps # %p resources object is needed for clean delete", this);
    }
    else {
      for(int i = 0; i < CPUMipmaps::MAX_MAPS; i++)
        resources()->eraseResource(m_keys + i);
    }
  }

  bool GPUMipmaps::bind(Nimble::Vector2 pixelsize)
  {
    int best = m_cpumaps->getOptimal(pixelsize);

    if(best < 0) {
      // trace("GPUMipmaps::bind # No mipmap");
      return false;
    }

    // trace("GPUMipmaps::bind %f %f %d", pixelsize.x, pixelsize.y, best);


    Collectable * key = m_keys + best;

    GLResource * res = resources()->getResource(key);
    
    Texture2D * tex = 0;

    if(res) {
      // trace("Got optimal texture");
      m_cpumaps->markImage(best);
      tex = dynamic_cast<Texture2D *> (res);
      assert(tex);
      tex->bind();
    }
    else {

      int mybest = -1;

      // Try to find a ready texture object already in GPU RAM

      for(int i = 0; i < m_cpumaps->maxLevel() && !res; i++) {
        int index = best + i;

        if(index <= m_cpumaps->maxLevel()) {

          res = resources()->getResource(m_keys + index);
          if(res) {
            mybest = index;
          }
        }

        index = best - i;

        if(index >= 0 && !res) {
          res = resources()->getResource(m_keys + index);
          if(res) {
            mybest = index;
          }
        }
      }
      
      // See what is in CPU RAM

      int closest = m_cpumaps->getClosest(pixelsize);

      if(((closest < 0) || Math::Abs(mybest - best) <= Math::Abs(closest - best))
         && mybest >= 0) {

        tex = dynamic_cast<Texture2D *> (res);
        assert(tex);
        tex->bind();
      }
      else if(closest >= 0) {
        tex = new Texture2D(resources());
        resources()->addResource(m_keys + closest, tex);
        tex->loadImage(*m_cpumaps->getImage(closest),
		       closest == CPUMipmaps::lowestLevel());
        tex->bind();
      }      
    }
    
    if(tex) {
      resources()->deleteAfter(tex, 10);
    }
    else
      return false;

    return true;
  }

  bool GPUMipmaps::bind(const Nimble::Matrix3 & transform, 
			Nimble::Vector2 pixelsize)
  {
    return bind(transform.extractScale() * pixelsize);
  }

}
