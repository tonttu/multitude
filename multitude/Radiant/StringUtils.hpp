/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */
#ifndef RADIANT_STRING_UTILS_HPP
#define RADIANT_STRING_UTILS_HPP

#include "Export.hpp"
#include "Defines.hpp"

#include <QString>

#include <memory>
#include <sstream>
#include <stdlib.h>
#include <type_traits>
#include <typeinfo>
#include <typeinfo>

/// Use atof and atod from assimp
#include "fast_atof.h"

namespace Radiant
{
  /// is_specialization<std::vector<int>, std::vector>::value == true
  /// https://stackoverflow.com/a/28796458
  template <typename Test, template<typename...> class Ref>
  struct is_specialization : std::false_type {};

  template <template<typename...> class Ref, typename... Args>
  struct is_specialization<Ref<Args...>, Ref>: std::true_type {};

  /// StringUtils is a collection of string manipulation functions.
  namespace StringUtils
  {

    /// Remove non-visible characters from QString.
    /// @param s String to process
    RADIANT_API void eraseNonVisibles(QString & s);

    /// Converts to string
    /// @param x Value to convert
    /// @returns Value as a string
    /// @tparam T Type of the value to convert
    template<class T>
    inline QString toString(const T & x)
    {
      if constexpr(is_specialization<T, std::shared_ptr>::value ||
                   is_specialization<T, std::unique_ptr>::value ||
                   std::is_pointer<T>::value) {
        return x ? toString(*x) : "<null>";
      } else if constexpr(std::is_enum_v<T>) {
        std::ostringstream os;
        os << long(x);
        return QString::fromStdString(os.str());
      } else if constexpr(std::is_same_v<T, bool>) {
        return x ? "1" : "0";
      } else if constexpr(std::is_same_v<T, QString>) {
        return x;
      } else if constexpr(std::is_same_v<T, QByteArray>) {
        return QString::fromUtf8(x);
      } else {
        std::ostringstream os;
        os << x;
        return QString::fromStdString(os.str());
      }
    }

    /// @cond
    template <typename T> MULTI_ATTR_DEPRECATED("stringify will be removed in Cornerstone 2.1, use toString instead",
                          inline QString stringify(const T & x));

    template <typename T>
    inline QString stringify(const T & x)
    {
      return toString(x);
    }
    /// @endcond

    /// Convert string to type T
    /// @param str String to convert
    /// @return Value converted from string
    /// @tparam T Type of value returned
    template <typename T>
    inline typename std::enable_if<!std::is_enum<T>::value, T>::type fromString(const QByteArray & str)
    {
      std::istringstream is(str.data());
      T t;
      is >> t;
      return t;
    }

    /// Convert string to type T
    /// @param str String to convert
    /// @return Value converted from string
    /// @tparam T Type of value returned
    template <typename T>
    inline typename std::enable_if<std::is_enum<T>::value, T>::type fromString(const QByteArray & str)
    {
      std::istringstream is(str.data());
      long t;
      is >> t;
      return (T)t;
    }

    /// @cond

    template <> inline short fromString<short>(const QByteArray & str) { return str.toShort(); }
    template <> inline unsigned short fromString<unsigned short>(const QByteArray & str) { return str.toUShort(); }
    template <> inline int fromString<int>(const QByteArray & str) { return str.toInt(); }
    template <> inline unsigned int fromString<unsigned int>(const QByteArray & str) { return str.toUInt(); }
    template <> inline long fromString<long>(const QByteArray & str) { return str.toLong(); }
    template <> inline unsigned long fromString<unsigned long>(const QByteArray & str) { return str.toULong(); }
    template <> inline long long fromString<long long>(const QByteArray & str) { return str.toLongLong(); }
    template <> inline unsigned long long fromString<unsigned long long>(const QByteArray & str) { return str.toULongLong(); }
    template <> inline float fromString<float>(const QByteArray & str) { return str.toFloat(); }
    template <> inline double fromString<double>(const QByteArray & str) { return str.toDouble(); }
    template <> inline bool fromString<bool>(const QByteArray & str) { return str.toInt() > 0; }

    /// @endcond

    /// Locale-independent string to float conversions. Notice that the return
    /// value of atof is float and not double, for double use atod.
    inline float atof(const char * str) { return Assimp::fast_atof(str); }
    inline double atod(const char * str) { return Assimp::fast_atod(str); }

#ifdef WIN32
    RADIANT_API QString getLastErrorMessage();
#endif

    /// Demangle names used by the compiler.
    /// For example: "N12MultiWidgets11ImageWidgetE" -> "MultiWidgets::ImageWidget"
    /// and "class Foo" -> "Foo".
    /// If the name can't be parsed, the original string is returned
    /// @param name Mangled symbol name
    /// @returns Demangled symbol name
    RADIANT_API QByteArray demangle(const char * name);

    template <typename T>
    inline QByteArray type(const T & t)
    {
      return demangle(typeid(t).name());
    }

    inline QByteArray demangle(const std::type_info & typeinfo)
    {
      return demangle(typeinfo.name());
    }

    template <typename T>
    inline QByteArray type()
    {
      return demangle(typeid(T).name());
    }
  }
}

#endif
