/* COPYRIGHT
 */

#include "GPUFont.hpp"

#include "CPUFont.hpp"

#include <Radiant/StringUtils.hpp>

#include <string.h>

namespace Poetic
{

  GPUFont::GPUFont()
  {
    setPersistent(true);
  }

  GPUFont::~GPUFont()
  {}

  void GPUFont::render(const char * str, float x, float y)
  {
    Nimble::Matrix3 transform;
    transform.identity();
    transform[0][2] = x;
    transform[1][2] = y;

    internalRender(str, (int) strlen(str), transform);
  }

  void GPUFont::render(const char * str, const Nimble::Matrix3 & m)
  {
    internalRender(str, (int) strlen(str), m);
  }

  void GPUFont::render(const wchar_t * str, const Nimble::Matrix3 & m)
  {
    internalRender(str, (int) wcslen(str), m);
  }

  void GPUFont::render(const wchar_t * str, int n,
                       const Nimble::Matrix3 & transform)
  {
    internalRender(str, n, transform);
  }

  void GPUFont::render(const QString & str, const Nimble::Matrix3 & transform)
  {
    std::wstring wstr = str.toStdWString();
    internalRender(wstr.c_str(), (int) wstr.size(), transform);
  }

  void GPUFont::render(const QString & str, const Nimble::Vector2 & location)
  {
    std::wstring wstr = str.toStdWString();
    internalRender(wstr.c_str(), (int) wstr.size(),
           Nimble::Matrix3::translate2D(location));
  }

  void GPUFont::render(const QString & str)
  {
    Nimble::Matrix3 transform;
    transform.identity();
    std::wstring wstr = str.toStdWString();
    internalRender(wstr.c_str(), (int) wstr.size(), transform);
  }

  void GPUFont::render(const char * str)
  {
    Nimble::Matrix3 transform;
    transform.identity();
    internalRender(str, (int) strlen(str), transform);
  }

  void GPUFont::render(const wchar_t * str)
  {
    Nimble::Matrix3 transform;
    transform.identity();
    internalRender(str, (int) wcslen(str), transform);
  }

  void GPUFont::render(const char * str, Nimble::Vector2 loc)
  {
    Nimble::Matrix3 transform;
    transform.identity();
    transform[0][2] = loc.x;
    transform[1][2] = loc.y;
    internalRender(str, (int) strlen(str), transform);
  }

  void GPUFont::render(const char * str, float scale, Nimble::Vector2 loc)
  {
    Nimble::Matrix3 transform;
    transform.identity();
    transform[0][0] = scale;
    transform[1][1] = scale;
    transform[0][2] = loc.x;
    transform[1][2] = loc.y;
    internalRender(str, (int) strlen(str), transform);
  }

  void GPUFont::render(const char * str, int n,
                       const Nimble::Matrix3 & transform)
  {
    internalRender(str, n, transform);
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

    int left, linelen;

    if(str) {
      left = (int) strlen(str);
      linelen = strchr(str, '\n') - str;
    }
    else {
      left = 0;
      linelen = 0;
    }

    while(left) {

      if(linelen) {
        render(str, linelen, Nimble::Matrix3::translate2D(loc));
      }

      loc.y += lh;

      str += linelen + 1;

      if(linelen >= left)
        left = 0;
      else if(str) {
        left = (int) strlen(str);
        linelen = strchr(str, '\n') - str;
      }
      else {
        left = 0;
        linelen = 0;
      }
    }

  }
}
