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

#ifndef POETIC_CPU_WRAPPER_FONT_HPP
#define POETIC_CPU_WRAPPER_FONT_HPP

#include <Poetic/CPUFont.hpp>
#include <Poetic/Export.hpp>

#include <Luminous/GLResources.hpp>

namespace Poetic
{
  class CPUManagedFont;
  class GPUWrapperFont;

  /// A font on the CPU that wraps the point size of the font for convenience.
  class POETIC_API CPUWrapperFont : public CPUFont
  {
  public:
    CPUWrapperFont(CPUManagedFont * mfont);
    ~CPUWrapperFont();

    float advance(const char * str, int n = -1);
    float advance(const wchar_t * str, int n = -1);
    float advance(const std::wstring & str)
    { return advance(str.c_str(), str.size()); }
    
    int faceSize() const        { return m_pointSize; }
    bool setFaceSize(int size, int = POETIC_DEFAULT_RESOLUTION)
    { m_pointSize = size; return true; }

    float minimumRenderSize() const        { return m_minimumRenderSize; }
    void setMinimumRenderSize(float size) { m_minimumRenderSize = size; }

    float ascender() const;
    float descender() const;
    float lineHeight() const;

    void bbox(const char * str, BBox & bbox);
    void bbox(const wchar_t * str, BBox & bbox);

    bool load(const char * fontFilePath);

    GPUWrapperFont * getGPUFont();

  protected:

    virtual GPUFont * createGPUFont();
    CPUManagedFont * m_managedFont;
    int m_pointSize;
    float m_minimumRenderSize;
  };


}

#endif
