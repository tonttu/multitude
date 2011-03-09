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
    /// Constructs a new wrapper for the given managed font
    CPUWrapperFont(CPUManagedFont * mfont);
    ~CPUWrapperFont();

    /// Returns the advance for the given string
    float advance(const char * str, int n = -1);
    /// @copydoc advance
    float advance(const wchar_t * str, int n = -1);
    /// @copydoc advance
    float advance(const QString & str)
    { return advance(str.c_str(), (int) str.size()); }
    
    /// Returns the face size
    int faceSize() const        { return m_pointSize; }
    /// Sets the face size for the wrapper
    bool setFaceSize(int size, int = POETIC_DEFAULT_RESOLUTION)
    { m_pointSize = size; return true; }

    /// Returns the minumum render size that the font is still used. Smaller text than this is rendered as lines.
    float minimumRenderSize() const        { return m_minimumRenderSize; }
    /// Sets the minimum render size for the font.
    void setMinimumRenderSize(float size) { m_minimumRenderSize = size; }

    /// Returns the ascender height
    float ascender() const;
    /// Returns the descender height
    float descender() const;
    /// Returns the line height
    float lineHeight() const;

    /// Returns the bounding box for the given string
    void bbox(const char * str, BBox & bbox);
    /// @copydoc bbox
    void bbox(const wchar_t * str, BBox & bbox);

    /// Loads a font from the given .ttf file
    bool load(const char * fontFilePath);

    /// Returns the matching GPUWrapper font
    GPUWrapperFont * getGPUFont();

  protected:
    /// Creates a new GPU font
    virtual GPUFont * createGPUFont();
    /// The managed CPU font that this wrapper wraps
    CPUManagedFont * m_managedFont;
    /// Returns the point size for the wrapper font
    int m_pointSize;
    /// The minimum render size for the font
    float m_minimumRenderSize;
  };


}

#endif
