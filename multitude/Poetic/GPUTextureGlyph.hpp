/* COPYRIGHT
 */

#ifndef POETIC_GPU_TEXTURE_GLYPH_HPP
#define POETIC_GPU_TEXTURE_GLYPH_HPP

#include "Export.hpp"
#include "Glyph.hpp"

#include <Luminous/Luminous.hpp>

namespace Luminous {

  class Texture2D;
}

namespace Poetic
{
  class CPUBitmapGlyph;

  /// A glyph stored in a texture on the GPU
  class POETIC_API GPUTextureGlyph : public Glyph
  {
    public:
      /// Constructs a new texture glyph
      GPUTextureGlyph(const CPUBitmapGlyph * glyph, Luminous::Texture2D * tex, int xOff, int yOff, GLsizei width, GLsizei height);
      virtual ~GPUTextureGlyph();

      virtual Nimble::Vector2 render(Nimble::Vector2 pen, const Nimble::Matrix3 & m, Nimble::Vector2f ** ptr);

    private:
      int m_width;
      int m_height;

      Nimble::Vector2 m_pos;
      Nimble::Vector2 m_uv[2];

      Luminous::Texture2D * m_textureId;
  };

}

#endif
