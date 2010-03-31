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

#include <Luminous/GLResource.hpp>

#include <Poetic/Export.hpp>

#include <Nimble/Matrix3.hpp>

namespace Poetic
{
  class CPUFont;

  /// A common interface for all fonts that reside on the GPU
  class POETIC_API GPUFont : public Luminous::GLResource
  {
    public:
      virtual ~GPUFont() {}

      virtual CPUFont * cpuFont() = 0;

      void render(const char * str);
      void render(const char * str, Nimble::Vector2 loc);
      void render(const char * str, float scale, Nimble::Vector2 loc);
      void render(const char * str, const Nimble::Matrix3 & transform);
      void render(const char * str, float x, float y);
      void render(const char * str, int n, const Nimble::Matrix3 & transform);

      void render(const std::string & str);
      void render(const std::string & str, const Nimble::Matrix3 & transform);
      void render(const std::string & str, const Nimble::Vector2 & location);

      void render(const std::wstring & str);
      void render(const std::wstring & str, const Nimble::Matrix3 & transform);
      void render(const std::wstring & str, const Nimble::Vector2 & location);


      void render(const wchar_t * str);
      void render(const wchar_t * str, const Nimble::Matrix3 & transform);
      void render(const wchar_t * str, int n,
                  const Nimble::Matrix3 & transform);

      /** Render text that is centered both horizontally and
	  vertically. */
      void renderCentered(const char * str, float x, float y);
      /** Render text that is centered both horizontally and
	  vertically. */
      void renderCentered(const char * str, const Nimble::Matrix3 & transform);
      /** Render text that is centered both horizontally and
	  vertically. */
      void renderCentered(const wchar_t * str,
			  const Nimble::Matrix3 & transform);

      void renderLines(const char * str, Nimble::Vector2 loc);
    protected:
      // The actual rendering methods, override these in derived classes
      virtual void internalRender(const char * str, int n, const Nimble::Matrix3 & transform) = 0;
      virtual void internalRender(const wchar_t * str, int n, const Nimble::Matrix3 & transform) = 0;
  };

}

#endif
