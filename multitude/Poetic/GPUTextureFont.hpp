/* COPYRIGHT
 */
#ifndef POETIC_GPU_TEXTURE_FONT_HPP
#define POETIC_GPU_TEXTURE_FONT_HPP

#include "Export.hpp"
#include "GPUFontBase.hpp"

#include <Luminous/Luminous.hpp>
#include <Luminous/Shader.hpp>
#include <Luminous/Texture.hpp>

#include <vector>

namespace Poetic
{

  /// A GPU font that uses textures to render the glyphs
  class POETIC_API GPUTextureFont : public GPUFontBase
  {
  public:
    /// Constructs a new GPU texture font
    GPUTextureFont(CPUFontBase * cpuFont);
    virtual ~GPUTextureFont();

  protected:
    virtual void internalRender(const char * str, int n, const Nimble::Matrix3 & m);
    virtual void internalRender(const wchar_t * str, int n, const Nimble::Matrix3 & m);

    inline virtual Glyph * makeGlyph(const Glyph * cpuGlyph);

    virtual void faceSizeChanged();

  private:
    inline void calculateTextureSize();
    inline Luminous::Texture2D * createTexture();

    void resetGLResources();

    GLsizei m_maxTextureSize;

    GLsizei m_texWidth;
    GLsizei m_texHeight;

    std::vector<Luminous::Texture2D *> m_textures;

    int m_glyphMaxWidth;
    int m_glyphMaxHeight;

    int m_padding;

    unsigned int m_numGlyphs;
    unsigned int m_remGlyphs;

    int m_xOffset;
    int m_yOffset;

    bool m_reset;

    Luminous::Shader* m_fontShader;
  };

}

#endif
