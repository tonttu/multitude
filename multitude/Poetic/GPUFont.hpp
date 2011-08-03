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
#ifndef POETIC_GPU_FONT_HPP
#define POETIC_GPU_FONT_HPP

#include "Export.hpp"

#include <Luminous/GLResource.hpp>

#include <Nimble/Matrix3.hpp>

namespace Poetic
{
  class CPUFont;

  /// A common interface for all fonts that reside on the GPU
  class POETIC_API GPUFont : public Luminous::GLResource
  {
    public:
    GPUFont();
      virtual ~GPUFont();

      /// Returns the CPU font for this GPU font
      virtual CPUFont * cpuFont() = 0;

      /// Renders a string
      void render(const char * str);
      /// @copybrief render
      void render(const char * str, Nimble::Vector2 loc);
      /// @copybrief render
      void render(const char * str, float scale, Nimble::Vector2 loc);
      /// @copybrief render
      void render(const char * str, const Nimble::Matrix3 & transform);
      /// @copybrief render
      void render(const char * str, float x, float y);
      /// @copybrief render
      void render(const char * str, int n, const Nimble::Matrix3 & transform);

      /// @copybrief render
      void render(const std::string & str);
      /// @copybrief render
      void render(const std::string & str, const Nimble::Matrix3 & transform);
      /// @copybrief render
      void render(const std::string & str, const Nimble::Vector2 & location);

      /// @copybrief render
      void render(const std::wstring & str);
      /// @copybrief render
      void render(const std::wstring & str, const Nimble::Matrix3 & transform);
      /// @copybrief render
      void render(const std::wstring & str, const Nimble::Vector2 & location);

      /// @copybrief render
      void render(const wchar_t * str);
      /// @copybrief render
      void render(const wchar_t * str, const Nimble::Matrix3 & transform);
      /// @copybrief render
      void render(const wchar_t * str, int n,
                  const Nimble::Matrix3 & transform);

      /// Render text that is centered both horizontally and vertically.
      void renderCentered(const char * str, float x, float y);
      /// Render text that is centered both horizontally and vertically.
      void renderCentered(const char * str, const Nimble::Matrix3 & transform);
      /// Render text that is centered both horizontally and vertically.
      void renderCentered(const wchar_t * str, const Nimble::Matrix3 & transform);

      /// Renders the given string as lines. Used to approximate text that is too small to read.
      void renderLines(const char * str, Nimble::Vector2 loc);
    protected:
      /// The actual rendering function implemented in derived classes.
      virtual void internalRender(const char * str, int n, const Nimble::Matrix3 & transform) = 0;
      /// @copybrief internalRender
      virtual void internalRender(const wchar_t * str, int n, const Nimble::Matrix3 & transform) = 0;
  };

}

#endif
