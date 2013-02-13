/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_HPP
#define VALUABLE_VALUE_HPP

#include "AttributeInt.hpp"
#include "AttributeFloat.hpp"
#include "AttributeVector.hpp"
#include "AttributeSize.hpp"

#define DefineType(_Type, _Klass) template <> struct AttributeFor< _Type > { typedef _Klass Type; }
#define DefineTypeT(_Type, _Klass) DefineType(_Type, _Klass< _Type >)

namespace Valuable
{

  /// Value struct helps finding the correct Attribute for any type T.
  /// For example AttributeFor<int>::Type == AttributeInt
  template <typename T> struct AttributeFor
  {
    /// The actual Attribute type
    typedef AttributeT<T> Type;
  };

  /// @cond

  DefineTypeT(int, AttributeIntT);
  DefineTypeT(float, AttributeFloatT);
  /// @todo do these correctly, that is, with some more generic thing
  DefineType(Nimble::Vector2f, AttributeVector2f);
  DefineType(Nimble::Vector3f, AttributeVector3f);
  DefineType(Nimble::Vector4f, AttributeVector4f);
  DefineType(Nimble::SizeF, AttributeSizeF);
  DefineType(Nimble::Size, AttributeSize);

  /// @endcond

  // Value<int> or Value<Vector4> are just better ways to say Numeric<int> or Vector<Vector4>.
  // Attributes with type T can be defined by Value<T>.
  /*template <typename T> class Value : public Type<T>::klass
  {
  public:
    Value(Node * parent, const QString & name, const T & v = T(), bool transit = false)
      : Type<T>::klass(parent, name, v, transit) {}
  };*/

}

#undef DefineTypeT
#undef DefineType

#endif // VALUABLE_VALUE_HPP
