/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include <Radiant/ColorUtils.hpp>

namespace Radiant
{

  namespace ColorUtils
  {

    using namespace Nimble;

    void rgbTohsv(float r, float g, float b, float & h, float & s, float & v)
    {
      h = s = v = 0.0f;

      // Find the amount of white
      const float   min = Math::Min(r, Math::Min(g, b));

      // Find the dominant primary
      const float   max = Math::Max(r, Math::Max(g, b));

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

    void rgbTohsv(Vector3f & rgb, Vector3f & hsv)
    {
      rgbTohsv(rgb[0], rgb[1], rgb[2], hsv[0], hsv[1], hsv[2]);
    }

    void hsvTorgb(float h, float s, float v, float & r, float & g, float & b)
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

    void hsvTorgb(Vector3f & hsv, Vector3f & rgb)
    {
      hsvTorgb(hsv[0], hsv[1], hsv[2], rgb[0], rgb[1], rgb[2]);
    }

  }

}
