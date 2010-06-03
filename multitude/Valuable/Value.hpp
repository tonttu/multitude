#ifndef VALUABLE_VALUE_HPP
#define VALUABLE_VALUE_HPP

#include "ValueInt.hpp"
#include "ValueFloat.hpp"
#include "ValueVector.hpp"

#define DefineType(_Type, _Klass) template <> struct Value< _Type > { typedef _Klass Type; }
#define DefineTypeT(_Type, _Klass) DefineType(_Type, _Klass< _Type >)

namespace Valuable
{

  /// Value struct helps finding the correct ValueObject for any type T.
  /// For example Value<int>::Type == ValueInt
  template <typename T> struct Value
  {
    /// The actual ValueObject type
    typedef ValueObjectT<T> Type;
  };

  /// @cond
  DefineTypeT(int, ValueIntT);
  DefineTypeT(float, ValueFloatT);
  /// @todo do these correctly, that is, with some more generic thing
  DefineType(Nimble::Vector2f, ValueVector2f);
  DefineType(Nimble::Vector3f, ValueVector3f);
  DefineType(Nimble::Vector4f, ValueVector4f);
  /// @endcond

  // Value<int> or Value<Vector4> are just better ways to say Numeric<int> or Vector<Vector4>.
  // ValueObjects with type T can be defined by Value<T>.
  /*template <typename T> class Value : public Type<T>::klass
  {
  public:
    Value(HasValues * parent, const std::string & name, const T & v = T(), bool transit = false)
      : Type<T>::klass(parent, name, v, transit) {}
  };*/

}

#undef DefineType

#endif // VALUABLE_VALUE_HPP
