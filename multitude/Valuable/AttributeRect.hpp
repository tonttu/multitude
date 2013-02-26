/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_RECT_HPP
#define VALUABLE_VALUE_RECT_HPP

#include <Radiant/StringUtils.hpp>
#include <Valuable/Export.hpp>
#include <Valuable/AttributeObject.hpp>

#include <Nimble/Rect.hpp>

namespace Valuable
{

  /// A valuable object holding a Nimble::Rect object
  template <class T>
  class AttributeRectT : public AttributeT<Nimble::RectT<T> >
  {
    typedef AttributeT<Nimble::RectT<T> > Base;
  public:
    using Base::operator =;

    /// @copydoc Attribute::Attribute(Node *, const std::string &, bool transit)
    /// @param r The rectangle to be stored in the AttributeRect
    AttributeRectT(Node * host, const QByteArray & name, const Nimble::RectT<T> & r, bool transit = false);

    virtual QString asString(bool * const ok = 0) const OVERRIDE;

    /// Converts the object to rectangle
    Nimble::RectT<T> asRect() const { return this->value(); }
  };

  /// Default floating point AttributeRectT typedef
  typedef AttributeRectT<float> AttributeRect;
  /// AttributeRectT of floats
  typedef AttributeRectT<float> AttributeRectf;
  /// AttributeRectT of doubles
  typedef AttributeRectT<double> AttributeRectd;
  /// AttributeRectT of ints
  typedef AttributeRectT<int> AttributeRecti;


  template <class T>
  AttributeRectT<T>::AttributeRectT(Node * host, const QByteArray & name, const Nimble::RectT<T> & r, bool transit)
    : Base(host, name, r, transit)
  {}

  template <class T>
  QString AttributeRectT<T>::asString(bool * const ok) const {
    if(ok) *ok = true;

    return Radiant::StringUtils::toString(this->value());
  }
}

#endif
