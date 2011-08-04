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

#ifndef VALUABLE_VALUE_INT_HPP
#define VALUABLE_VALUE_INT_HPP

#include <Valuable/Export.hpp>
#include <Valuable/ValueNumeric.hpp>

#include <Radiant/TimeStamp.hpp>

#define VO_TYPE_INT "int"

namespace Valuable
{

  /// Template class for integer values.
  /** The actual value objects are created by using ValueIntT<int>
      etc.

      @see ValueInt, ValueTimeStamp */

  template<class T>
      class VALUABLE_API ValueIntT : public ValueNumeric<T>
  {
    typedef ValueNumeric<T> Base;
  public:
    using Base::value;
    using Base::m_current;
    using Base::m_values;
    using Base::m_valueSet;
    using ValueObjectT<T>::operator =;

    ValueIntT() : Base() {}
    /// @copydoc ValueObject::ValueObject(HasValues *, const QString &, bool transit)
    /// @param v The numeric value of this object
    ValueIntT(HasValues * host, const QString & name, T v, bool transit = false)
        : ValueNumeric<T>(host, name, v, transit)
    {}

    /// Assignment by subtraction
    ValueIntT<T> & operator -= (T i) { *this = value() - i; return *this; }
    /// Assignment by addition
    ValueIntT<T> & operator += (T i) { *this = value() + i; return *this; }
    /// Assignment by multiplication
    ValueIntT<T> & operator *= (T i) { *this = value() * i; return *this; }
    /// Assignment by division
    ValueIntT<T> & operator /= (T i) { *this = value() / i; return *this; }

    /// Does a logical OR for the integer
    ValueIntT<T> & operator |= (T i) { *this = value() | i; return *this; }
    /// Does a logical AND for the integer
    ValueIntT<T> & operator &= (T i) { *this = value() & i; return *this; }
    /// Modulo operator
    ValueIntT<T> & operator %= (T i) { *this = value() % i; return *this; }
    /// Does a bitwise exclusive OR
    ValueIntT<T> & operator ^= (T i) { *this = value() ^ i; return *this; }

    /// Prefix increment
    ValueIntT<T> & operator ++ ()
    {
      if(m_current != Base::OVERRIDE) {
        m_values[Base::OVERRIDE] = m_values[m_current];
        m_valueSet[Base::OVERRIDE] = true;
        m_current = Base::OVERRIDE;
      }
      ++m_values[m_current];
      this->emitChange();
      return *this;
    }

    /// Prefix decrement
    ValueIntT<T> & operator -- ()
    {
      if(m_current != Base::OVERRIDE) {
        m_values[Base::OVERRIDE] = m_values[m_current];
        m_valueSet[Base::OVERRIDE] = true;
        m_current = Base::OVERRIDE;
      }
      --m_values[m_current];
      this->emitChange();
      return *this;
    }

    /// Shift left
    ValueIntT<T> & operator <<= (int i) { *this = value() << i; return *this; }
    /// Shift right
    ValueIntT<T> & operator >>= (int i) { *this = value() >> i; return *this; }

    /// Sets the numeric value
    inline virtual bool set(int v, ValueObject::Layer layer = ValueObject::OVERRIDE)
    {
      this->setValue(v, layer);
      return true;
    }
    /// @copydoc set
    inline virtual bool set(float v, ValueObject::Layer layer = ValueObject::OVERRIDE)
    {
      this->setValue(v, layer);
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

    const char * type() const { return VO_TYPE_INT; }

    virtual void processMessage(const char * id, Radiant::BinaryData & data);

    bool deserialize(const ArchiveElement & element);
  };

  /// 32-bit integer value object.
  typedef ValueIntT<int32_t> ValueInt;
  /// 32-bit integer value object.
  typedef ValueIntT<int32_t> ValueInt32;
  /// 32-bit unsigned integer value object.
  typedef ValueIntT<uint32_t> ValueUInt32;
  /// 64-bit integer value object.
  typedef ValueIntT<int64_t> ValueInt64;
  /// 64-bit unsigned integer value object.
  typedef ValueIntT<uint64_t> ValueUInt64;

  /// Time-stamp value object.
  typedef ValueIntT<Radiant::TimeStamp> ValueTimeStamp;

}

#endif
