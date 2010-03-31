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

#ifndef LUMINOUS_DYNAMICTEXTURE_HPP
#define LUMINOUS_DYNAMICTEXTURE_HPP

#include <Luminous/Texture.hpp>
#include <Luminous/GLResource.hpp>
#include <Luminous/ImagePyramid.hpp>

#include <Radiant/RefPtr.hpp>

#include <Nimble/Vector2.hpp>
#include <Nimble/Matrix3.hpp>

#include <vector>

namespace Luminous
{

  /// Texture class that handles the management of a pruned mipmap pyramid.
  /// @todo Remove
  class DynamicTexture : public GLResource
  {
    public:
      DynamicTexture(GLResources * resources = 0);
      virtual ~DynamicTexture();

      /// Bind the dynamic texture to current active texture unit
      bool bind(const Nimble::Matrix3f & sceneToScreen, Nimble::Vector2 sceneSize);

      virtual int mipmapsOnGPU() const;
      virtual int mipmapsOnCPU() const;

      Radiant::RefPtr<ImagePyramid> pyramid() { return m_pyramid; }
      void setPyramid(Radiant::RefPtr<ImagePyramid> pyramid) { m_pyramid = pyramid; }

      Texture2D * getMipmap(int n);

      float aspect(int level = 0) const;

    protected:
      virtual void updateGPUMipmaps(Nimble::Vector2i size);
      virtual Texture2D * selectMipmap(Nimble::Vector2i size);

      std::vector<Radiant::RefPtr<Texture2D> > m_mipmaps;

      Radiant::RefPtr<ImagePyramid> m_pyramid;
  };
}

#endif
