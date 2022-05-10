/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_COLOR_UTILS_HPP
#define RADIANT_COLOR_UTILS_HPP

#include "Export.hpp"
#include "VideoImage.hpp"
#include "Color.hpp"

#include <Nimble/Math.hpp>
#include <Nimble/Vector3.hpp>

namespace Radiant
{

  /// ColorUtils contains color conversion utilities
  /// @todo move to Color class
  class RADIANT_API ColorUtils
  {
  public:
    /// Conversion between RGB and HSV using code published in Hearn, D. and Baker, M.
    /// (1997) "Computer Graphics", New Jersey: Prentice Hall Inc. (pp. 578-579.).
    /// @param r Value of red
    /// @param g Value of green
    /// @param b Value of blue
    /// @param[out] h Hue
    /// @param[out] s Saturation
    /// @param[out] v Value (brightness)
    static void rgbTohsv(float r, float g, float b, float & h, float & s, float & v);
    /// @copybrief rgbTohsv
    static void rgbTohsv(const Nimble::Vector3f & rgb, Nimble::Vector3f & hsv);
    /// @copybrief rgbTohsv
    static Color rgbTohsv(const Color & rgb);
    /// @copybrief rgbTohsv
    static void hsvTorgb(float h, float s, float v, float & r, float & g, float & b);
    /// @copybrief rgbTohsv
    static void hsvTorgb(const Nimble::Vector3f & hsv, Nimble::Vector3f & rgb);
    /// @copybrief rgbTohsv
    static Color hsvTorgb(const Color & hsv);

    /// Convert linear RGB to CIE XYZ tristimulus values
    /// @param rgb linear rgb color
    /// @param[out] cie CIE XYZ tristimulus values
    static void rgbToCIEXYZ(const Nimble::Vector3f & rgb, Nimble::Vector3f & cie);
    /// Convert CIE XYZ to RGB
    /// @param cie XYZ vector to convert
    /// @param[out] rgb rgb vector to store the color
    static void CIEXYZToRGB(const Nimble::Vector3f & cie, Nimble::Vector3f & rgb);

    /// Convert CIE XYZ tristimulus values to CIE xyY values
    /// @param xyz CIE XYZ tristimulus values
    /// @param[out] xyy CIE xyY values
    static void CIEXYZtoCIEXYY(const Nimble::Vector3f & xyz, Nimble::Vector3f & xyy);

    /// Convert CIE xyY values to CIE XYZ tristimulus  values
    /// @param xyy CIE xyY values
    /// @param [xyz] CIE XYZ tristimulus values
    static void CIEXYYtoCIEXYZ(const Nimble::Vector3f & xyy, Nimble::Vector3f & xyz);

    /// Color balance the given RGB image
    /// Performs linear scaling of the RGB components with the given values and clamps them to [0,255] range
    /// @param img image to color balance
    /// @param rgbCoeff scaling coefficients for each color component
    static void colorBalance(VideoImage & img, Nimble::Vector3f rgbCoeff);
  };

}

#endif
