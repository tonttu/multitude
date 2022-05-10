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

  template <typename T>
  struct IsRect { static constexpr bool value = false; };

  template <typename E>
  struct IsRect<Nimble::RectT<E>> { static constexpr bool value = true; };

  /// A valuable object holding a Nimble::Rect object
  template <class RectType>
  class AttributeT<RectType, typename std::enable_if<IsRect<RectType>::value>::type>
      : public AttributeBaseT<RectType>
  {
    typedef AttributeBaseT<RectType> Base;
  public:
    using Base::operator =;

    /// @copydoc Attribute::Attribute(Node *, const QString &)
    /// @param r The rectangle to be stored in the AttributeRect
    AttributeT(Node * host = nullptr, const QByteArray & name = QByteArray(), const RectType & r = RectType())
      : Base(host, name, r) {}

    virtual QString asString(bool * const ok, Attribute::Layer layer) const OVERRIDE
    {
      if(ok) *ok = true;

      return Radiant::StringUtils::toString(this->value(layer));
    }

    virtual QByteArray type() const override
    {
      return "rect:" + Radiant::StringUtils::type<decltype(RectType().width())>();
    }

    static inline RectType interpolate(RectType a, RectType b, float m)
    {
      return m >= 0.5f ? b : a;
    }

    /// Converts the object to rectangle
    /// FIXME: can this be just removed? why not call value() or operator*?
    RectType asRect() const { return this->value(); }
  };

  /// Default floating point AttributeRectT typedef
  typedef AttributeT<Nimble::Rectf> AttributeRect;
  /// AttributeRectT of floats
  typedef AttributeT<Nimble::Rectf> AttributeRectf;
  /// AttributeRectT of doubles
  typedef AttributeT<Nimble::Rectd> AttributeRectd;
  /// AttributeRectT of ints
  typedef AttributeT<Nimble::Recti> AttributeRecti;
}

#endif
