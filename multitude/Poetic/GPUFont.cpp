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

#include "GPUFont.hpp"

#include "CPUFont.hpp"

#include <Radiant/StringUtils.hpp>

#include <string.h>

namespace Poetic
{

  void GPUFont::render(const char * str, float x, float y)
  {
    Nimble::Matrix3 transform;
    transform.identity();
    transform[0][2] = x;
    transform[1][2] = y;

    internalRender(str, strlen(str), transform);
  }

  void GPUFont::render(const char * str, const Nimble::Matrix3 & m)
  {
    internalRender(str, strlen(str), m);
  }

  void GPUFont::render(const wchar_t * str, const Nimble::Matrix3 & m)
  {
    internalRender(str, wcslen(str), m);
  }

  void GPUFont::render(const wchar_t * str, int n,
                       const Nimble::Matrix3 & transform)
  {
    internalRender(str, n, transform);
  }

  void GPUFont::render(const std::string & str, const Nimble::Matrix3 & transform)
  {
    internalRender(str.c_str(), str.size(), transform);
  }

  void GPUFont::render(const std::string & str, const Nimble::Vector2 & location)
  {
    internalRender(str.c_str(), str.size(),
		   Nimble::Matrix3::translate2D(location));
  }

  void GPUFont::render(const std::wstring & str, const Nimble::Vector2 & location)
  {
    internalRender(str.c_str(), str.size(),
		   Nimble::Matrix3::translate2D(location));
  }

  void GPUFont::render(const std::string & str)
  {
    Nimble::Matrix3 transform;
    transform.identity();
    internalRender(str.c_str(), str.size(), transform);
  }

  void GPUFont::render(const char * str)
  {
    Nimble::Matrix3 transform;
    transform.identity();
    internalRender(str, strlen(str), transform);
  }

  void GPUFont::render(const wchar_t * str)
  {
    Nimble::Matrix3 transform;
    transform.identity();
    internalRender(str, wcslen(str), transform);
  }

  void GPUFont::render(const std::wstring & str)
  {
    Nimble::Matrix3 transform;
    transform.identity();
    internalRender(str.c_str(), str.length(), transform);
  }

  void GPUFont::render(const char * str, Nimble::Vector2 loc)
  {
    Nimble::Matrix3 transform;
    transform.identity();
    transform[0][2] = loc.x;
    transform[1][2] = loc.y;
    internalRender(str, strlen(str), transform);    
  }

  void GPUFont::render(const char * str, float scale, Nimble::Vector2 loc)
  {
    Nimble::Matrix3 transform;
    transform.identity();
    transform[0][0] = scale;
    transform[1][1] = scale;
    transform[0][2] = loc.x;
    transform[1][2] = loc.y;
    internalRender(str, strlen(str), transform);    
  }

  void GPUFont::render(const char * str, int n,
                       const Nimble::Matrix3 & transform)
  {
    internalRender(str, n, transform);
  }

  void GPUFont::render(const std::wstring & str, const Nimble::Matrix3 & transform)
  {
    internalRender(str.c_str(), str.length(), transform);
  }


  void GPUFont::renderCentered(const char * str, float x, float y)
  {
    BBox bb;

    cpuFont()->bbox(str, bb);
    // usually bb.low() != (0,0)
    Nimble::Vector2 offset = bb.center();

    render(str, x - offset.x, y + offset.y);
  }

  void GPUFont::renderCentered(const char * str,
			       const Nimble::Matrix3 & transform)
  {
    BBox bb;

    cpuFont()->bbox(str, bb);
    Nimble::Vector2 center = bb.center();

    render(str, transform * Nimble::Matrix3::translate2D(-center.x, center.y));
  }

  void GPUFont::renderCentered(const wchar_t * str,
			       const Nimble::Matrix3 & transform)
  {
    BBox bb;

    cpuFont()->bbox(str, bb);
    Nimble::Vector2 center = bb.center();

    render(str, transform * Nimble::Matrix3::translate2D(-center.x, center.y));
  }


  void GPUFont::renderLines(const char * str, Nimble::Vector2 loc)
  {
    float lh = cpuFont()->lineHeight();

    int left = strlen(str);
    int linelen = Radiant::StringUtils::strchrnul(str, '\n') - str;

    while(left) {
          
      if(linelen) {
        render(str, linelen, Nimble::Matrix3::translate2D(loc));
      }

      loc.y += lh;
          
      str += linelen + 1;

      if(linelen >= left)
        left = 0;
      else {
        left = strlen(str);
        linelen = Radiant::StringUtils::strchrnul(str, '\n') - str;
      }
    }
    
  }
}
