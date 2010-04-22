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

#define VALUEMIT_STD_OP this->emitChange(); return *this;

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
    ValueIntT() : Base() {}
    /// @copydoc ValueObject::ValueObject(HasValues *, const std::string &, bool transit)
    ValueIntT(HasValues * parent, const std::string & name, T v, bool transit = false)
        : ValueNumeric<T>(parent, name, v, transit)
    {}

    /// Copy an integer
    ValueIntT<T> & operator = (T i) { Base::m_value = i; VALUEMIT_STD_OP }

    /// Convert the value object to integer
    operator const T & () const { return Base::m_value; }
    /// Returns the data in its native format
    const T & data() const { return Base::m_value; }

    /// Does a logical OR for the integer
    ValueIntT<T> & operator |= (T i) { Base::m_value |= i; VALUEMIT_STD_OP }
    /// Does a logical AND for the integer
    ValueIntT<T> & operator &= (T i) { Base::m_value &= i; VALUEMIT_STD_OP }

    /// Postfix increment
    ValueIntT<T> & operator ++ (int) { Base::m_value++; VALUEMIT_STD_OP }
    /// Postfix decrement
    ValueIntT<T> & operator -- (int) { Base::m_value--; VALUEMIT_STD_OP }

    /// Prefix increment
    ValueIntT<T> & operator ++ () { ++Base::m_value; VALUEMIT_STD_OP }
    /// Prefix decrement
    ValueIntT<T> & operator -- () { --Base::m_value; VALUEMIT_STD_OP }

    /// Shift left
    ValueIntT<T> & operator <<= (int i) { Base::m_value <<= i; VALUEMIT_STD_OP }
    /// Shift right
    ValueIntT<T> & operator >>= (int i) { Base::m_value >>= i; VALUEMIT_STD_OP }

    /// Compares less than
    bool operator < (const T & i) const { return Base::m_value < i; }
    /// Compares less or equal than
    bool operator <= (const T & i) const { return Base::m_value <= i; }
    /// Compares greater than
    bool operator > (const T & i) const { return Base::m_value > i; }
    /// Compares greater or equal than
    bool operator >= (const T & i) const { return Base::m_value >= i; }

    const char * type() const { return VO_TYPE_INT; }

    virtual void processMessage(const char * id, Radiant::BinaryData & data);

    bool deserialize(ArchiveElement & element);
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

#undef VALUEMIT_STD_OP

#endif
