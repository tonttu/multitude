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

#ifndef VALUABLE_VALUE_STRING_HPP
#define VALUABLE_VALUE_STRING_HPP

#include <Radiant/StringUtils.hpp>

#include <Valuable/Export.hpp>
#include <Valuable/ValueNumeric.hpp>

#define VALUEMIT_STD_OP this->emitChange(); return *this;

#define VO_TYPE_STRING "string"

namespace Valuable
{

  /// String value
  /** This template class is used to implement both normal 7/8-bit
      strings and wide strings*/
  template<typename T>
      class VALUABLE_API ValueStringT : public ValueObjectT<T>
  {
    typedef ValueObjectT<T> Base;

  public:

    /// The character type of this string class
    /** This type depends on the template class. */
    typedef typename T::value_type char_type;

    ValueStringT() : Base() {}
    /// @copydoc ValueObject::ValueObject(HasValues *, const std::string &, bool transit)
    /// @param v The string to store in this object
    ValueStringT(HasValues * host, const std::string & name,
                 const T & v, bool transit = false);
    /// @copydoc ValueObject::ValueObject(HasValues *, const std::string &, bool transit)
    /// @param v The string to store in this object
    ValueStringT(HasValues * host, const std::string & name,
                 const char * v, bool transit = false);
    /// @copydoc ValueObject::ValueObject(HasValues *, const std::string &, bool transit)
    ValueStringT(HasValues * host, const std::string & name,
                 bool transit = false);

    virtual void processMessage(const char * id, Radiant::BinaryData & data);

    /// Copies a string
    ValueStringT<T> & operator = (const ValueStringT<T> & i)
                                 { Base::m_value = i.m_value; VALUEMIT_STD_OP }
    /// Copies a string
    ValueStringT<T> & operator = (const T & i) { Base::m_value = i; VALUEMIT_STD_OP }

    /// Concatenates two strings
    /// @param i The string to be appended to this string
    /// @return A new string that contains both this string, and the argument string.
    T operator + (const ValueStringT<T> & i) const
    { return Base::m_value + i.m_value; }


    /// Concatenates two strings
    T operator + (const T & i) const
    { return Base::m_value + i; }

    /// Concatenates two strings
    T operator + (const char_type * i) const
    { return Base::m_value + T(i); }

        /*
    T operator + (const char * i) const
    { return Base::m_value + i; }
*/

    /// Compares if two strings are equal
    bool operator == (const T & that) const { return that == Base::m_value; }
    /// Compares if two strings are not equal
    bool operator != (const T & that) const { return that != Base::m_value; }

    /// Returns the value as float
    float asFloat(bool * const ok = 0) const;
    /// Returns the value as integer
    int asInt(bool * const ok = 0) const;
    /// Returns the value as string
    std::string asString(bool * const ok = 0) const;
    /// Returns the value as wide-byte string
    std::wstring asWString(bool * const ok = 0) const;

    /// Returns the string
    const T & str() const { return Base::m_value; }

    /// Gets C-style pointer to the string contents.
    /** Replicates std::string::c_str for ease of use. */
    const char_type * c_str() const { return Base::m_value.c_str(); }
    /// Check if the string is empty
    bool empty() const { return Base::m_value.empty(); }


    virtual bool set(const std::string & v);

    const char * type() const { return VO_TYPE_STRING; }

    ArchiveElement serialize(Archive & archive) const;
    bool deserialize(const ArchiveElement & element);

    /// Makes the string empty
    void clear() { Base::m_value.clear(); }

    /// Returns the length of the string
    unsigned size() const { return (unsigned) Base::m_value.size(); }
  };

  /// A byte string value object
  typedef ValueStringT<std::string> ValueString;
  /// A wide-byte string value object
  typedef ValueStringT<std::wstring> ValueWString;

  // Instantiation of template classes
  // See ValueStringImpl.hpp for std::wstring member specializations

}

template <class T>
    T operator + (const T & a, const Valuable::ValueStringT<T> & b)
{
  return a + b.str();
}

#undef VALUEMIT_STD_OP

#endif
