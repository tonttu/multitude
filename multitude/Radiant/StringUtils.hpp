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

#include <sstream>
#include <QString>
#include <typeinfo>
#include <type_traits>
#include <typeinfo>

#include <stdlib.h>

/// Use atof and atod from assimp
#include "fast_atof.h"

namespace Radiant
{
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
    inline typename std::enable_if<!std::is_enum<T>::value, QString>::type toString(const T & x)
    {
      std::ostringstream os;
      os << x;
      return QString::fromUtf8(os.str().c_str());
    }

    template <typename T>
    inline typename std::enable_if<std::is_enum<T>::value, QString>::type toString(const T & x)
    {
      std::ostringstream os;
      os << long(x);
      return QString::fromUtf8(os.str().c_str());
    }

    /// Convert boolean to string
    /// @param b boolean to convert
    /// @return "1" or "0"
    template <> inline QString toString<bool>(const bool & b) { return b ? "1" : "0"; }

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

    /// Demangle names used by the compiler
    /// for example "N12MultiWidgets11ImageWidgetE" -> "MultiWidgets::ImageWidget"
    /// If the name can't be parsed, the original string is returned
    /// @param name Mangled symbol name
    /// @returns Demangled symbol name
    RADIANT_API QByteArray demangle(const char * name);

    template <typename T>
    inline QByteArray type(const T & t)
    {
      return demangle(typeid(t).name());
    }

    template <typename T>
    inline QByteArray type()
    {
      return demangle(typeid(T).name());
    }
  }
}

#endif
