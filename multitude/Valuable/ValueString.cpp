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

#include "ValueString.hpp"

namespace Valuable
{
  ValueString::ValueString() {}
  ValueString::ValueString(HasValues * parent, const QString & name,
                           const QString & v, bool transit)
    : Base(parent, name, v, transit)
  {}

  ValueString::ValueString(HasValues * parent, const QString & name,
                           const char * v, bool transit)
    : Base(parent, name, v, transit)
  {}

  ValueString::ValueString(HasValues * parent, const QString & name,
                           bool transit)
    : Base(parent, name, QString(), transit)
  {}

  void ValueString::processMessage(const char * /*id*/, Radiant::BinaryData & data)
  {
    bool ok = true;
    QString tmp = data.read<QString>(&ok);
    if(ok)
      *this = tmp;
  }

  bool ValueString::deserialize(ArchiveElement & element)
  {
    *this = element.get();
    return true;
  }

  float ValueString::asFloat(bool * const ok) const
  {
    return m_value.toFloat(ok);
  }

  int ValueString::asInt(bool * const ok) const
  {
    return m_value.toInt(ok, 0);
  }

  QString ValueString::asString(bool * const ok) const
  {
    if(ok) *ok = true;
    return m_value.toUtf8().data();
  }

  bool ValueString::set(const QString & v)
  {
    *this = v;
    return true;
  }

  ValueString & ValueString::operator=(const ValueString & i)
  {
    return (*this = i.m_value);
  }

  ValueString & ValueString::operator=(const QString & i)
  {
    if(m_value != i) {
      m_value = i;
      emitChange();
    }
    return *this;
  }

  QString ValueString::operator+(const ValueString & i) const
  {
    return m_value + i.m_value;
  }

  QString ValueString::operator+(const QString & i) const
  {
    return m_value + i;
  }

  QString ValueString::operator+(const char * utf8) const
  {
    return m_value + QString::fromUtf8(utf8);
  }

  bool ValueString::operator == (const QString & that) const
  {
    return m_value == that;
  }

  bool ValueString::operator != (const QString & that) const
  {
    return m_value != that;
  }

  void ValueString::clear()
  {
    *this = "";
  }

  unsigned ValueString::size() const
  {
    return (unsigned) m_value.size();
  }
}

QString operator + (const QString & a, const Valuable::ValueString & b)
{
  return a + *b;
}
