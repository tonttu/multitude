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

#ifndef VALUABLE_VALUE_STRING_IMPL_HPP
#define VALUABLE_VALUE_STRING_IMPL_HPP

#include "ValueString.hpp"
#include "DOMElement.hpp"
#include "DOMDocument.hpp"

#include <Radiant/StringUtils.hpp>

#define STD_EM this->emitChange();
#define VALUEMIT_STD_OP this->emitChange(); return *this;

/// @cond

namespace Valuable
{


  template<class T>
  ValueStringT<T>::ValueStringT(HasValues * parent, const QString & name,
                const T & v, bool transit)
    : Base(parent, name, v, transit)
  {}

  template<class T>
  ValueStringT<T>::ValueStringT(HasValues * parent, const QString & name,
                const char * v, bool transit)
    : Base(parent, name, v, transit)
  {}


  template<class T>
  ValueStringT<T>::ValueStringT(HasValues * parent, const QString & name,
                bool transit)
    : Base(parent, name, T(), transit)
  {}


  template<class T>
  void ValueStringT<T>::processMessage(const char * id, Radiant::BinaryData & data)
  {
      (void) id;
      bool ok = true;
      T tmp = data.read<T>(&ok);
      if(ok)
    *this = tmp;
      /*
      Radiant::info("ValueStringT<T>::processMessage # Ok = %d %s",
                    (int) ok, m_value.c_str());
      */
  }

  template<class T>
  bool ValueStringT<T>::deserialize(ArchiveElement & element)
  {
    Base::m_value = T(element.get());

    STD_EM;

    return true;
  }

  template<class T>
  float ValueStringT<T>::asFloat(bool * const ok) const
  {
    if(ok) *ok = true;
    return float(atof(Base::m_value.c_str()));
  }

  template<class T>
  int ValueStringT<T>::asInt(bool * const ok) const
  {
    if(ok) *ok = true;
    return atoi(Base::m_value.c_str());
  }

  template<class T>
  QString ValueStringT<T>::asString(bool * const ok) const
  {
    if(ok) *ok = true;
    return Base::m_value;
  }

  template<class T>
  QString ValueStringT<T>::asWString(bool * const ok) const
  {
    if(ok) *ok = true;
    QString tmp;
    Radiant::StringUtils::utf8ToStdWstring(tmp, Base::m_value);
    return tmp;
  }

  template<class T>
  bool ValueStringT<T>::set(const QString & v)
  {
    Base::m_value = T(v);
    STD_EM;
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  // Specializations for wide strings.

  /*
  template <>
  ValueStringT<QString>::ValueStringT()
  : ValueObject()
  {}

  template <>
  ValueStringT<QString>::ValueStringT(HasValues * parent, const QString & name, const QString & v, bool transit)
  : ValueObject(parent, name, transit),
  m_value(v)
  {}
  */

  // copybrief ValueObject::ValueObject(HasValues *, const QString &, bool transit)
  template<>
  ValueStringT<QString>::ValueStringT(HasValues * parent, const QString & name,
                       const char * v, bool transit)
    : Base(parent, name, QString(), transit)
  {
    QString tmp(v);
    Radiant::StringUtils::utf8ToStdWstring(m_value, tmp);
    m_orig = m_value;
  }


  template <> ValueStringT<QString> & ValueStringT<QString>::operator = (const ValueStringT<QString> & i)
  {
    m_value = i.m_value;
    VALUEMIT_STD_OP
  }

  template <>
      ValueStringT<QString> & ValueStringT<QString>::operator=(const QString & i)
                                                                        {
    m_value = i;
    VALUEMIT_STD_OP
  }

  template <>
  float ValueStringT<QString>::asFloat(bool * const ok) const
  {
    if(ok) *ok = true;
    QString tmp;
    Radiant::StringUtils::stdWstringToUtf8(tmp, m_value);
    return float(atof(tmp.c_str()));
  }

  /// Converts the string to integer
  template <>
  int ValueStringT<QString>::asInt(bool * const ok) const
  {
    if(ok) *ok = true;
    QString tmp;
    Radiant::StringUtils::stdWstringToUtf8(tmp, m_value);
    return atoi(tmp.c_str());
  }

  /// Converts the wide-byte string to ascii string
  template<>
  QString ValueStringT<QString>::asString(bool * const ok) const
  {
    if(ok) *ok = true;
    QString tmp;
    Radiant::StringUtils::stdWstringToUtf8(tmp, m_value);
    return tmp;
  }

  /// Converts the wide-byte string to ascii string
  template<>
  QString ValueStringT<QString>::asWString(bool * const ok) const
  {
    if(ok) *ok = true;
    return m_value;
  }

  template<>
  bool ValueStringT<QString>::set(const QString & v)
  {
    Radiant::StringUtils::utf8ToStdWstring(m_value, v);
    STD_EM;
    return true;
  }

  template<>
  ArchiveElement & ValueStringT<QString>::serialize(Archive & archive)
  {
    return ValueObject::serialize(archive);
  }

  template<>
  ArchiveElement & ValueStringT<QString>::serialize(Archive & archive)
  {
    if(name().empty()) {
      Radiant::error("ValueWString::serialize # attempt to serialize object with no name");
      return archive.emptyElement();
    }

    ArchiveElement & elem = archive.createElement(name().c_str());
    elem.add("type", type());

    QString ws = asWString();
    elem.set(ws);

    return elem;
  }

  template<>
  bool ValueStringT<QString>::deserialize(ArchiveElement & element)
  {
    m_value = element.getW();
    STD_EM;
    return true;
  }

}

/// @endcond

#undef VALUEMIT_STD_OP
#undef STD_EN

#endif
