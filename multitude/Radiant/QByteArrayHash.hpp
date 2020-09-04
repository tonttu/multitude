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
