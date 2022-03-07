
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
#include <QMap>

#include <Nimble/Vector4.hpp>

#include <cstdint>

namespace Radiant
{
  template <typename Self>
  class ColorBase
  {
  public:
    inline ColorBase() {}

    /// Constructs a color from the given floats. The values are usually
    /// between [0,1], but other values are also allowed.
    /// @param red Value of red
    /// @param green Value of green
    /// @param blue Value of blue
    /// @param alpha Value of alpha (transparency, 0 = fully transparent, 1 = fully opaque)
    inline ColorBase(float red, float green, float blue, float alpha = 1.f)
      : r(red), g(green), b(blue), a(alpha) {}

    inline ColorBase(const ColorBase &) = default;
    inline ColorBase & operator=(const ColorBase &) = default;

    /// Destructor, does nothing
    inline ~ColorBase() {}

    inline bool operator==(const Self & c) const { return r == c.r && g == c.g && b == c.b && a == c.a; }
    inline bool operator!=(const Self & c) const { return r != c.r || g != c.g || b != c.b || a != c.a; }

    inline Self operator*(float s) const { return Self(r * s, g * s, b * s, a * s); }
    inline Self operator/(float s) const { return Self(r / s, g / s, b / s, a / s); }
    friend inline Self operator*(float s, const Self & c) { return Self(s * c.r, s * c.g, s * c.b, s * c.a); }
    inline Self operator*(const Self & c) const { return Self(r * c.r, g * c.g, b * c.b, a * c.a); }
    inline Self operator+(const Self & c) const { return Self(r + c.r, g + c.g, b + c.b, a + c.a); }
    inline Self operator-(const Self & c) const { return Self(r - c.r, g - c.g, b - c.b, a - c.a); }

    inline Self & operator*=(float s) { r *= s; g *= s; b *= s; a *= s; return static_cast<Self&>(*this); }
    inline Self & operator/=(float s) { r /= s; g /= s; b /= s; a /= s; return static_cast<Self&>(*this); }
    inline Self & operator*=(const Self & c) { r *= c.r; g *= c.g; b *= c.b; a *= c.a; return static_cast<Self&>(*this); }
    inline Self & operator+=(const Self & c) { r += c.r; g += c.g; b += c.b; a += c.a; return static_cast<Self&>(*this); }


    /// Make RGBA color from floats.
    /// @param red Value of red
    /// @param green Value of green
    /// @param blue Value of blue
    /// @param alpha Value of alpha (transparency, 0 = fully transparent, 1 = fully opaque)
    inline void setRGBA(float red, float green, float blue, float alpha)
    { r = red; g = green; b = blue; a = alpha; }

    /// Returns the red color component
    /// @return Value of red
    float red()   const { return r; }
    /// Returns the green color component
    /// @return Value of green
    float green() const { return g; }
    /// Returns the blue color component
    /// @return Value of blue
    float blue()  const { return b; }
    /// Returns the alpha component
    /// Alpha value zero means fully transparent color, while alpha
    /// value of 1 means fully opaque color.
    /// @return alpha component of the color
    float alpha() const { return a; }

    /// Sets the red color component
    /// @param r Value of red
    void setRed(float red) { r = red; }
    /// Sets the green color component
    /// @param g Value of green
    void setGreen(float green) { g = green; }
    /// Sets the blue color component
    /// @param b Value of blue
    void setBlue(float blue) { b = blue; }
    /// Sets the alpha color component
    /// @param a Value of alpha
    void setAlpha(float alpha) { a = alpha; }

    inline float * data() { return &r; }
    inline const float * data() const { return &r; }

    inline Nimble::Vector4f toVector() const { return {r, g, b, a}; }

    /// Checks if all components are zero
    inline bool isZero() const { return r == 0 && g == 0 && b == 0 && a == 0; }

    /// Returns the largest component
    inline float maximum() const { return std::max(std::max(r,g),std::max(b,a)); }
    /// Returns the smallest component
    inline float minimum() const { return std::min(std::min(r,g),std::min(b,a)); }

  public:
    float r, g, b, a;
  };

  /// Utility class for color management & conversion
  /** Color implements a classical RGBA color in floating-point format.

      <b>Caveat:</b> You need to be careful with the floating-point and
      integer versions of the functions. For integers normal range
      is 0-255, while for floating point values it is 0.0-1.0.
   */
  /// @todo not really the correct place for this (Luminous would be
  /// better place, but dependency with Valuable)
  class Color : public ColorBase<Color>
  {
  public:
    /// Constructor, sets the color to (0, 0, 0, 1)
    inline Color() : ColorBase(0, 0, 0, 1) {}

    /// Constructs a color from the given floats. The values are usually
    /// between [0,1], but other values are also allowed.
    /// @param red Value of red
    /// @param green Value of green
    /// @param blue Vallue of blue
    /// @param alpha Value of alpha (transparency, 0 = fully transparent, 1 = fully opaque)
    inline Color(float red, float green, float blue, float alpha = 1.f)
      : ColorBase(red, green, blue, alpha) {}

    /// Constructs a color from the given vector. The component values are
    /// usually between [0,1], but other values are also allowed.
    /// @param v RGBA-vector
    inline Color(const Nimble::Vector4f & v)
      : ColorBase(v.x, v.y, v.z, v.w) {}

    inline Color(const Nimble::Vector3f & rgb, float alpha = 1.f)
     : ColorBase(rgb.x, rgb.y, rgb.z, alpha) {}

    inline Color(const Color &) = default;
    inline Color & operator=(const Color &) = default;

    /// Destructor, does nothing
    inline ~Color() {}

    /// Construct a color from a string
    /// @param color Name of the color.
    /// @sa set
    RADIANT_API Color(const QByteArray & color);

    RADIANT_API Color(const char * color);

    /// Constructs a color from the given QColor
    /// @param color QColor to be copied
    RADIANT_API Color(const QColor & color);

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
    static inline Color fromRGBA(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255u)
    { return Color(red / 255.f, green / 255.f, blue / 255.f, alpha / 255.f); }

    /// Returns all named colors Color recognizes.
    /// These are CSS Color Module Level 3 - Extended color keywords
    /// (SVG 1.0 color keywords)
    RADIANT_API static const QMap<QByteArray, Color> & namedColors();

    inline Nimble::Vector3f rgb() const { return {r, g, b}; }

    inline Nimble::Vector4f toVector() const { return {r, g, b, a}; }

    RADIANT_API QString toHex() const;
  };

  /// Color that has premultiplied alpha
  class ColorPMA : public ColorBase<ColorPMA>
  {
  public:
    inline ColorPMA() : ColorBase(0, 0, 0, 1) {}

    /// Construct a color from a string
    /// @param color Name of the color.
    /// @sa set
    inline ColorPMA(const QByteArray & color)
      : ColorPMA(Color(color)) {}

    inline ColorPMA(const char * color)
      : ColorPMA(Color(color)) {}

    inline ColorPMA(const Nimble::Vector4f & v)
      : ColorBase(v.x, v.y, v.z, v.w) {}

    inline ColorPMA(float red, float green, float blue, float alpha = 1.f)
      : ColorBase(red, green, blue, alpha) {}

    inline ColorPMA(const Color & color)
      : ColorBase(color.r * color.a, color.g * color.a, color.b * color.a, color.a) {}

    inline Color toColor() const
    {
      if (std::abs(a) > std::numeric_limits<float>::epsilon()) {
        return Color(r / a, g / a, b / a, a);
      } else {
        /// @todo what to do here actually?
        return Color(r, g, b, a);
      }
    }

    inline Nimble::Vector4f toVector() const { return {r, g, b, a}; }
  };

  static_assert(sizeof(Color) == sizeof(float) * 4, "Color alignment check");
  static_assert(sizeof(ColorPMA) == sizeof(float) * 4, "Color alignment check");

  inline std::ostream & operator<<(std::ostream & os, const Color & t)
  {
    return os << t.r << ' ' << t.g << ' ' << t.b << ' ' << t.a;
  }

  inline std::istream & operator>>(std::istream & is, Color & t)
  {
    return is >> t.r >> t.g >> t.b >> t.a;
  }

  inline std::ostream & operator<<(std::ostream & os, const ColorPMA & t)
  {
    return os << t.r << ' ' << t.g << ' ' << t.b << ' ' << t.a;
  }

  inline std::istream & operator>>(std::istream & is, ColorPMA & t)
  {
    return is >> t.r >> t.g >> t.b >> t.a;
  }

}

#endif
