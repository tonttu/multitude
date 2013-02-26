/* COPYRIGHT
 *
 * This file is part of FireView.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "Binning.hpp"

#include <Luminous/Luminous.hpp>

#include <Radiant/ColorUtils.hpp>
#include <Radiant/Color.hpp>
#include <Radiant/Trace.hpp>

#include <cstdio>

namespace FireView {

  Quadrangle::Quadrangle()
  {}

  Quadrangle::Quadrangle(Nimble::Vector2 a, Nimble::Vector2 b, Nimble::Vector2 c, Nimble::Vector2 d)
  {
    m_p[0] = a;
    m_p[1] = b;
    m_p[2] = c;
    m_p[3] = d;
  }

  bool Quadrangle::inside(Nimble::Vector2 p) const
  {
    for (int i = 0; i < 4; ++i) {

      int j = (i + 1) % 4;
      Nimble::Vector2 edge = m_p[j] - m_p[i];
      Nimble::Vector2 diff = p - m_p[i];

      edge.normalize();
      diff.normalize();

      float dot = Nimble::dot(edge.perpendicular(), diff);

      if (dot > 0.0f)
        return false;
    }

    return true;
  }

  //////////
  //////////

  Binning::Binning()
  {
  }

  void Binning::defineBin(const QString &name, const Quadrangle &region)
  {
    if(m_regions.contains(name))
      Radiant::warning("Binning::defineBin # bin '%s' already exists! Overwriting...", name.toUtf8().data());

    // Radiant::info("Binning::defineBin # %s %f %f", name.toUtf8().data(), region.m_p[2].x, region.m_p[0].x);

    m_regions[name] = region;
  }

  static const QString s_nilStr("unknown");

  const QString & Binning::classify(Nimble::Vector2 p) const
  {
    m_debugLastPoint = p;

    for(Regions::const_iterator i = m_regions.begin(); i != m_regions.end(); ++i) {

      if(i.value().inside(p))
        return i.key();
    }

    return s_nilStr;
  }

  void Binning::debugVisualize(int sx, int sy)
  {
#if 0
    /*
    Radiant::Color c[] = {
      Radiant::Color(1.f, 0.f, 0.f, 1.f),
      Radiant::Color(0.f, 1.f, 0.f, 1.f),
      Radiant::Color(0.f, 0.f, 1.f, 1.f),
      Radiant::Color(1.f, 1.f, 0.f, 1.f),
    };*/

    glColor3f(1.f, 1.f, 1.f);
    glBegin(GL_QUADS);

    foreach(Quadrangle q, m_regions) {

      for(int i = 0; i < 4; ++i) {

        // Convert chromaticity to RGB
        //Nimble::Vector3f XYZ(q.m_p[i].x, q.m_p[i].y, 1.f - q.m_p[i].x - q.m_p[i].y);

        const float Y = 1.0f;
        Nimble::Vector3f XYZ(Y * q.m_p[i].x / q.m_p[i].y, Y, Y * (1.f - q.m_p[i].x - q.m_p[i].y) / q.m_p[i].y);

        //Radiant::Color rgb = c[i];
        Nimble::Vector3f rgb;
        Radiant::ColorUtils::CIEXYZToRGB(XYZ, rgb);

        glColor3fv(rgb.data());
        glVertex2f(q.m_p[i].x * sx, (1.f - q.m_p[i].y) * sy);
      }
    }

    glEnd();

    glColor3f(1.f, 0.f, 0.f);
    glPointSize(2.f);
    glBegin(GL_POINTS);
    glVertex2f(m_debugLastPoint.x * sx, (1.f - m_debugLastPoint.y) * sy);
    glEnd();
#endif
  }

  void Binning::defineBins_ANSI_C78_377()
  {
    m_regions.clear();

    defineBin("2700K", Quadrangle(
                Nimble::Vector2(0.4593, 0.3944),
                Nimble::Vector2(0.4373, 0.3893),
                Nimble::Vector2(0.4562, 0.4260),
                Nimble::Vector2(0.4813, 0.4319)
                )
              );

    defineBin("3000K", Quadrangle(
                Nimble::Vector2(0.4373, 0.3893),
                Nimble::Vector2(0.4147, 0.3814),
                Nimble::Vector2(0.4299, 0.4165),
                Nimble::Vector2(0.4562, 0.4260)
                )
              );

    defineBin("3500K", Quadrangle(
                Nimble::Vector2(0.4147, 0.3814),
                Nimble::Vector2(0.3889, 0.3690),
                Nimble::Vector2(0.3996, 0.4015),
                Nimble::Vector2(0.4299, 0.4165)
                )
              );

    defineBin("4000K", Quadrangle(
                Nimble::Vector2(0.3898, 0.3716),
                Nimble::Vector2(0.3670, 0.3578),
                Nimble::Vector2(0.3736, 0.3874),
                Nimble::Vector2(0.4006, 0.4044)
                )
              );

    defineBin("4500K", Quadrangle(
                Nimble::Vector2(0.3670, 0.3578),
                Nimble::Vector2(0.3512, 0.3465),
                Nimble::Vector2(0.3548, 0.3736),
                Nimble::Vector2(0.3736, 0.3874)
                )
              );

    defineBin("5000K", Quadrangle(
                Nimble::Vector2(0.3515, 0.3487),
                Nimble::Vector2(0.3366, 0.3369),
                Nimble::Vector2(0.3376, 0.3616),
                Nimble::Vector2(0.3551, 0.3760)
                )
              );

    defineBin("5700K", Quadrangle(
                Nimble::Vector2(0.3366, 0.3369),
                Nimble::Vector2(0.3222, 0.3243),
                Nimble::Vector2(0.3207, 0.3462),
                Nimble::Vector2(0.3376, 0.3616)
                )
              );

    defineBin("6500K", Quadrangle(
                Nimble::Vector2(0.3221, 0.3261),
                Nimble::Vector2(0.3068, 0.3113),
                Nimble::Vector2(0.3028, 0.3304),
                Nimble::Vector2(0.3205, 0.3481)
                )
              );
  }

  void Binning::defineBins_CREE()
  {
    m_regions.clear();

    defineBin("WK", Quadrangle(
                Nimble::Vector2(0.283, 0.284),
                Nimble::Vector2(0.295, 0.297),
                Nimble::Vector2(0.298, 0.288),
                Nimble::Vector2(0.287, 0.276)
                )
              );

    defineBin("WA", Quadrangle(
                Nimble::Vector2(0.292, 0.306),
                Nimble::Vector2(0.295, 0.297),
                Nimble::Vector2(0.283, 0.284),
                Nimble::Vector2(0.279, 0.291)
                )
              );

    defineBin("WM", Quadrangle(
                Nimble::Vector2(0.295, 0.297),
                Nimble::Vector2(0.308, 0.311),
                Nimble::Vector2(0.310, 0.300),
                Nimble::Vector2(0.298, 0.288)
                )
              );

    defineBin("WB", Quadrangle(
                Nimble::Vector2(0.306, 0.322),
                Nimble::Vector2(0.308, 0.311),
                Nimble::Vector2(0.295, 0.297),
                Nimble::Vector2(0.292, 0.306)
                )
              );

    defineBin("WE", Quadrangle(
                Nimble::Vector2(0.301, 0.342),
                Nimble::Vector2(0.306, 0.322),
                Nimble::Vector2(0.292, 0.306),
                Nimble::Vector2(0.287, 0.321)
                )
              );

    defineBin("WN", Quadrangle(
                Nimble::Vector2(0.308, 0.311),
                Nimble::Vector2(0.317, 0.319),
                Nimble::Vector2(0.318, 0.308),
                Nimble::Vector2(0.310, 0.300)
                )
              );

    defineBin("WC", Quadrangle(
                Nimble::Vector2(0.316, 0.332),
                Nimble::Vector2(0.317, 0.319),
                Nimble::Vector2(0.308, 0.311),
                Nimble::Vector2(0.306, 0.322)
                )
              );

    defineBin("WF", Quadrangle(
                Nimble::Vector2(0.314, 0.355),
                Nimble::Vector2(0.316, 0.332),
                Nimble::Vector2(0.306, 0.322),
                Nimble::Vector2(0.301, 0.342)
                )
              );

    defineBin("WP", Quadrangle(
                Nimble::Vector2(0.317, 0.319),
                Nimble::Vector2(0.329, 0.330),
                Nimble::Vector2(0.329, 0.318),
                Nimble::Vector2(0.318, 0.308)
                )
              );

    defineBin("WD", Quadrangle(
                Nimble::Vector2(0.329, 0.345),
                Nimble::Vector2(0.329, 0.330),
                Nimble::Vector2(0.317, 0.319),
                Nimble::Vector2(0.316, 0.332)
                )
              );

    defineBin("WG", Quadrangle(
                Nimble::Vector2(0.329, 0.369),
                Nimble::Vector2(0.329, 0.345),
                Nimble::Vector2(0.316, 0.332),
                Nimble::Vector2(0.314, 0.355)
                )
              );

    defineBin("WJ", Quadrangle(
                Nimble::Vector2(0.329, 0.330),
                Nimble::Vector2(0.329, 0.345),
                Nimble::Vector2(0.346, 0.359),
                Nimble::Vector2(0.344, 0.342)
                )
              );

    defineBin("WH", Quadrangle(
                Nimble::Vector2(0.348, 0.384),
                Nimble::Vector2(0.346, 0.359),
                Nimble::Vector2(0.329, 0.345),
                Nimble::Vector2(0.329, 0.369)
                )
              );
  }

  void Binning::defineBins_TACTION7()
  {
    const int bins = 7;

    // These ranges are expected to exceed the actual range of x, so that we do not run out of bins

    const float xmin = 0.3014f;
    const float xmax = 0.322f;

    const float xstep = (xmax - xmin) / bins;

    // These are basically out of range, sorting is done based purely on X value
    const float ymin = 0.0f;
    const float ymax = 1.0f;

    char buf[32];

    for(int i = 0; i < bins; i++) {

      float xlower  = xmin + i * xstep;
      float xhigher = xmin + (i+1) * xstep;

      /* This is the "X" binning system, where each bin name starts with capital X. */
      sprintf(buf, "X%d", i + 1);

      defineBin(buf, Quadrangle(Nimble::Vector2(xhigher, ymax),
                                Nimble::Vector2(xhigher, ymin),
                                Nimble::Vector2(xlower, ymin),
                                Nimble::Vector2(xlower, ymax)));
    }
  }
  void Binning::defineBins_TACTION7AB()
  {
    Radiant::info("Binning::defineBins_TACTION7AB");

    const int bins = 7;

    // These ranges are expected to exceed the actual range of x, so that we do not run out of bins

    const float xmin = 0.3014f;
    const float xmax = 0.322f;

    const float xstep = (xmax - xmin) / bins;

    // These are basically out of range, sorting is done based purely on X value
    const float ymin = 0.0f;
    const float ymax = 1.0f;

    char buf[32];

    for(int i = 0; i < bins; i++) {

      float xlower  = xmin + i * xstep;
      float xhigher = xmin + (i+1) * xstep;
      float xmid = (xlower + xhigher) * 0.5f;

      /* This is the "X" binning system, where each bin name starts with capital X. */
      sprintf(buf, "X%dA", i + 1);

      defineBin(buf, Quadrangle(Nimble::Vector2(xmid, ymax),
                                Nimble::Vector2(xmid, ymin),
                                Nimble::Vector2(xlower, ymin),
                                Nimble::Vector2(xlower, ymax)));
      /* This is the "X" binning system, where each bin name starts with capital X. */
      sprintf(buf, "X%dB", i + 1);

      defineBin(buf, Quadrangle(Nimble::Vector2(xhigher, ymax),
                                Nimble::Vector2(xhigher, ymin),
                                Nimble::Vector2(xmid, ymin),
                                Nimble::Vector2(xmid, ymax)));
    }
  }

  }
