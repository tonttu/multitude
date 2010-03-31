/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include <Luminous/TCBSpline.hpp>

#include <Radiant/Trace.hpp>

#include <GL/glew.h>
#include <cassert>

namespace Luminous {

  TCBSpline2::TCBSpline2(int segments,
      const std::vector<float> & time, 
      const std::vector<Nimble::Vector2f> & points, 
      const std::vector<float> & tension,
      const std::vector<float> & continuity, 
      const std::vector<float> & bias)
    : m_segments(segments),
    m_time(time),
    m_points(points),
    m_tension(tension),
    m_continuity(continuity),
    m_bias(bias)
  {
    assert(points.size() == m_segments + 1);

    rebuildPolys();
  }

  TCBSpline2::~TCBSpline2()
  {}

  void TCBSpline2::rebuildPolys()
  {
    size_t segments = m_points.size() - 1;

    m_A.resize(segments);
    m_B.resize(segments);
    m_C.resize(segments);
    m_D.resize(segments);

    // Treat the first point as if it occured twice
    computePoly(0, 0, 1, 2);

    for(size_t i = 1; i < segments - 1; i++)
      computePoly(i - 1, i, i + 1, i + 2);

    // Treat the last point as if it occured twice
    computePoly(segments - 2, segments - 1, segments, segments);
  }

  void TCBSpline2::transform(const Nimble::Matrix3f & m)
  {
    for(size_t i = 0; i < m_points.size(); i++)
      m_points[i] = (m * m_points[i]).xy();

    rebuildPolys();
  }

  void TCBSpline2::computePoly(int i0, int i1, int i2, int i3)
  {
    Nimble::Vector2f diff = m_points[i2] - m_points[i1];
    float dt = m_time[i2] - m_time[i1];

    // P1
    float fOmt0 = 1.f - m_tension[i1];
    float fOmc0 = 1.f - m_continuity[i1];
    float fOpc0 = 1.f + m_continuity[i1];
    float fOmb0 = 1.f - m_bias[i1];
    float fOpb0 = 1.f + m_bias[i1];
    float fAdj0 = 2.f * dt / (m_time[i2] - m_time[i0]);
    float fOut0 = 0.5f * fAdj0 * fOmt0 * fOpc0 * fOpb0;
    float fOut1 = 0.5f * fAdj0 * fOmt0 * fOmc0 * fOmb0;

    // Tangent out at P1
    Nimble::Vector2f tOut = fOut1 * diff + fOut0 * (m_points[i1] - m_points[i0]);

    // P2
    float fOmt1 = 1.f - m_tension[i2];
    float fOmc1 = 1.f - m_continuity[i2];
    float fOpc1 = 1.f + m_continuity[i2];
    float fOmb1 = 1.f - m_bias[i2];
    float fOpb1 = 1.f + m_bias[i2];
    float fAdj1 = 2.f * dt / (m_time[i3] - m_time[i1]);
    float fIn0 = 0.5f * fAdj1 * fOmt1 * fOmc1 * fOpb1;
    float fIn1 = 0.5f * fAdj1 * fOmt1 * fOpc1 * fOmb1;

    // Tangent in at P2
    Nimble::Vector2f tIn = fIn1 * (m_points[i3] - m_points[i2]) + fIn0 * diff;

    m_A[i1] = m_points[i1];
    m_B[i1] = tOut;
    m_C[i1] = 3.f * diff - 2.f * tOut - tIn;
    m_D[i1] = -2.f * diff + tOut + tIn;
  }

  Nimble::Vector2f TCBSpline2::value(float t) const
  {
    int key;
    float dt;
    getKeyInfo(t, key, dt);

    dt /= (m_time[key + 1] - m_time[key]);

    Nimble::Vector2f result = m_A[key] + dt * (m_B[key] + dt * (m_C[key] + dt * m_D[key]));

    return result;
  }

  void TCBSpline2::getKeyInfo(float t, int & key, float & dt) const
  {
    if(t <= m_time[0]) {
      key = 0;
      dt = 0.f;
    } else if (t >= m_time[m_segments]) {
      key = m_segments - 1;
      dt = m_time[m_segments] - m_time[m_segments - 1];
    } else {
      for(size_t i = 0; i < m_segments; i++) {
        if(t < m_time[i + 1]) {
          key = i;
          dt = t - m_time[i];
          break;
        }
      }
    }
  }

  Nimble::Vector2f TCBSpline2::firstDerivative(float t) const
  {
    int key;
    float dt;
    getKeyInfo(t, key, dt);

    dt /= (m_time[key + 1] - m_time[key]);

    Nimble::Vector2f result = m_B[key] + dt * (m_C[key] * 2.f) + m_D[key] * (3.f * dt);

    return result;
  }

  void TCBSpline2::render() const
  {
    glBegin(GL_LINE_STRIP);
    for(float t = 0.f; t <= m_time.back(); t += 5.f) {
      Nimble::Vector2f p = value(t);

      glVertex2fv(p.data());
    }
    glEnd();
  }

  void TCBSpline2::renderQuads(float step, float thickness, const Nimble::Matrix3f & m) const
  {
    const float len = m_time.back();
    const float ht = 0.5f * thickness;

    glBegin(GL_QUAD_STRIP);

    for(float t = 0.f; t <= len; t += step) {
      Nimble::Vector2f p = value(t);
      Nimble::Vector2f d = firstDerivative(t);
      Nimble::Vector2f n(-d.y, d.x);
      n.normalize();

      Nimble::Vector2f v0 = p + ht * n;
      Nimble::Vector2f v1 = p - ht * n;

      v0 = (m * v0).xy();
      v1 = (m * v1).xy();

      glTexCoord2f(t / len, 0);
      glVertex2fv(v0.data());
      glTexCoord2f(t / len, 1);
      glVertex2fv(v1.data());
    }

    glEnd();
  }

}

