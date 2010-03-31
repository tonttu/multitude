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
#include "CPUWrapperFont.hpp"
#include "GPUWrapperFont.hpp"
#include "CPUManagedFont.hpp"

#include <Radiant/Trace.hpp>

namespace Poetic
{

  CPUWrapperFont::CPUWrapperFont(CPUManagedFont * mfont)
  : m_managedFont(mfont),
    m_pointSize(16),
    m_minimumRenderSize(3)
  {}

  CPUWrapperFont::~CPUWrapperFont()
  {}

  GPUWrapperFont * CPUWrapperFont::getGPUFont()
  {
    Luminous::GLResources * glr = Luminous::GLResources::getThreadResources();

    GPUWrapperFont * gf = dynamic_cast<GPUWrapperFont *> (glr->getResource(this));

    if(gf) 
      return gf;

    // Get the managed GPU font
    GPUManagedFont * gmf = dynamic_cast<GPUManagedFont *> (glr->getResource(m_managedFont));
    if(!gmf) {
      gmf = new GPUManagedFont(m_managedFont, glr);
      glr->addResource(m_managedFont, gmf);
    }

    // Create the resource                
    GPUWrapperFont * font = new GPUWrapperFont(gmf, this);
    glr->addResource(this, font);

    return font;
  }

  float CPUWrapperFont::advance(const char * str, int n)
  {
    CPUFont * f = m_managedFont->getMetricFont();
//    float s = static_cast<float> (f->faceSize()) / static_cast<float> (m_pointSize);
    float s = static_cast<float> (m_pointSize) / static_cast<float> (f->faceSize());

    return f->advance(str, n) * s;
  }

  float CPUWrapperFont::advance(const wchar_t * str, int n)
  {
    CPUFont * f = m_managedFont->getMetricFont();
    float s = static_cast<float> (m_pointSize) / static_cast<float> (f->faceSize());

    return f->advance(str, n) * s;
  }
  
  float CPUWrapperFont::ascender() const 
  {
    CPUFont * f = m_managedFont->getMetricFont();
    float s = static_cast<float> (m_pointSize) / static_cast<float> (f->faceSize());

    return f->ascender() * s;
  }

  float CPUWrapperFont::descender() const 
  {
    CPUFont * f = m_managedFont->getMetricFont();
    float s = static_cast<float> (m_pointSize) / static_cast<float> (f->faceSize());

    return f->descender() * s;
  }

  float CPUWrapperFont::lineHeight() const 
  {
    CPUFont * f = m_managedFont->getMetricFont();
    float s = static_cast<float> (m_pointSize) / static_cast<float> (f->faceSize());

    return f->lineHeight() * s;
  }

  void CPUWrapperFont::bbox(const char * str, BBox & bbox)
  {
    CPUFont * f = m_managedFont->getMetricFont();
    float s = static_cast<float> (m_pointSize) / static_cast<float> (f->faceSize());

    f->bbox(str, bbox);
    bbox.scale(s);
  }

  void CPUWrapperFont::bbox(const wchar_t * str, BBox & bbox)
  {
    CPUFont * f = m_managedFont->getMetricFont();
    float s = static_cast<float> (m_pointSize) / static_cast<float> (f->faceSize());

    f->bbox(str, bbox);
    bbox.scale(s);
  }

  bool CPUWrapperFont::load(const char * )
  {
    Radiant::error("CPUWrapperFont::load # don't call me");
    return false;
  }

  GPUFont * CPUWrapperFont::createGPUFont()
  {
    Radiant::error("CPUWrapperFont::createGPUFont # Should not be called");
    return 0;
  }
}

