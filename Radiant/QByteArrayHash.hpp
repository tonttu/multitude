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
#include <QByteArray>

#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)

#include <string_view>

namespace std
{
  template <>
  struct hash<QByteArray>
  {
    inline size_t operator()(const QByteArray & x) const
    {
      return std::hash<std::string_view>()(std::string_view(x.data(), x.size()));
    }
  };
}
#endif
