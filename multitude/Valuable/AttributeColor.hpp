/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_COLOR_HPP
#define VALUABLE_VALUE_COLOR_HPP

#include <Radiant/Color.hpp>

#include <Valuable/Export.hpp>
#include <Valuable/AttributeVector.hpp>
#include <Valuable/StyleValue.hpp>

namespace Valuable
{
  /** A value object holding a #Radiant::Color value. */
  template <>
  class AttributeT<Radiant::Color> : public AttributeBaseT<Radiant::Color>
  {
    typedef AttributeBaseT<Radiant::Color> Base;
  public:
    using Base::operator=;

    /// @copydoc Attribute::Attribute()
    AttributeT() : Base()
    {}

    /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
    /// @param c The color value
    AttributeT(Node * host, const QByteArray & name, const Radiant::Color & c, bool transit = false)
      : Base(host, name, c, transit)
    {}

    /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
    /// @param c The color value as string
    AttributeT(Node * host, const QByteArray & name, const QByteArray & c, bool transit = false)
      : Base(host, name, Radiant::Color(c), transit)
    {}

    /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
    /// @param c The color value as string
    AttributeT(Node * host, const QByteArray & name, const char * c, bool transit = false)
      : Base(host, name, Radiant::Color(c), transit)
    {}

    ~AttributeT()
    {}

    bool set(const Nimble::Vector4f & color, Layer layer = USER,
             QList<ValueUnit> = QList<ValueUnit>()) OVERRIDE
    {
      this->setValue(color, layer);
      return true;
    }

    virtual bool set(const QString & v, Layer layer = USER, ValueUnit = VU_UNKNOWN) OVERRIDE
    {
      Radiant::Color c;
      if (c.set(v.toUtf8())) {
        this->setValue(c, layer);
        return true;
      }
      return false;
    }

    virtual bool set(const StyleValue & v, Layer layer = USER) OVERRIDE
    {
      if (v.size() != 1)
        return false;
      Radiant::Color c;
      if (c.set(v.stringify().toUtf8())) {
        this->setValue(c, layer);
        return true;
      }
      return false;
    }

    virtual QString asString(bool * const ok, Layer layer) const OVERRIDE
    {
      if (ok)
        *ok = true;
      Radiant::Color c = value(layer);
      int r = Nimble::Math::Clamp<int>(0, 255, c.red()*255);
      int g = Nimble::Math::Clamp<int>(0, 255, c.green()*255);
      int b = Nimble::Math::Clamp<int>(0, 255, c.blue()*255);
      int a = Nimble::Math::Clamp<int>(0, 255, c.alpha()*255);
      return QString("#%1%2%3%4").arg(r, 2, 16, QChar('0')).arg(g, 2, 16, QChar('0')).
          arg(b, 2, 16, QChar('0')).arg(a, 2, 16, QChar('0'));
    }

    /// Converts the value object to color
    Radiant::Color asColor() const { return value(); }

    /// Returns the red comoponent of the color (0-1).
    inline float red() const   { return value()[0]; }
    /// Returns the green comoponent of the color (0-1).
    inline float green() const { return value()[1]; }
    /// Returns the blue comoponent of the color (0-1).
    inline float blue() const  { return value()[2]; }
    /// Returns the alpha comoponent of the color (0-1).
    inline float alpha() const { return value()[3]; }
  };
  typedef AttributeT<Radiant::Color> AttributeColor;
}

#endif
