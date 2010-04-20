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
  template<class T>
      class VALUABLE_API ValueStringT : public ValueObject
  {
  public:
    ValueStringT() : ValueObject() {}
    /// @copydoc ValueObject::ValueObject(HasValues *, const std::string &, bool transit)
    ValueStringT(HasValues * parent, const std::string & name,
                 const T & v, bool transit = false);
    /// @copydoc ValueObject::ValueObject(HasValues *, const std::string &, bool transit)
    ValueStringT(HasValues * parent, const std::string & name,
                 const char * v, bool transit = false);
    /// @copydoc ValueObject::ValueObject(HasValues *, const std::string &, bool transit)
    ValueStringT(HasValues * parent, const std::string & name,
                 bool transit = false);

    virtual void processMessage(const char * id, Radiant::BinaryData & data);

    /// Copies a string
    ValueStringT<T> & operator = (const ValueStringT<T> & i)
                                 { m_value = i.m_value; VALUEMIT_STD_OP }
    /// Copies a string
    ValueStringT<T> & operator = (const T & i) { m_value = i; VALUEMIT_STD_OP }

    /// Compares if two strings are equal
    bool operator == (const T & that) { return that == m_value; }
    /// Compares if two strings are not equal
    bool operator != (const T & that) { return that != m_value; }

    /// Returns the value as float
    float asFloat(bool * const ok = 0) const;
    /// Returns the value as integer
    int asInt(bool * const ok = 0) const;
    /// Returns the value as string
    std::string asString(bool * const ok = 0) const;
    /// Returns the value as wide-byte string
    std::wstring asWString(bool * const ok = 0) const;

    /// Returns the string
    const T & str() const { return m_value; }

    /// @todo use ValueTyped<T> or something similar instead
    operator const T & () const { return m_value; }
    const T * operator->() const { return &m_value; }

    virtual bool set(const std::string & v);

    const char * type() const { return VO_TYPE_STRING; }

    ArchiveElement & serialize(Archive & archive);
    bool deserialize(ArchiveElement & element);

    /// Makes the string empty
    void clear() { m_value.clear(); }

    /// Returns the length of the string
    unsigned size() const { return m_value.size(); }

  private:
    T m_value;
  };

  /// A byte string value object
  typedef ValueStringT<std::string> ValueString;
  /// A wide-byte string value object
  typedef ValueStringT<std::wstring> ValueWString;

  // Instantiation of template classes
  // See ValueStringImpl.hpp for std::wstring member specializations

}

#undef VALUEMIT_STD_OP

#endif
