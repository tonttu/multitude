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
    ValueIntT(HasValues * parent, const std::string & name, T v, bool transit = false)
        : ValueNumeric<T>(parent, name, v, transit)
    {}

    ValueIntT<T> & operator = (T i) { Base::m_value = i; VALUEMIT_STD_OP }

    operator const T & () const { return Base::m_value; }
    /// Returns the data in its native format
    const T & native() const { return Base::m_value; }

    ValueIntT<T> & operator |= (T i) { Base::m_value |= i; VALUEMIT_STD_OP }
    ValueIntT<T> & operator &= (T i) { Base::m_value &= i; VALUEMIT_STD_OP }

    // Postfix
    ValueIntT<T> & operator ++ (int) { Base::m_value++; VALUEMIT_STD_OP }
    ValueIntT<T> & operator -- (int) { Base::m_value--; VALUEMIT_STD_OP }

    // Prefix
    ValueIntT<T> & operator ++ () { ++Base::m_value; VALUEMIT_STD_OP }
    ValueIntT<T> & operator -- () { --Base::m_value; VALUEMIT_STD_OP }

    // Shift
    ValueIntT<T> & operator <<= (int i) { Base::m_value <<= i; VALUEMIT_STD_OP }
    ValueIntT<T> & operator >>= (int i) { Base::m_value >>= i; VALUEMIT_STD_OP }

    bool operator < (const T & i) const { return Base::m_value < i; }
    bool operator <= (const T & i) const { return Base::m_value <= i; }
    bool operator > (const T & i) const { return Base::m_value > i; }
    bool operator >= (const T & i) const { return Base::m_value >= i; }

    const char * type() const { return VO_TYPE_INT; }

    virtual void processMessage(const char * id, Radiant::BinaryData & data);

    bool deserializeXML(DOMElement element);
  };

  /// Integer value object.
  typedef ValueIntT<int32_t> ValueInt;
  typedef ValueIntT<int32_t> ValueInt32;
  typedef ValueIntT<uint32_t> ValueUInt32;
  typedef ValueIntT<int64_t> ValueInt64;
  typedef ValueIntT<uint64_t> ValueUInt64;

  /// Time-stamp value object.
  typedef ValueIntT<Radiant::TimeStamp> ValueTimeStamp;
  /*
#ifdef WIN32
#ifdef VALUABLE_EXPORT
  // In WIN32 template classes must be instantiated to be exported
  template class ValueIntT<int>;
  template class ValueIntT<Radiant::TimeStamp>;
#endif
#endif
*/
}

#undef VALUEMIT_STD_OP

#endif
