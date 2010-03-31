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
  ///@todo Doc
  template<class T>
      class VALUABLE_API ValueStringT : public ValueObject
  {
  public:
    ValueStringT() : ValueObject() {}
    ValueStringT(HasValues * parent, const std::string & name,
                 const T & v, bool transit = false);

    ValueStringT(HasValues * parent, const std::string & name,
                 const char * v, bool transit = false);

    ValueStringT(HasValues * parent, const std::string & name,
                 bool transit = false);

    virtual void processMessage(const char * id, Radiant::BinaryData & data);

    ValueStringT<T> & operator = (const ValueStringT<T> & i)
                                 { m_value = i.m_value; VALUEMIT_STD_OP }
    ValueStringT<T> & operator = (const T & i) { m_value = i; VALUEMIT_STD_OP }

    bool operator == (const T & that) { return that == m_value; }
    bool operator != (const T & that) { return that != m_value; }

    float asFloat(bool * const ok = 0) const;
    int asInt(bool * const ok = 0) const;
    std::string asString(bool * const ok = 0) const;
    std::wstring asWString(bool * const ok = 0) const;

    const T & str() const { return m_value; }

    virtual bool set(const std::string & v);

    const char * type() const { return VO_TYPE_STRING; }

    DOMElement serializeXML(DOMDocument * doc);
    bool deserializeXML(DOMElement element);

    void clear() { m_value.clear(); }

    unsigned size() const { return m_value.size(); }

  private:
    T m_value;
  };

  typedef ValueStringT<std::string> ValueString;
  typedef ValueStringT<std::wstring> ValueWString;

  // Instantiation of template classes
  // See ValueStringImpl.hpp for std::wstring member specializations

}

#undef VALUEMIT_STD_OP

#endif
