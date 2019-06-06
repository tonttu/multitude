/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ColorUtils.hpp"

#include <Nimble/Matrix3.hpp>

#include <cassert>

namespace Radiant
{

  void ColorUtils::rgbTohsv(float r, float g, float b, float & h, float & s, float & v)
  {
    h = s = v = 0.0f;

    // Find the amount of white
    const float   min = std::min(r, std::min(g, b));

    // Find the dominant primary
    const float   max = std::max(r, std::max(g, b));

    // Compute saturation and value
    const float   delta = max - min;
    s = ((max == 0.0f) ? 0.0f : delta / max);
    v = max;

    // Compute hue
    if(s != 0.0f)
    {
      if(r == max)
      {
        h = (g - b) / delta;
      }
      else if(g == max)
      {
        h = 2.0f + (b - r) / delta;
      }
      else if(b == max)
      {
        h = 4.0f + (r - g) / delta;
      }

      h *= 60.0f;
      if(h < 0.0f)
      {
        h += 360.0f;
      }
      h /= 360.0f;
    }
  }

  void ColorUtils::rgbTohsv(const Nimble::Vector3f & rgb, Nimble::Vector3f & hsv)
  {
    rgbTohsv(rgb[0], rgb[1], rgb[2], hsv[0], hsv[1], hsv[2]);
  }

  Color ColorUtils::rgbTohsv(const Color & rgb)
  {
    Color hsv;
    hsv.a = rgb.a;
    ColorUtils::rgbTohsv(rgb.r, rgb.g, rgb.b, hsv.r, hsv.g, hsv.b);
    return hsv;
  }

  void ColorUtils::hsvTorgb(float h, float s, float v, float & r, float & g, float & b)
  {
    r = g = b = 0.0f;

    if(s == 0.0f)
      // Grey
    {
      r = g = b = v;
    }
    else
      // Color
    {
      h = (h == 1.0f) ? 0.0f : h * 6.0f;

      const int     i = int(h);
      const float   f = h - i;

      const float   aa = v * (1.0f - s);
      const float   bb = v * (1.0f - (s * f));
      const float   cc = v * (1.0f - (s * (1.0f - f)));

      switch (i)
      {
      case 0: r = v,  g = cc, b = aa; break;

      case 1: r = bb, g = v,  b = aa; break;

      case 2: r = aa, g = v,  b = cc; break;

      case 3: r = aa, g = bb, b = v;  break;

      case 4: r = cc, g = aa, b = v;  break;

      case 5: r = v,  g = aa, b = bb; break;

      default: assert(0);
      }
    }
  }

  void ColorUtils::hsvTorgb(const Nimble::Vector3f & hsv, Nimble::Vector3f & rgb)
  {
    hsvTorgb(hsv[0], hsv[1], hsv[2], rgb[0], rgb[1], rgb[2]);
  }

  Color ColorUtils::hsvTorgb(const Color & hsv)
  {
    Color rgb;
    rgb.a = hsv.a;
    hsvTorgb(hsv.r, hsv.g, hsv.b, rgb.r, rgb.g, rgb.b);
    return rgb;
  }

  void ColorUtils::colorBalance(VideoImage & img, Nimble::Vector3f rgbCoeff)
  {
    size_t offset = 0;

    assert(img.m_format == IMAGE_RGB);

    for(int y = 0; y < img.height(); ++y) {
      for(int x = 0; x < img.width(); ++x) {

        for(int i = 0; i < 3; ++i) {
          unsigned char & v = img.m_planes[0].m_data[offset++];
          v = static_cast<unsigned char> (Nimble::Math::Clamp<int>(v * rgbCoeff[i], 0, 255));
        }
      }
    }
  }

  const Nimble::Matrix3f g_tristimulusMatrix(0.4142f, 0.3576f, 0.1805f,
                                           0.2126f, 0.7152f, 0.0722f,
                                           0.0193f, 0.1192f, 0.9505f);


  void ColorUtils::rgbToCIEXYZ(const Nimble::Vector3f &rgb, Nimble::Vector3f &cie)
  {
    cie = g_tristimulusMatrix * rgb;
  }

  void ColorUtils::CIEXYZToRGB(const Nimble::Vector3f & cie, Nimble::Vector3f & rgb)
  {
    rgb = g_tristimulusMatrix.inverse() * cie;
  }

  void ColorUtils::CIEXYZtoCIEXYY(const Nimble::Vector3f & xyz, Nimble::Vector3f & xyy)
  {
    xyy.z = xyz.y;
    float sum = xyz.sum();
    xyy.x = xyz.x / sum;
    xyy.y = xyz.y / sum;
  }

  void ColorUtils::CIEXYYtoCIEXYZ(const Nimble::Vector3f & xyy, Nimble::Vector3f & xyz)
  {
    xyz.x = xyy.x * (xyy.z / xyy.y);
    xyz.y = xyy.z;
    xyz.z = (1.0f - xyy.x - xyy.y) * (xyy.z / xyy.y);
  }
}
