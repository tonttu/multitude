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
#include "FontManager.hpp"

#include <Luminous/Utils.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#define DEFAULT_PADDING 3

#define VERTEX_ARRAY_SIZE 1024



namespace Poetic
{
  using namespace Radiant;

#define SHADER(str) #str
  static const char * g_fontVShaderSource = SHADER(
    uniform mat3   transform;
    varying vec4   color;
    varying vec4   uv;
    void main (void) {
      mat4 trans4 = mat4(transform[0].x, transform[0].y, 0, transform[0].z,
        transform[1].x, transform[1].y, 0, transform[1].z,
        0, 0, 1, 0,
        transform[2].x, transform[2].y, 0, transform[2].z);
      vec3 pos = transform * vec3(gl_Vertex.x, gl_Vertex.y, 1);
      pos.xy = pos.xy / pos.z;
      pos.z = 1.0;
      uv = gl_MultiTexCoord0;
      gl_ClipVertex = gl_ModelViewMatrix * trans4 * gl_Vertex;
      color = gl_Color;
      gl_Position = gl_ModelViewProjectionMatrix * vec4(pos.x, pos.y, pos.z, 1);
    }
  );

  static const char * g_fontFShaderSource = SHADER(
    uniform sampler2D fontTexture;
    varying vec4 color;
    varying vec4 uv;
    void main (void) {
      gl_FragColor = color;
      gl_FragColor.a *= texture2D(fontTexture, uv.st).a;
    }
  );

  /* Creates a number that is a multiple of four. Four is used as the
     buggy OS X (NVidia) drivers cannot handle arbitratry textures,
     even OpenGL 2.0 spec-compliant multiples-of-two -textures do not
     work in all conditions (sigh).

     Well, the issue really is with OpenGL
*/
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
    m_reset(false),
    m_fontShader(new Luminous::Shader())
  {

    if(!m_fontShader->isDefined()) {
      m_fontShader->setVertexShader(g_fontVShaderSource);
      m_fontShader->setFragmentShader(g_fontFShaderSource);
    }

    m_remGlyphs = m_numGlyphs = m_cpuFont->face()->numGlyphs();
  }

  GPUTextureFont::~GPUTextureFont()
  {
    delete m_fontShader;

    // Accessing [0] on empty vectors crashes on Windows
    if(!m_textures.empty())
        glDeleteTextures((GLsizei) m_textures.size(), (const GLuint *)&m_textures[0]);
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

  Luminous::Texture2D * GPUTextureFont::createTexture()
  {
    calculateTextureSize();

    int totalMemory = m_texWidth * m_texHeight;
    std::vector<uint8_t> bytes(totalMemory);

    if(!bytes.empty())
      bzero( & bytes[0], totalMemory);

    Luminous::Texture2D * tex = new Luminous::Texture2D();
    tex->loadBytes(GL_ALPHA, m_texWidth, m_texHeight, 0, Luminous::PixelFormat::alphaUByte(), false);
    tex->setPersistent(true);
    /*
    GLuint texID;
    glGenTextures(1, & texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, m_texWidth, m_texHeight,
         0, GL_ALPHA, GL_UNSIGNED_BYTE,  & bytes[0]);
         */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    return tex;
  }

  void GPUTextureFont::calculateTextureSize()
  {
    if(!m_maxTextureSize) {
      glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint *)&m_maxTextureSize);
      assert(m_maxTextureSize);

      /* Limit the maximum dimensions of the texture. This is done so
     that OS X would not crash (Leopard) or corrupt the graphics
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
    if(!n)
      return;

    if(m_reset)
      resetGLResources();

    // GPUTextureGlyph::resetActiveTexture();

    // info("GPUTextureFont::internalRender # in");
    Luminous::GLSLProgramObject * shader = m_fontShader->bind();
    // info("GPUTextureFont::internalRender # out");

    shader->setUniformInt("fontTexture", 0);

    Nimble::Vector2f tmp[VERTEX_ARRAY_SIZE];

    int used = 0;
    int use = 0;

#if 0
    const GLsizei vertexSize = 2 * sizeof(Nimble::Vector2f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);


    Nimble::Matrix3 trans = m;
    while (true) {
      shader->setUniformMatrix3("transform", trans);

      use = std::min(VERTEX_ARRAY_SIZE/8, n-used);
      Nimble::Vector2f * ptr = &tmp[0];

      GPUFontBase::internalRender(str+used, use, trans, &ptr);

      glVertexPointer(2, GL_FLOAT, vertexSize, &tmp[0]);
      glTexCoordPointer(2, GL_FLOAT, vertexSize, &tmp[1]);
      glDrawArrays(GL_QUADS, 0, 4 * use);

      if (VERTEX_ARRAY_SIZE >= 4*2*(n-used))
        break;

      float offset = getLastAdvance();
      //float offset = cpuFont()->advance(str+used, use);
      trans *= Nimble::Matrix3::translate2D(offset, .0f);
      used += use;
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#else

    Nimble::Matrix3 trans = m;
    while (true) {
      shader->setUniformMatrix3("transform", trans);

      use = std::min(VERTEX_ARRAY_SIZE/8, n-used);
      Nimble::Vector2f * ptr = &tmp[0];

      GPUFontBase::internalRender(str+used, use, trans, &ptr);

      glBegin(GL_QUADS);
      for(int i = 0; i < (use * 2 * 4); i += 2) {
        glTexCoord2fv(tmp[i+1].data());
        glVertex2fv(tmp[i].data());
      }

      glEnd();

      if (VERTEX_ARRAY_SIZE >= 4*2*(n-used))
        break;

      float offset = getLastAdvance();

      trans *= Nimble::Matrix3::translate2D(offset, .0f);
      used += use;
    }
#endif
  }

  /// exactly the same code as below
  void GPUTextureFont::internalRender(const wchar_t * str, int n,
                      const Nimble::Matrix3 & m)
  {
    if(!n)
      return;

    if(m_reset)
      resetGLResources();

    // GPUTextureGlyph::resetActiveTexture();

    // info("GPUTextureFont::internalRender # in");
    Luminous::GLSLProgramObject * shader = m_fontShader->bind();
    // info("GPUTextureFont::internalRender # out");

    Luminous::Utils::glCheck("GPUTextureFont::internalRender");

    shader->setUniformInt("fontTexture", 0);

    Nimble::Vector2f tmp[VERTEX_ARRAY_SIZE];

    int used = 0;
    int use = 0;

#if 0
    const GLsizei vertexSize = 2 * sizeof(Nimble::Vector2f);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);


    Nimble::Matrix3 trans = m;
    while (true) {
      shader->setUniformMatrix3("transform", trans);

      use = std::min(VERTEX_ARRAY_SIZE/8, n-used);
      Nimble::Vector2f * ptr = &tmp[0];

      GPUFontBase::internalRender(str+used, use, trans, &ptr);

      glVertexPointer(2, GL_FLOAT, vertexSize, &tmp[0]);
      glTexCoordPointer(2, GL_FLOAT, vertexSize, &tmp[1]);
      glDrawArrays(GL_QUADS, 0, 4 * use);

      if (VERTEX_ARRAY_SIZE >= 4*2*(n-used))
        break;

      float offset = getLastAdvance();
      //float offset = cpuFont()->advance(str+used, use);
      trans *= Nimble::Matrix3::translate2D(offset, .0f);
      used += use;
    }

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
#else


    Nimble::Matrix3 trans = m;
    while (true) {
      shader->setUniformMatrix3("transform", trans);

      use = std::min(VERTEX_ARRAY_SIZE/8, n-used);
      Nimble::Vector2f * ptr = &tmp[0];

      GPUFontBase::internalRender(str+used, use, trans, &ptr);

      glBegin(GL_QUADS);
      for(int i = 0; i < (use * 2 * 4); i += 2) {
        glTexCoord2fv(tmp[i+1].data());
        glVertex2fv(tmp[i].data());
      }

      glEnd();

      if (VERTEX_ARRAY_SIZE >= 4*2*(n-used))
        break;

      float offset = getLastAdvance();

      trans *= Nimble::Matrix3::translate2D(offset, .0f);
      used += use;
    }


#endif

    Luminous::Utils::glCheck("GPUTextureFont::internalRender # EXIT");

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
      glDeleteTextures((GLsizei) m_textures.size(), (const GLuint *)&m_textures[0]);
      m_textures.clear();
    }
  }

}
