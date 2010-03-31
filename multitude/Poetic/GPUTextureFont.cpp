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
#include "GPUTextureFont.hpp"
#include "GPUTextureGlyph.hpp"
#include "CPUBitmapGlyph.hpp"
#include "CPUFont.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#define DEFAULT_PADDING 3

namespace Poetic
{

  /*inline GLuint nextPowerOf2(GLuint in)
  {
     in -= 1;

     in |= in >> 16;
     in |= in >> 8;
     in |= in >> 4;
     in |= in >> 2;
     in |= in >> 1;

     return in + 1;
  }
  */

  /* Creates a number that is a multiple of four. Four is used as the
     buggy OSX (NVidia) drivers cannot handle arbitratry textures,
     even OpenGL 2.0 spec-compliant multiples-of-two -textures do not
     work in all conditions (sigh). */
  inline GLuint nextSize(GLuint in)
  {
    if((in & 0x3) == 0)
      return in;
    
    return in + 4 - (in & 0x3);
  }

  /// @todo make the class use Luminous::Texture2D internally
  GPUTextureFont::GPUTextureFont(CPUFontBase * cpuFont)
  : GPUFontBase(cpuFont),
    m_maxTextureSize(0),
    m_texWidth(0),
    m_texHeight(0),
    m_glyphMaxWidth(0),
    m_glyphMaxHeight(0),
    m_padding(DEFAULT_PADDING),
    m_xOffset(0),
    m_yOffset(0),
    m_reset(false)
  {
      m_remGlyphs = m_numGlyphs = m_cpuFont->face()->numGlyphs();
  }

  GPUTextureFont::~GPUTextureFont()
  {
    glDeleteTextures(m_textures.size(), (const GLuint *)&m_textures[0]);
  }

  Glyph * GPUTextureFont::makeGlyph(const Glyph * glyph)
  {
    const CPUBitmapGlyph * bmGlyph = dynamic_cast<const CPUBitmapGlyph *> (glyph);

    if(bmGlyph) {
      m_glyphMaxHeight = static_cast<int> (m_cpuFont->size().height());
      m_glyphMaxWidth = static_cast<int> (m_cpuFont->size().width());

      if(m_textures.empty()) {
        m_textures.push_back(createTexture());                                                                
        m_xOffset = m_yOffset = m_padding;
      }

      if(m_xOffset > (m_texWidth - m_glyphMaxWidth))
      {
        m_xOffset = m_padding;
        m_yOffset += m_glyphMaxHeight;

        if(m_yOffset > (m_texHeight - m_glyphMaxHeight)) {
          m_textures.push_back(createTexture());
          m_yOffset = m_padding;   
        }                        
      }

      GPUTextureGlyph * tempGlyph = 
        new GPUTextureGlyph(bmGlyph, m_textures.back(), m_xOffset, m_yOffset,
            m_texWidth, m_texHeight);               
      m_xOffset += 
        static_cast<int>(tempGlyph->bbox().high().x -
            tempGlyph->bbox().low().x + m_padding);
      --m_remGlyphs;
      return tempGlyph;            
    }    

    return 0;
  }

  GLuint GPUTextureFont::createTexture()
  {
    calculateTextureSize();
    
    int totalMemory = m_texWidth * m_texHeight;
    std::vector<uint8_t> bytes(totalMemory);

    if(!bytes.empty())
      bzero( & bytes[0], totalMemory);

    GLuint texID;
    glGenTextures(1, & texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m_texWidth, m_texHeight,
		 0, GL_ALPHA, GL_UNSIGNED_BYTE,  & bytes[0]);

    return texID;
  }

  void GPUTextureFont::calculateTextureSize()
  {
    if(!m_maxTextureSize) {   
      glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint *)&m_maxTextureSize);
      assert(m_maxTextureSize);

      /* Limit the maximum dimensions of the texture. This is done so
	 that OSX would not crash (Leopard) or corrupt the graphics
	 (Tiger).*/
      if(m_maxTextureSize > 2048)
	m_maxTextureSize = 2048;
    }
    
    m_texWidth = nextSize((m_remGlyphs * m_glyphMaxWidth) + (m_padding * 2));
    m_texWidth = m_texWidth > m_maxTextureSize ? m_maxTextureSize : m_texWidth;
        
    int h = (int) ( (m_texWidth - (m_padding * 2)) / m_glyphMaxWidth);
    
    /* Do not try to allocate space for all glyphs at once. This is
       relevant with DejaVu fonts, since most glyphs are never
       used. */
    int allocate = Nimble::Math::Min(256u, m_numGlyphs, m_remGlyphs);

    m_texHeight = nextSize((allocate / h + 1) * m_glyphMaxHeight);
    m_texHeight = m_texHeight > m_maxTextureSize ?
      m_maxTextureSize : m_texHeight; 
  }

  void GPUTextureFont::internalRender(const char * str, int n,
				      const Nimble::Matrix3 & m)
  {
    if(m_reset) 
      resetGLResources();

    GPUTextureGlyph::resetActiveTexture();

    GPUFontBase::internalRender(str, n, m);
  }

  void GPUTextureFont::internalRender(const wchar_t * str, int n,
				      const Nimble::Matrix3 & m)
  {
    if(m_reset)
      resetGLResources();

    GPUTextureGlyph::resetActiveTexture();

    GPUFontBase::internalRender(str, n, m);
  }

  void GPUTextureFont::faceSizeChanged()
  {
    m_reset = true;
    
    m_remGlyphs = m_numGlyphs = m_cpuFont->face()->numGlyphs();

    GPUFontBase::faceSizeChanged();

    m_reset = false;
  }

  void GPUTextureFont::resetGLResources()
  {
    if(!m_textures.empty()) {
      glDeleteTextures(m_textures.size(), (const GLuint *)&m_textures[0]);
      m_textures.clear();
    }
  }

}

