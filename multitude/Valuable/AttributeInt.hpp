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
  /** The actual value objects are created by using AttributeIntT<int>
      etc.

      @see AttributeInt*/

  template<class T>
  class AttributeIntT : public AttributeNumericT<T>
  {
    typedef AttributeNumericT<T> Base;
  public:
    using Base::value;
    using Base::m_current;
    using Base::m_values;
    using Base::m_valueSet;
    using Base::operator =;

    AttributeIntT() : Base() {}
    /// @copydoc Attribute::Attribute(Node *, const QString &, bool transit)
    /// @param v The numeric value of this object
    AttributeIntT(Node * host, const QByteArray & name, T v = T(), bool transit = false)
        : AttributeNumericT<T>(host, name, v, transit)
    {}

    /// Assignment by subtraction
    AttributeIntT<T> & operator -= (T i) { *this = value() - i; return *this; }
    /// Assignment by addition
    AttributeIntT<T> & operator += (T i) { *this = value() + i; return *this; }
    /// Assignment by multiplication
    AttributeIntT<T> & operator *= (T i) { *this = value() * i; return *this; }
    /// Assignment by division
    AttributeIntT<T> & operator /= (T i) { *this = value() / i; return *this; }

    /// Does a logical OR for the integer
    AttributeIntT<T> & operator |= (T i) { *this = value() | i; return *this; }
    /// Does a logical AND for the integer
    AttributeIntT<T> & operator &= (T i) { *this = value() & i; return *this; }
    /// Modulo operator
    AttributeIntT<T> & operator %= (T i) { *this = value() % i; return *this; }
    /// Does a bitwise exclusive OR
    AttributeIntT<T> & operator ^= (T i) { *this = value() ^ i; return *this; }

    /// Prefix increment
    AttributeIntT<T> & operator ++ ()
    {
      if(m_current != Base::USER) {
        m_values[Base::USER] = m_values[m_current];
        m_valueSet[Base::USER] = true;
        m_current = Base::USER;
      }
      ++m_values[m_current];
      this->emitChange();
      return *this;
    }

    /// Prefix decrement
    AttributeIntT<T> & operator -- ()
    {
      if(m_current != Base::USER) {
        m_values[Base::USER] = m_values[m_current];
        m_valueSet[Base::USER] = true;
        m_current = Base::USER;
      }
      --m_values[m_current];
      this->emitChange();
      return *this;
    }

    /// Shift left
    AttributeIntT<T> & operator <<= (int i) { *this = value() << i; return *this; }
    /// Shift right
    AttributeIntT<T> & operator >>= (int i) { *this = value() >> i; return *this; }

    /// Sets the numeric value
    inline virtual bool set(int v, Attribute::Layer layer = Attribute::USER,
                            Attribute::ValueUnit = Attribute::VU_UNKNOWN)
    {
      this->setValue(v, layer);
      return true;
    }
    /// @copydoc set
    inline virtual bool set(float v, Attribute::Layer layer = Attribute::USER,
                            Attribute::ValueUnit = Attribute::VU_UNKNOWN)
    {
      this->setValue(Nimble::Math::Round(v), layer);
      return true;
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
  };

  /// 32-bit integer value object.
  typedef AttributeIntT<int32_t> AttributeInt;
  /// 32-bit integer value object.
  typedef AttributeIntT<int32_t> AttributeInt32;
  /// 32-bit unsigned integer value object.
  typedef AttributeIntT<uint32_t> AttributeUInt32;
  /// 64-bit integer value object.
  typedef AttributeIntT<int64_t> AttributeInt64;
  /// 64-bit unsigned integer value object.
  typedef AttributeIntT<uint64_t> AttributeUInt64;
}

#endif
