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
