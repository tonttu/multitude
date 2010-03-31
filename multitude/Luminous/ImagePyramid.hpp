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
#ifndef LUMINOUS_IMAGE_PYRAMID_HPP
#define LUMINOUS_IMAGE_PYRAMID_HPP

#include <Nimble/Vector2.hpp>

#include <Luminous/Image.hpp>

#include <vector>

namespace Luminous
{

  /// A simple image pyramid used for storing mipmaps.
  /// @todo remove
  class ImagePyramid
  {
    public:
      /// A single level in the mipmap pyramid
      struct MipmapLevel
      {
        Luminous::Image * image;
        bool ready;
      };

      ImagePyramid() {};
      ~ImagePyramid() {};

      void addLevel(Luminous::Image * image) {
        MipmapLevel level = { image, false };
        m_levels.push_back(level);
      }

      const MipmapLevel & getLevel(int n) const { return m_levels[n]; }
      int levels() const { return m_levels.size(); }

      Nimble::Vector2i levelSize(int n) const { return Nimble::Vector2i(m_levels[n].image->width(), m_levels[n].image->height()); }

    private:
      std::vector<MipmapLevel> m_levels;
  };

}

#endif
