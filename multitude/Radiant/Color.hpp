
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
      
      <b>Caveat:</b> You need to be careful with the floating-point and
      integer versions of the functions. For integers the valid range
      is 0-255, while for floating point values it is 0.0-1.0.
   */
    /// @todo not really the correct place for this (Luminous would be
    /// better place, but dependency with Valuable)
    class Color : public Nimble::Vector4f
  {
    public:
      /// Constructor
      RADIANT_API Color();

      /// Construct a color from a string
      /// @param color Name of the color.
      /// @sa set
      RADIANT_API Color(const QByteArray & color);

      /// Construct a color from a string
      /// @param color Name of the color.
      /// @sa set
      RADIANT_API Color(const char * color);

      /// Constructs a color from the given floats. The values are expected to be [0,1]
      /// @param red Value of red
      /// @param green Value of green
      /// @param blue Vallue of blue
      /// @param alpha Value of alpha (transparency, 0 = fully transparent, 1 = fully opaque)
      RADIANT_API Color(float red, float green, float blue, float alpha = 1.f);

      /// Constructs a color from the given vector. The component values are expected to be [0,1]
      /// @param v RGBA-vector
      RADIANT_API Color(const Nimble::Vector4f & v);

      /// Constructs a color from the given QColor
      /// @param color QColor to be copied
      RADIANT_API Color(const QColor & color);
      /// Destructor
      RADIANT_API ~Color();

      /// Make RGBA color from floats. Valid range for parameters is [0, 1]
      /// @param red Value of red
      /// @param green Value of green
      /// @param blue Vallue of blue
      /// @param alpha Value of alpha (transparency, 0 = fully transparent, 1 = fully opaque)
      RADIANT_API void setRGBA(float red, float green, float blue, float alpha);

      /// Make HSVA color from floats. Valid range for parameters is [0, 1]
      /// @param hue Hue of the color
      /// @param saturation Saturation of the color
      /// @param value Value (brightness) of the color
      /// @param alpha Alpha (transparency) of the color
      RADIANT_API void setHSVA(float hue, float saturation, float value, float alpha);

      /// Set color as a string
      /// @param color CSS3 color module extended color keyword or RGB(A) hexadecimal notation.
      /// See http://www.w3.org/TR/css3-color/#svg-color and http://www.w3.org/TR/css3-color/#rgb-color
      /// Example arguments: "black", "purple", "\#FFF" (white), "\#F00" (red) and "\#FF000080" (transparent red).
      /// @returns true if color was valid and set
      RADIANT_API bool set(const QByteArray & color);

      /// Converts this color to QColor. QColor uses 16-bit integers, so this
      /// conversion might lose some data. Values are also clamped between 0 and 255.
      /// @return Corresponding QColor-object.
      RADIANT_API QColor toQColor() const;

      /// Returns a new Color from RGBA values. All parameters are from 0 to 255.
      /// @param red Value of red
      /// @param green Value of green
      /// @param blue Value of blue
      /// @param alpha Value of alpha (transparency)
      /// @return Corresponding color object
      RADIANT_API static Color fromRGBA(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255u);

      /// Returns all named colors Color recognizes.
      /// These are CSS Color Module Level 3 - Extended color keywords
      /// (SVG 1.0 color keywords)
      RADIANT_API static const QMap<QByteArray, Color> & namedColors();

      /// Returns the red color component
      /// @return Value of red
      float red()   const { return get(0); }
      /// Returns the green color component
      /// @return Value of green
      float green() const { return get(1); }
      /// Returns the blue color component
      /// @return Value of blue
      float blue()  const { return get(2); }
      /// Returns the alpha component
      /// Alpha value zero means fully transparent color, while alpha
      /// value of 1 means fully opaque color.
      /// @return alpha component of the color
      float alpha() const { return get(3); }

      /// Sets the red color component
      /// @param r Value of red
      void setRed(float r) { x = r; }
      /// Sets the green color component
      /// @param g Value of green
      void setGreen(float g) { y = g; }
      /// Sets the blue color component
      /// @param b Value of blue
      void setBlue(float b) { z = b; }
      /// Sets the alpha color component
      /// @param a Value of alpha
      void setAlpha(float a) { w = a; }
    };

}

#endif
