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
  class AttributeColor : public AttributeVector<Radiant::Color>
  {
  public:
    using AttributeVector<Radiant::Color>::operator=;

    /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
    /// @param c The color value
    AttributeColor(Node * host, const QByteArray & name, const Radiant::Color & c, bool transit = false)
      : AttributeVector<Radiant::Color>(host, name, c, transit)
    {}

    /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
    /// @param c The color value as string
    AttributeColor(Node * host, const QByteArray & name, const QByteArray & c, bool transit = false)
      : AttributeVector<Radiant::Color>(host, name, Radiant::Color(c), transit)
    {}

    ~AttributeColor()
    {}

    bool set(const Nimble::Vector4f & color, Layer layer = USER,
             QList<ValueUnit> = QList<ValueUnit>())
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

    /// Converts the value object to color
    Radiant::Color asColor() const { return asVector(); }
    
    /// Returns the red comoponent of the color (0-1).
    inline float red() const   { return get(0); }
    /// Returns the green comoponent of the color (0-1).
    inline float green() const { return get(1); }
    /// Returns the blue comoponent of the color (0-1).
    inline float blue() const  { return get(2); }
    /// Returns the alpha comoponent of the color (0-1).
    inline float alpha() const { return get(3); }
  };

}

#endif
