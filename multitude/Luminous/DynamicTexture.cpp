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

#include "DynamicTexture.hpp"

#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

#include <Nimble/Math.hpp>

#include <sstream>

using namespace Radiant;
using namespace std;

namespace Luminous
{

  DynamicTexture::DynamicTexture(GLResources * resources)
    : GLResource(resources),
      m_pyramid(0)
  {}

  DynamicTexture::~DynamicTexture()
  {}

  /// @todo check resources 
  Texture2D* DynamicTexture::selectMipmap(Nimble::Vector2i size) 
  {
    // Select a mipmap based on the size hint
    int desiredMax = std::max(size.x, size.y);

    int available = mipmapsOnGPU();

    if(available == 0) return 0;

    int level = 0;
    for(; level < available - 1; level++) {
      Luminous::Image * img = m_pyramid->getLevel(level).image;

      int dim = std::max(img->width(), img->height());

      if(desiredMax < dim) break;
    }

    return m_mipmaps[level].ptr();
  }

  int DynamicTexture::mipmapsOnGPU() const
  {
    return m_mipmaps.size();
  }

  int DynamicTexture::mipmapsOnCPU() const
  {
    return m_pyramid.ptr() ? m_pyramid->levels() : 0;
  }

  Texture2D * DynamicTexture::getMipmap(int n)
  {
    if(n >= (int)m_mipmaps.size()) 
      return 0;

    return m_mipmaps[n].ptr();
  }

  void DynamicTexture::updateGPUMipmaps(Nimble::Vector2i size)
  {
    // Nothing we can do if nothing has been loaded yet
    if(mipmapsOnCPU() == 0) {
      return;
    }

    // Based on the currently most detailed resident mipmap and the hint, upload
    // a new mipmap or purge the existing one

    int residentLevel = mipmapsOnGPU() - 1;

    int residentDim = (residentLevel >= 0 ? m_pyramid->levelSize(residentLevel).maximum() : 0);
    int nextLowerDim = (residentLevel > 0 ? m_pyramid->levelSize(residentLevel - 1).maximum() : 0);

    int desiredDim = std::max(size.x, size.y); 

    if(desiredDim > residentDim) {
      // If there's not more detailed data on the CPU, nothing we can do
      if(mipmapsOnCPU() <= mipmapsOnGPU()) {
        return;
      }
    
      // Upload a sharper mipmap
      Luminous::Image * img = m_pyramid->getLevel(residentLevel + 1).image;

      Texture2D * tex = Texture2D::fromImage(*img, false, resources());
     
      if(!tex) {
        error("DynamicTexture::updateGPUMipmaps # failed to create a texture mipmap");
        return;
      }
 
      /// @todo texture addressing shouldn't be changed here 
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
     
      m_mipmaps.push_back(Radiant::RefPtr<Texture2D>(tex));
    } else if(desiredDim < nextLowerDim) {    
      // Anything to throw away? Should we throw the last level away?
      if(residentLevel <= 0) {
        return;
      }

      m_mipmaps.pop_back();
      //trace("\tPurged mipmap level %d", residentLevel);     
    }
  }

  float DynamicTexture::aspect(int level) const
  {
    return m_pyramid.ptr()->getLevel(level).image->aspect();
  }

  bool DynamicTexture::bind(const Nimble::Matrix3f & sceneToScreen, Nimble::Vector2 sceneSize)
  {
    Nimble::Vector2 trueSize = sceneSize * sceneToScreen.extractScale();

    updateGPUMipmaps(trueSize);
    Texture2D * tex = selectMipmap(trueSize);

    if(tex) {
      tex->bind();
      return true;
    }

    return false;
  }

}
