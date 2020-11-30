/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "KeyStone.hpp"

#include <Nimble/Rect.hpp>
#include <Nimble/Matrix4.hpp>

#include <stdio.h>
#include <stdlib.h>

namespace Nimble {

  KeyStone::KeyStone()
      : m_centerShift(0, 0),
      m_centerShiftSpan(10),
      m_useCenterShift(true),
      m_width(640),
      m_height(480),
      m_dpyWidth(1920),
      m_dpyHeight(1080),
      m_dpyCenter(960, 540),
      m_dpyX(0),
      m_dpyY(0),
      m_extra(0, 0, 0, 0),
      m_containedPixelCount(m_width * m_height),
      m_generation(0)
  {
    m_matrix.identity();
    m_matrixOut.identity();
    m_matrixExtension.identity();

    setVertices("0 0  640 0  640 480  0 480", 640, 480, 1920, 1080, 0, 0);
  }

  void KeyStone::setVertices(const char * str,
                             int w, int h,
                             int dpyw, int dpyh,
                             int dpyx, int dpyy)
  {
    char * ptr = (char *) str;

    Nimble::Vector2f vertices[4];

    float * corners = vertices[0].data();
    int i;

    for(i = 0; i < 8; i++)
      corners[i] = float(strtod(ptr, & ptr));

    setVertices(vertices, w, h, dpyw, dpyh, dpyx, dpyy);
  }

  void KeyStone::setVertices(const Nimble::Vector2 * vertices,
                             int w, int h,
                             int dpyw, int dpyh,
                             int dpyx, int dpyy)
  {
    int i;

    m_lensCorrection.setCameraResolution(w, h);

    for(i = 0; i < 4; i++) {

      Nimble::Vector2f tmp = vertices[i];

      tmp.x = std::min(tmp.x, (float) (w - 1));
      tmp.y = std::min(tmp.y, (float) (h - 1));

      m_originals[i] = tmp;
    }
    m_width = w;
    m_height = h;

    setOutputGeometry(dpyw, dpyh, dpyx, dpyy);
  }

  void KeyStone::setOutputGeometry(unsigned w, unsigned h, int x, int y)
  {
    m_dpyWidth  = w;
    m_dpyHeight = h;

    m_dpyX = x;
    m_dpyY = y;

    m_dpyCenter.make(m_dpyX + m_dpyWidth * 0.5f,
                     m_dpyY + m_dpyHeight * 0.5f);

    calculateMatrix();
    updateLimits();
  }


  void KeyStone::calculateMatrix()
  {
    for(int i = 0; i < 4; i++) {
      Nimble::Vector2f tmp = m_lensCorrection.correct(m_originals[i]);
      m_vertices[i].make(tmp.x / m_width, tmp.y / m_height);
    }

    m_matrix = projectionMatrix(m_vertices);

    bool ok = true;
    m_matrix = m_matrix.inverse(&ok);

    Matrix3 tmp;
    tmp.identity();
    tmp[0][0] = 1.0f / m_width;
    tmp[1][1] = 1.0f / m_height;

    m_matrix = m_matrix * tmp;

    tmp.identity();

    tmp[0][0] = (float)m_dpyWidth;
    tmp[1][1] = (float)m_dpyHeight;

    tmp[0][2] = (float)m_dpyX;
    tmp[1][2] = (float)m_dpyY;

    m_matrixOut = tmp * m_matrixExtension * m_matrix;

    updated();
  }

  Nimble::Vector2 KeyStone::project(const Nimble::Vector2 & v) const
  {
    // "p.z" is really "p.w", we need to do the "perspective" correction here.
    Vector3 p = m_matrixOut * Nimble::Vector3(m_lensCorrection.correct(v), 1);
    Nimble::Vector2f res(p.x / p.z, p.y / p.z);
    if(m_useCenterShift) {
      float dist = (res - m_dpyCenter).length();
      if(dist < m_centerShiftSpan) {
        float weight = 0.5f + 0.5f * cosf(static_cast<float>(Math::PI) * dist / m_centerShiftSpan);
        Nimble::Vector2f move = powf(weight, 1.0f) * m_centerShift;
        res += move;
      }
    }

    return res;
  }

  Nimble::Vector2 KeyStone::project01(const Nimble::Vector2 & v) const
  {
    // "p.z" is really "p.w", we need to do the "perspective" correction here.
    Vector3 p = m_matrix * Nimble::Vector3(m_lensCorrection.correct(v), 1);
    return Vector2(p.x / p.z, p.y / p.z);
  }

  Nimble::Vector2 KeyStone::projectInverse(const Nimble::Vector2 & v) const
  {
    Matrix3 m = m_matrixOut.inverse();
    return m.project(v);
  }

  void  KeyStone::moveCorner(Nimble::Vector2 loc)
  {
    int index = closestCorner(loc);
    m_originals[index] = loc;

    calculateMatrix();
    updateLimits();
  }

  int KeyStone::closestCorner(Nimble::Vector2 loc) const
  {
    float best = (m_originals[0] - loc).length();
    int index = 0;

    for(int i = 1; i < 4; i++) {
      float d2 = (m_originals[i] - loc).length();
      if(best > d2) {
        best = d2;
        index = i;
      }
    }

    return index;
  }


  void KeyStone::flipHorizontal()
  {
    Nimble::Vector2 tmp = m_originals[1];
    m_originals[1] = m_originals[0];
    m_originals[0] = tmp;

    tmp = m_originals[3];
    m_originals[3] = m_originals[2];
    m_originals[2] = tmp;

    calculateMatrix();
    updateLimits();
  }

  void KeyStone::flipVertical()
  {
    Nimble::Vector2 tmp = m_originals[1];
    m_originals[1] = m_originals[2];
    m_originals[2] = tmp;

    tmp = m_originals[3];
    m_originals[3] = m_originals[0];
    m_originals[0] = tmp;

    calculateMatrix();
    updateLimits();
  }

  void KeyStone::rotate(int turns)
  {
    for(int t = 0; t < turns; t++) {
      Nimble::Vector2 v = m_originals[0];

      for(int i = 0; i < 3; i++)
        m_originals[i] = m_originals[i + 1];

      m_originals[3] = v;

      // Swap output width & height
      int tmp = m_dpyWidth;
      m_dpyWidth = m_dpyHeight;
      m_dpyHeight = tmp;

      calculateMatrix();
    }
  }

  void KeyStone::getCornerOrdering(int indices[4])
  {
    indices[0] = closestCorner(Nimble::Vector2(0.f, 0.f));
    indices[1] = closestCorner(Nimble::Vector2(static_cast<float>(m_width), 0.f));
    indices[2] = closestCorner(Nimble::Vector2(static_cast<float>(m_width), static_cast<float>(m_height)));
    indices[3] = closestCorner(Nimble::Vector2(0.f, static_cast<float>(m_height)));
  }

  void KeyStone::addExtra(int index, float v)
  {
    m_extra[index] += v;

    if(m_extra[index] < 0.0f)
      m_extra[index] = 0.0f;

    if(m_extra[index] >= 100.0f)
      m_extra[index] = 100.0f;
  }

  Nimble::Rect KeyStone::outputBounds()
  {
    return Nimble::Rect(float(m_dpyX), float(m_dpyY), float(m_dpyX + m_dpyWidth), float(m_dpyY + m_dpyHeight));
  }

  void KeyStone::setLensParam(int i, float v)
  {
    m_lensCorrection.setParam(i, v);
    calculateMatrix();
    updateLimits();
  }

  void KeyStone::calibrateOutput(const Nimble::Vector2 * targets,
                                 const Nimble::Vector2 * real,
                                 const Nimble::Vector2 * center)
  {
    int i;


    // Do the real matrix calculation

    std::array<Nimble::Vector2, 4> tnorm; // Target points in [0-1] space
    std::array<Nimble::Vector2, 4> rnorm; // Real points in [0-1] space
    Nimble::Vector2 rcnorm(0, 0); // Real center point in [0-1] space

    Matrix3 tmp;
    tmp.identity();

    tmp[0][0] = (float) m_dpyWidth;
    tmp[1][1] = (float) m_dpyHeight;

    tmp[0][2] = (float) m_dpyX;
    tmp[1][2] = (float) m_dpyY;

    Nimble::Matrix3 backToNorm = (tmp * m_matrixExtension).inverse();

    for(i = 0; i < 4; i++) {
      tnorm[i] = backToNorm.project(targets[i]);
      rnorm[i] = backToNorm.project(real[i]);
    }

    // Reverse engineer the center location:

    if(center) {
      float dist = (*center - m_dpyCenter).length();

      float weight = 0.5f + 0.5f * cosf(static_cast<float>(Math::PI) * dist / m_centerShiftSpan);
      rcnorm = backToNorm.project(*center - weight * m_centerShift);

      printf("rcnorm = %f %f dpyc = %f %f\n",
             rcnorm.x, rcnorm.y, m_dpyCenter.x, m_dpyCenter.y);
    }

    Nimble::Matrix3 realToNormalized = projectionMatrix(rnorm).inverse();
    Nimble::Matrix3 normalizedToTarget = projectionMatrix(tnorm);

    m_matrixExtension =
        m_matrixExtension * normalizedToTarget * realToNormalized;

    calculateMatrix();

    if(center) {

      // rcnorm.x *= m_width;
      // rcnorm.y *= m_height;

      rcnorm = m_matrix.inverse().project(rcnorm);

      Nimble::Vector2f projcenter = m_matrixOut.project(rcnorm);
      m_centerShift = m_dpyCenter - projcenter;
      m_centerShiftSpan = (m_dpyCenter - targets[0]).length();
      printf("KeyStone::calibrateOutput # pc = [%.2f %.2f] offset = [%.2f %.2f] span = %f\n",
             projcenter.x, projcenter.y,
             m_centerShift.x, m_centerShift.y, m_centerShiftSpan);
      fflush(0);
    }
  }

  void KeyStone::setOutputExtension(const Nimble::Matrix3 & m)
  {
    m_matrixExtension = m;
    calculateMatrix();
  }

  /// @todo KeyStone class shouldn't have anything to do with image processing
  ///       Split this to two classes?
  void KeyStone::updateLimits()
  {
    updateLimits(m_limits);

    Nimble::Vector2f tl = topLeft();
    Nimble::Vector2f tr = topRight();

    Nimble::Vector2f bl = bottomLeft();
    Nimble::Vector2f br = bottomRight();

    float ksw = (tr.x - tl.x + br.x - bl.x) * 0.5f;
    float ksh = (bl.y - tl.y + br.y - tr.y) * 0.5f;

    Vector4 scaledExtra(m_extra[0] / ksw, m_extra[1] / ksw,
                        m_extra[2] / ksh, m_extra[3] / ksh);

    updateLimits(m_extraLimits, & scaledExtra);
  }


  /** Calculates the projection matrix. See Paul Heckbert's master's
   * thesis, pages 19-21. */

  Matrix3 KeyStone::projectionMatrix(const std::array<Nimble::Vector2f, 4> & vertices)
  {
    return Nimble::Matrix3::makeProjectionMatrix(vertices);
  }


  void KeyStone::updateLimits(std::vector<Nimble::Vector2i> & limits,
                              const Vector4 * offsets)  {
    limits.resize(m_height);
    memset(static_cast<void*>(limits.data()), 0, limits.size() * sizeof(Nimble::Vector2i));

    Rect bounds(0, 0, 1, 1);

    if(offsets) {

      Nimble::Vector2f tests[4] = {
        m_matrix.project(Vector2(float(m_width), float(m_height / 2))),
        m_matrix.project(Vector2(float(0), float(m_height / 2))),
        m_matrix.project(Vector2(float(m_width / 2), float(0))),
        m_matrix.project(Vector2(float(m_width / 2), float(m_height)))
      };

      int i;
      Rect b2(tests[0], tests[0]);

      for(i = 1; i < 4; i++)
        b2.expand(tests[i]);

      for(i = 0; i < 4; i++) {
        Nimble::Vector2f test = tests[i];

        if(test.y == b2.low().y)
          bounds.low().y -= (*offsets)[i];
        else if(test.y == b2.high().y)
          bounds.high().y += (*offsets)[i];
        else if(test.x == b2.low().x)
          bounds.low().x -= (*offsets)[i];
        else if(test.x == b2.high().x)
          bounds.high().x += (*offsets)[i];
      }
    }

    int count = 0;

    for(int y = 0; y < m_height; y++) {

      int first = 0;
      int last = 0;

      bool inside = false;

      for(int x = 0; x < m_width; x++) {

        Nimble::Vector2f v1 = m_lensCorrection.correct(Vector2(float(x), float(y)));
        v1 = m_matrix.project(v1);

        bool in = bounds.contains(v1);

        if(!in) {
          if(inside) {
            last = x;
            inside = false;
            break;
          }
        }
        else {
          count++;
          if(!inside) {
            first = x;
            inside = true;
          }
        }
      }

      if(inside)
        last = m_width - 1;

      int wid = last - first;
      limits[y].make(first, wid);
    }

    if(!offsets)
      m_containedPixelCount = count;

    m_boundsROI.clear();
    for (int y=0; y < (int) m_limits.size(); ++y) {
      if (!m_limits[y][1])
        continue;
      m_boundsROI.expand(Nimble::Rect(Nimble::Vector2(m_limits[y][0], y), Nimble::Vector2(m_limits[y][0]+m_limits[y][1], y)));
    }

    updated();
  }
}
