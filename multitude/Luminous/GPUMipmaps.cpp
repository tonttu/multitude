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
#include "Utils.hpp"

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
/*      for(int i = 0; i < CPUMipmaps::MAX_MAPS; i++)
        resources()->eraseResource(m_keys + i);*/
    }
  }

  bool GPUMipmaps::bind(Nimble::Vector2 pixelsize)
  {   
    // Luminous::Utils::glCheck("GPUMipmaps::bind # 0");

    int best = m_cpumaps->getClosest(pixelsize);

    if(best < 0) {
      // trace("GPUMipmaps::bind # No mipmap");
      return false;
    }

    m_cpumaps->markImage(best);

    // We can upload the image at once:

    std::shared_ptr<ImageTex> img = m_cpumaps->getImage(best);

    if(img->isFullyLoadedToGPU()) {
      img->bind(resources(), GL_TEXTURE0, false);
      Luminous::Utils::glCheck("GPUMipmaps::bind # 1");
      return true;
    }

    long instantLimit = 1500000;

    if(((img->width() * img->height()) < instantLimit) /* &&
       (resources()->canUseGPUBandwidth(80.0f))*/) {
      img->bind(resources(), GL_TEXTURE0, false);

      // Luminous::Utils::glCheck("GPUMipmaps::bind # 2");
      return true;
    }

    else {

      // Then perform incremental texture upload:

      // if(resources()->canUseGPUBandwidth(50.0f))
      img->uploadBytesToGPU(resources(), instantLimit);

      // Lets check if we find something to use:
      for(unsigned i = 0; i < m_cpumaps->stackSize(); i++) {
        std::shared_ptr<ImageTex> test = m_cpumaps->getImage(i);

        if(!test)
          continue;

        int area = test->width() * test->height();

        if(test->isFullyLoadedToGPU() ||
           (area >= 64 && (area < (instantLimit / 3)))) {
          test->bind(resources(), GL_TEXTURE0, false);
          /*if(!Luminous::Utils::glCheck("GPUMipmaps::bind # 3"))
            error("GPUMipmaps::bind # %.5d %p %d x %d",
                  (int) test->ref().id(), resources(),
                  (int)test->width(), (int) test->height());
                  */
          return true;
        }
      }

    }

    // Luminous::Utils::glCheck("GPUMipmaps::bind");

    return false;
  }

  bool GPUMipmaps::bind(const Nimble::Matrix3 & transform,
            Nimble::Vector2 pixelsize)
  {
    Nimble::Vector2 lb = transform.project(0, 0);
    Nimble::Vector2 rb = transform.project(pixelsize.x, 0);
    Nimble::Vector2 lt = transform.project(0, pixelsize.y);
    Nimble::Vector2 rt = transform.project(pixelsize.x, pixelsize.y);

    float x1 = (rb-lb).length();
    float x2 = (rt-lt).length();

    float y1 = (lt-lb).length();
    float y2 = (rt-rb).length();

    return bind(Nimble::Vector2(std::max(x1, x2), std::max(y1, y2)));
  }

}
