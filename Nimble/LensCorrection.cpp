/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "LensCorrection.hpp"

namespace Nimble {

  LensCorrection::LensCorrection()
    : m_center(320, 240),
      m_radiusInv(1.0f / std::sqrt(320 * 320 + 240 * 240.0f)),
      m_params(0, 0, 0, 1)
  {
    setParams(0.0f, 0.1f, 0.0f);
  }

  void LensCorrection::setCameraResolution(int w, int h)
  {
    m_center.make((w - 1) * 0.5f, (h - 1) * 0.5f);
    m_radiusInv = 1.0f / m_center.length();
  }

  Nimble::Vector2f LensCorrection::correct(Vector2 loc) const
  {
    Nimble::Vector2f local  = loc - m_center;
    float r1 = local.length() * m_radiusInv;

    if(r1 < 0.0001f)
      return m_center;

    float r2 = r1 * r1;
    float r3 = r2 * r1;
    float r4 = r2 * r2;

    float rcorr = 
      m_params[0] * r4 + m_params[1] * r3 + 
      m_params[2] * r2 + m_params[3] * r1;

    return local * (rcorr / r1) + m_center;
  }

}
