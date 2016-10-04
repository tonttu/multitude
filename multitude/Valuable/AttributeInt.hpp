/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUE_INT_HPP
#define VALUABLE_VALUE_INT_HPP

#include <Valuable/Export.hpp>
#include <Valuable/AttributeNumeric.hpp>

#include <Radiant/TimeStamp.hpp>

#include <Nimble/Math.hpp>

namespace Valuable
{

  /// Template class for integer values.
  /** The actual value objects are created by using AttributeT<int>
      etc. */
  template<class T>
  class AttributeT<T, typename std::enable_if<std::is_integral<T>::value>::type>
      : public AttributeNumericT<T>
  {
    typedef AttributeNumericT<T> Base;
  public:
    using Base::value;
    using Base::operator =;

#ifdef CLANG_XML
    virtual int asInt(bool * const ok, Attribute::Layer layer) const OVERRIDE { if(ok) *ok = true; return static_cast<int> (value(layer)); }
#endif

    AttributeT() : Base() {}
    /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
    /// @param v The numeric value of this object
    AttributeT(Node * host, const QByteArray & name, T v = T())
        : AttributeNumericT<T>(host, name, v)
    {}

    /// Assignment by subtraction
    AttributeT & operator -= (T i) { *this = value() - i; return *this; }
    /// Assignment by addition
    AttributeT & operator += (T i) { *this = value() + i; return *this; }
    /// Assignment by multiplication
    AttributeT & operator *= (T i) { *this = value() * i; return *this; }
    /// Assignment by division
    AttributeT & operator /= (T i) { *this = value() / i; return *this; }

    /// Does a logical OR for the integer
    AttributeT & operator |= (T i) { *this = value() | i; return *this; }
    /// Does a logical AND for the integer
    AttributeT & operator &= (T i) { *this = value() & i; return *this; }
    /// Modulo operator
    AttributeT & operator %= (T i) { *this = value() % i; return *this; }
    /// Does a bitwise exclusive OR
    AttributeT & operator ^= (T i) { *this = value() ^ i; return *this; }

    /// Prefix increment
    AttributeT & operator ++ ()
    {
      auto v = value();
      this->setValue(++v);
      return *this;
    }

    /// Prefix decrement
    AttributeT & operator -- ()
    {
      auto v = value();
      this->setValue(--v);
      return *this;
    }

    /// Shift left
    AttributeT & operator <<= (int i) { *this = value() << i; return *this; }
    /// Shift right
    AttributeT & operator >>= (int i) { *this = value() >> i; return *this; }

    /// Sets the numeric value
    inline virtual bool set(int v, Attribute::Layer layer = Attribute::USER,
                            Attribute::ValueUnit = Attribute::VU_UNKNOWN) OVERRIDE
    {
      this->setValue(v, layer);
      return true;
    }
    /// @copydoc set
    inline virtual bool set(float v, Attribute::Layer layer = Attribute::USER,
                            Attribute::ValueUnit = Attribute::VU_UNKNOWN) OVERRIDE
    {
      this->setValue(Nimble::Math::Round(v), layer);
      return true;
    }

    virtual bool set(const StyleValue &value, Attribute::Layer layer) OVERRIDE
    {
      return Base::set(value, layer);
    }

    /// Compares less than
    bool operator < (const T & i) const { return value() < i; }
    /// Compares less or equal than
    bool operator <= (const T & i) const { return value() <= i; }
    /// Compares greater than
    bool operator > (const T & i) const { return value() > i; }
    /// Compares greater or equal than
    bool operator >= (const T & i) const { return value() >= i; }

    virtual void eventProcess(const QByteArray & /*id*/, Radiant::BinaryData & data) OVERRIDE
    {
      bool ok = true;
      T v = data.read<T>( & ok);

      if(ok)
        *this = v;
    }

    static inline T interpolate(T a, T b, float m)
    {
      float tmp = a*(1.f - m) + b*m;
      if(tmp > 0.f) return (T)(tmp + 0.5f);
      else return (T)(tmp - 0.5f);
    }
  };

  /// 32-bit integer value object.
  typedef AttributeT<int32_t> AttributeInt;
  /// 32-bit integer value object.
  typedef AttributeT<int32_t> AttributeInt32;
  /// 32-bit unsigned integer value object.
  typedef AttributeT<uint32_t> AttributeUInt32;
  /// 64-bit integer value object.
  typedef AttributeT<int64_t> AttributeInt64;
  /// 64-bit unsigned integer value object.
  typedef AttributeT<uint64_t> AttributeUInt64;
}

#endif
