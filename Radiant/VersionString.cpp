/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "VersionString.hpp"

#include <QRegExp>

namespace Radiant
{

  VersionString::VersionString(const QString &str)
    : m_str(str)
  {}

  bool VersionString::operator<(const VersionString &other) const
  {
    if(m_str.isEmpty() || other.m_str.isEmpty()) return m_str < other.m_str;

    QRegExp r1("\\d+"), r2("\\d+");

    int offset1 = 0, offset2 = 0;

    while(true) {
      int i1 = r1.indexIn(m_str, offset1);
      int i2 = r2.indexIn(other.m_str, offset2);

      /// "abc" "xyz"
      if(i1 == -1 || i2 == -1)
        return m_str.mid(offset1) < other.m_str.mid(offset2);

      /// "abc12" "abc34"
      if(i1 > offset1 && i2 > offset2) {
        if(m_str.mid(offset1, i1-offset1) != other.m_str.mid(offset2, i2-offset2))
          return m_str.mid(offset1, i1-offset1) < other.m_str.mid(offset2, i2-offset2);
        offset1 = i1;
        offset2 = i2;
      } else if(i1 == offset1 && i2 == offset2) {
        long n1 = r1.cap(0).toLong();
        long n2 = r2.cap(0).toLong();
        if(n1 != n2) return n1 < n2;
        offset1 += r1.cap(0).length();
        offset2 += r2.cap(0).length();
      } else {
        /// "12abc" "qwe12" or "abc12" "123qwe"
        return m_str.mid(offset1) < other.m_str.mid(offset2);
      }
    }
  }

  bool VersionString::operator<=(const VersionString &other) const
  {
    return m_str == other.m_str || operator<(other);
  }

  bool VersionString::operator>(const VersionString &other) const
  {
    return !(m_str == other.m_str || operator<(other));
  }

  bool VersionString::operator>=(const VersionString &other) const
  {
    return !operator<(other);
  }

  bool VersionString::operator==(const VersionString &other) const
  {
    return m_str == other.m_str;
  }

  bool VersionString::operator!=(const VersionString &other) const
  {
    return m_str != other.m_str;
  }

  const QString &VersionString::str() const
  {
    return m_str;
  }

}
