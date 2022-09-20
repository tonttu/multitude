/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#pragma once

#include <QtGlobal>
#include <QString>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)

#include <string_view>

namespace std
{
  template <>
  struct hash<QString>
  {
    inline size_t operator()(const QString & x) const
    {
      const char16_t* bytes = reinterpret_cast<const char16_t*>(x.utf16());
      return std::hash<std::u16string_view>{}(std::u16string_view(bytes, x.size()));
    }
  };
}
#endif
