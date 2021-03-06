/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_LENS_CORRECTION_HPP
#define NIMBLE_LENS_CORRECTION_HPP

#include "Export.hpp"
#include "Vector4.hpp"

namespace Nimble {

  /// Camera distortion correction
  /** Correct camera lens distortion. Uses simple and fast polynomial
      correction to transform coordinates in the distorted input image
      to more ideal coordinates.

      The correction is based on camera resolution and a third-order
      polynomial mapping:

      http://www.panotools.info/mediawiki/index.php?title=Lens_correction_model

      @author Tommi Ilmonen
  */
  /// @todo implement better lens correction (grid maybe)

  class NIMBLE_API LensCorrection
  {
  public:
    LensCorrection();
    ~LensCorrection() {}

    /// Sets the camera resolution that will be used for camera correction
    void setCameraResolution(int w, int h);

    /// Performs barrel distortion correction on the given vector
    Nimble::Vector2f correct(Vector2 loc) const;

    /// Set the correction mapping to identity
    void setIdentity()
    { setParams(0.0f, 0.0f, 0.0f); }

    /// Sets all the lens correction parameters
    void setParams(float a, float b, float c)
    { m_params.make(a, b, c, 1.0f - (a + b + c)); }

    /// Sets all the lens correction parameters
    void setParams(const float * abc)
    { setParams(abc[0], abc[1], abc[2]); }

    /// Sets a single floating value
    void setParam(int i, float v)
    { 
      m_params[i] = v;
      m_params[3] = 1.0f - (m_params.x + m_params.y + m_params.z);
    }

    /// Access the correction parameters
    /// The fourth element of the vector is the multiplier that normalizes the
    /// calibration, based on the three lens correction parameters.
    /// @return Correction parameters
    const Vector4 & params() const { return m_params; }

  private:

    Nimble::Vector2f m_center;
    float   m_radiusInv;
    Vector4 m_params;
  };

}

#endif
