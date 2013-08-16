/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_RECT_HPP
#define VALUABLE_VALUE_RECT_HPP

#include <Radiant/StringUtils.hpp>
#include <Valuable/Export.hpp>
#include <Valuable/Attribute.hpp>

#include <Nimble/Rect.hpp>

namespace Valuable
{

  /// A valuable object holding a Nimble::Rect object
  template <class T>
  class AttributeT<T, Attribute::ATTR_RECT> : public AttributeBaseT<T>
  {
    typedef AttributeBaseT<T> Base;
  public:
    using Base::operator =;

    /// @copydoc Attribute::Attribute(Node *, const std::string &, bool transit)
    /// @param r The rectangle to be stored in the AttributeRect
    AttributeT(Node * host, const QByteArray & name, const Nimble::RectT<T> & r, bool transit = false)
      : Base(host, name, r, transit) {}

    virtual QString asString(bool * const ok, Attribute::Layer layer) const OVERRIDE;

    /// Converts the object to rectangle
    Nimble::RectT<T> asRect() const { return this->value(); }
  };

  /// Default floating point AttributeRectT typedef
  typedef AttributeT<Nimble::Rectf> AttributeRect;
  /// AttributeRectT of floats
  typedef AttributeT<Nimble::Rectf> AttributeRectf;
  /// AttributeRectT of doubles
  typedef AttributeT<Nimble::Rectd> AttributeRectd;
  /// AttributeRectT of ints
  typedef AttributeT<Nimble::Recti> AttributeRecti;

  template <class T>
  QString AttributeT<T, Attribute::ATTR_RECT>::asString(bool * const ok, Attribute::Layer layer) const {
    if(ok) *ok = true;

    return Radiant::StringUtils::toString(this->value(layer));
  }
}

#endif
