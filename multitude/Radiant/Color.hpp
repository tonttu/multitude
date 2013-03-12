
/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#ifndef RADIANT_COLOR_HPP
#define RADIANT_COLOR_HPP

#include "Export.hpp"

#include <QByteArray>
#include <QColor>

#include <Nimble/Vector4.hpp>

#include <cstdint>

namespace Radiant
{

  /// Utility class for color management & conversion
  /** Color implements a classical RGBA color in floating-point format. 
      
      <b>Caveat:</b> You need to careful with the floating-point and
      integer versions of the functions. For integers the valid range
      is 0-255, while for floating point values it is 0.0-1.0.
   */

    /// @todo not really the correct place for this (Luminous would be
    /// better place, but dependency with Valuable), add HSV support
    class Color : public Nimble::Vector4f
  {
    public:
      RADIANT_API Color();

      /// Construct a color from a string
      /// @sa set
      RADIANT_API Color(const QByteArray & color);
      /// Constructs a color from the given floats. The values are expected to be [0,1]
      RADIANT_API Color(float r, float g, float b, float a = 1.f);
      /// Constructs a color from the given vector. The component values are expected to be [0,1]
      RADIANT_API Color(const Nimble::Vector4f & v);
      RADIANT_API ~Color();

      /// Make RGBA color from floats. Valid range is [0, 1]
      RADIANT_API void setRGBA(float r, float g, float b, float a);
      /// Make HSVA color from floats.
      RADIANT_API void setHSVA(float h, float s, float v, float a);

      /// Set color as a string
      /// @param color CSS3 color module extended color keyword or RGB(A) hexadecimal notation.
      /// See http://www.w3.org/TR/css3-color/#svg-color and http://www.w3.org/TR/css3-color/#rgb-color
      /// Example arguments: "black", "purple", "\#FFF" (white), "\#F00" (red) and "\#FF000080" (transparent red).
      /// @returns true if color was valid
      RADIANT_API bool set(const QByteArray & color);

      /// Converts this color to QColor. QColor uses 16-bit integers, so this
      /// conversion might lose some data. Values are also clamped between 0 and 255.
      RADIANT_API QColor toQColor() const;

      /// Returns a new Color from RGBA values. All parameters are from 0 to 255.
      RADIANT_API static Color fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255u);

      /// Returns the red color component
      float red()   const { return get(0); }
      /// Returns the green color component
      float green() const { return get(1); }
      /// Returns the blue color component
      float blue()  const { return get(2); }
      /// Returns the alpha component
      /// Alpha value zero means fully transparent color, while alpha
      /// value of 1 means fully opaque color.
      /// @return alpha component of the color
      float alpha() const { return get(3); }

      /// Sets the red color component
      void setRed(float r) { x = r; }
      /// Sets the green color component
      void setGreen(float g) { y = g; }
      /// Sets the blue color component
      void setBlue(float b) { z = b; }
      /// Sets the alpha color component
      void setAlpha(float a) { w = a; }
    };

}

#endif
