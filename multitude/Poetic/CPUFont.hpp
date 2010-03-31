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
#ifndef POETIC_CPU_FONT_HPP
#define POETIC_CPU_FONT_HPP

#include <Poetic/GlyphContainer.hpp>
#include <Poetic/BBox.hpp>

#include <Luminous/Collectable.hpp>
#include <Luminous/GLResources.hpp>

#define POETIC_DEFAULT_RESOLUTION 72

namespace Poetic
{

  class GPUFont;

  /// An abstract base class providing a common interface for all fonts residing
  /// in CPU memory.
  /// @todo Doc
  class POETIC_API CPUFont : public::Luminous::Collectable
  {
  public:
    virtual ~CPUFont() {}
    virtual float advance(const char * str, int n = -1) = 0;
    virtual float advance(const wchar_t * str, int n = -1) = 0;

    float advance(const std::string & str)
    {
      return advance(str.c_str());
    }
    float advance(const std::wstring & str)
    {
      return advance(str.c_str());
    }

      virtual int faceSize() const = 0;
      virtual bool setFaceSize(int size, int resolution = POETIC_DEFAULT_RESOLUTION) = 0;

      virtual float ascender() const = 0;
      virtual float descender() const = 0;
      virtual float lineHeight() const = 0;

      virtual void bbox(const char * str, BBox & bbox) = 0;
      virtual void bbox(const wchar_t * wstr, BBox & bbox) = 0;

      virtual bool load(const char * fontFilePath) = 0;

      virtual GPUFont * createGPUFont() = 0;
      GPUFont * getGPUFont();
  };

}

#endif
