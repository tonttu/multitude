/* COPYRIGHT
 *
 * This file is part of Poetic.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Poetic.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */
#ifndef POETIC_GPU_TEXTURE_FONT_HPP
#define POETIC_GPU_TEXTURE_FONT_HPP

#include <Poetic/Export.hpp>
#include <Poetic/GPUFontBase.hpp>

#include <Luminous/Luminous.hpp>

#include <vector>

namespace Poetic 
{

  /// A GPU font that uses textures to render the glyphs
  class POETIC_API GPUTextureFont : public GPUFontBase
  {
  public:
    GPUTextureFont(CPUFontBase * cpuFont);
    virtual ~GPUTextureFont();

  protected:
    virtual void internalRender(const char * str, int n, const Nimble::Matrix3 & m);
    virtual void internalRender(const wchar_t * str, int n, const Nimble::Matrix3 & m);
    
    inline virtual Glyph * makeGlyph(const Glyph * cpuGlyph);

    virtual void faceSizeChanged();

  private:
    inline void calculateTextureSize();
    inline GLuint createTexture();

    void resetGLResources();
   
    GLsizei m_maxTextureSize;

    GLsizei m_texWidth;
    GLsizei m_texHeight;

    std::vector<GLuint> m_textures;

    int m_glyphMaxWidth;
    int m_glyphMaxHeight;

    int m_padding;

    unsigned int m_numGlyphs;
    unsigned int m_remGlyphs;

    int m_xOffset;
    int m_yOffset;

    bool m_reset;
  };

}

#endif
