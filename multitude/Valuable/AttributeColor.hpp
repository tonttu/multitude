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
  /** A value object holding a #Radiant::ColorPMA value. */
  template <>
  class AttributeT<Radiant::ColorPMA> : public AttributeBaseT<Radiant::ColorPMA>
  {
    typedef AttributeBaseT<Radiant::ColorPMA> Base;
  public:
    using Base::operator=;

    /// @copydoc Attribute::Attribute()
    AttributeT() : Base()
    {}

    /// @copydoc Attribute::Attribute(Node *, const QString &)
    /// @param c The color value
    AttributeT(Node * host, const QByteArray & name, const Radiant::ColorPMA & c)
      : Base(host, name, c)
    {}

    /// @copydoc Attribute::Attribute(Node *, const QString &)
    /// @param c The color value as string
    AttributeT(Node * host, const QByteArray & name, const QByteArray & c)
      : Base(host, name, Radiant::Color(c))
    {}

    /// @copydoc Attribute::Attribute(Node *, const QString &)
    /// @param c The color value as string
    AttributeT(Node * host, const QByteArray & name, const char * c)
      : Base(host, name, Radiant::Color(c))
    {}

    ~AttributeT()
    {}

    /// Sets the attribute color in non-premultiplied format
    bool set(const Nimble::Vector4f & color, Layer layer = USER,
             QList<ValueUnit> = QList<ValueUnit>()) OVERRIDE
    {
      this->setValue(Radiant::Color(color.x, color.y, color.z, color.w), layer);
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

      auto & c = v[0];

      if (c.type() == StyleValue::TYPE_COLOR) {
        this->setValue(c.asColorPMA(), layer);
        return true;
      } else if (c.type() == StyleValue::TYPE_COLOR_PMA) {
        this->setValue(c.asColorPMA(), layer);
        return true;
      } else if (c.canConvert(StyleValue::TYPE_KEYWORD)) {
        Radiant::Color tmp;
        if (tmp.set(c.asKeyword())) {
          this->setValue(tmp, layer);
          return true;
        }
      }

      return false;
    }

    virtual QString asString(bool * const ok = nullptr, Layer layer = CURRENT_VALUE) const OVERRIDE
    {
      if (ok)
        *ok = true;
      Radiant::ColorPMA c = value(layer);
      return QString("rgba_pma(%1, %2, %3, %4)").arg(c.r*255).arg(c.g*255).arg(c.b*255).arg(c.a);
    }

    /// Converts the value object to color
    Radiant::ColorPMA asColor() const { return value(); }

    /// Returns the premultiplied red component of the color (0-1).
    inline float red() const   { return value().r; }
    /// Returns the premultiplied green component of the color (0-1).
    inline float green() const { return value().g; }
    /// Returns the premultiplied blue component of the color (0-1).
    inline float blue() const  { return value().b; }
    /// Returns the alpha component of the color (0-1).
    inline float alpha() const { return value().a; }
  };
  typedef AttributeT<Radiant::ColorPMA> AttributeColor;

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

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

    /// @copydoc Attribute::Attribute(Node *, const QString &)
    /// @param c The color value
    AttributeT(Node * host, const QByteArray & name, const Radiant::Color & c)
      : Base(host, name, c)
    {}

    /// @copydoc Attribute::Attribute(Node *, const QString &)
    /// @param c The color value as string
    AttributeT(Node * host, const QByteArray & name, const QByteArray & c)
      : Base(host, name, Radiant::Color(c))
    {}

    /// @copydoc Attribute::Attribute(Node *, const QString &)
    /// @param c The color value as string
    AttributeT(Node * host, const QByteArray & name, const char * c)
      : Base(host, name, Radiant::Color(c))
    {}

    ~AttributeT()
    {}

    /// Sets the attribute color in non-premultiplied format
    bool set(const Nimble::Vector4f & color, Layer layer = USER,
             QList<ValueUnit> = QList<ValueUnit>()) OVERRIDE
    {
      this->setValue(Radiant::Color(color.x, color.y, color.z, color.w), layer);
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

      auto & c = v[0];

      if (c.type() == StyleValue::TYPE_COLOR) {
        this->setValue(c.asColor(), layer);
        return true;
      } else if (c.type() == StyleValue::TYPE_COLOR_PMA) {
        this->setValue(c.asColor(), layer);
        return true;
      } else if (c.canConvert(StyleValue::TYPE_KEYWORD)) {
        Radiant::Color tmp;
        if (tmp.set(c.asKeyword())) {
          this->setValue(tmp, layer);
          return true;
        }
      }

      return false;
    }

    virtual QString asString(bool * const ok, Layer layer) const override
    {
      if (ok)
        *ok = true;
      Radiant::Color c = value(layer);
      return QString("rgba(%1, %2, %3, %4)").arg(c.r*255, c.g*255, c.b*255, c.a);
    }

    /// Converts the value object to color
    Radiant::Color asColor() const { return value(); }

    /// Returns the premultiplied red component of the color (0-1).
    inline float red() const   { return value().r; }
    /// Returns the premultiplied green component of the color (0-1).
    inline float green() const { return value().g; }
    /// Returns the premultiplied blue component of the color (0-1).
    inline float blue() const  { return value().b; }
    /// Returns the alpha component of the color (0-1).
    inline float alpha() const { return value().a; }
  };

}

#endif
