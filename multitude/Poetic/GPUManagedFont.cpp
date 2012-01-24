/* COPYRIGHT
 */
#include "GPUManagedFont.hpp"
#include "CPUBitmapFont.hpp"
#include "CPUManagedFont.hpp"

#include <Nimble/Math.hpp>

//#include <Radiant/Trace.hpp>

namespace Poetic
{

  GPUManagedFont::GPUManagedFont(CPUManagedFont * cmf,
                 Luminous::RenderContext * glrc)
    : GLResource(glrc),
    m_cmf(cmf)
  {
    assert(m_cmf != 0);
    m_fonts.resize(m_cmf->fontCount());
  }

  GPUManagedFont::~GPUManagedFont()
  {
    for(size_t i = 0; i < m_fonts.size(); i++)
      delete m_fonts.at(i);
  }

  void GPUManagedFont::render(const QString & text,
			      int pointSize, const Nimble::Matrix3 & m,
                              float minimumSize)
  {
    GPUFont * gf;
    float sfix;

    if(!computeRenderParams(m, pointSize, &gf, &sfix, minimumSize))
      return;

    gf->render(text, m * Nimble::Matrix3::scale2D(Nimble::Vector2(sfix,sfix)));
  }

  void GPUManagedFont::render(const char * str, int n, int pointSize,
                              const Nimble::Matrix3 & m, float minimumSize)
  {
    GPUFont * gf;
    float sfix;


    if(!computeRenderParams(m, pointSize, &gf, &sfix, minimumSize))
      return;

    gf->render(str, n,
               m * Nimble::Matrix3::scale2D(Nimble::Vector2(sfix,sfix)));
  }

  void GPUManagedFont::render(const wchar_t * str, int n, int pointSize,
                              const Nimble::Matrix3 & m, float minimumSize)
  {
    GPUFont * gf;
    float sfix;

    if(!computeRenderParams(m, pointSize, &gf, &sfix, minimumSize))
      return;

    gf->render(str, n ,
               m * Nimble::Matrix3::scale2D(Nimble::Vector2(sfix, sfix)));
  }


  GPUFontBase * GPUManagedFont::getFont(int fontNo)
  {
    GPUFontBase * font = m_fonts[fontNo];
    if(!font) {
      // Create new
      CPUFontBase * cFont = static_cast<CPUFontBase *> (m_cmf->getFont(fontNo));
      assert(cFont);
      font = new GPUTextureFont(cFont);

      m_fonts[fontNo] = font;
    }

    return font;
  }

  bool GPUManagedFont::computeRenderParams
  (const Nimble::Matrix3 & m, int pts,
   GPUFont ** gf, float * scale, float minimumSize)
  {
    float s = m.extractScale();

    float actual = pts * s;

    if(actual < minimumSize) {
      return false;
    }

    int fontNo = m_cmf->selectFont(actual);

    *gf = getFont(fontNo);

    // Amount we need to scale
    const CPUFont * cf = (*gf)->cpuFont();
    *scale = actual / (cf->faceSize() * s);

    return true;
//    Radiant::trace("GPUManagedFont::render # (scale %f, font pts %d, actual %f); (used pts %d, scale fix %f, result %f)", s, pts, actual, cf->faceSize(), *scale, s * *scale * (float)cf->faceSize());
  }

}
