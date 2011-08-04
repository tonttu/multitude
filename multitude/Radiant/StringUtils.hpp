/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */
#ifndef RADIANT_STRING_UTILS_HPP
#define RADIANT_STRING_UTILS_HPP

#include "Export.hpp"

#include <sstream>
#include <QString>

#include <strings.h> // atoll on windows
#include <stdlib.h>

namespace Radiant
{

  /// StringUtils is a collection of string manipulation functions.

  namespace StringUtils
  {

    /// Remove non-visible characters from QString.
    RADIANT_API void eraseNonVisibles(QString & s);

    /// Converts to string
    /// @todo Rename to toString
    /// @param x Value to convert
    /// @returns Value as a string
    template<class T>
    inline QString stringify(const T & x) {
        /// @todo fix to use qt string utils
        std::ostringstream os;
        os << x;
        return QString::fromUtf8(os.str().c_str());
    }

    /// Convert std::wstring to std::string
    template<>
    inline std::string stringify(const std::wstring & x) {
      std::string out;
      stdWstringToUtf8(out, x);
      return out;
    }

    /// Convert string to integer
    template <class T>
    inline T fromString(const char * str)
    { return T(atoll(str)); }

    template <long>
    inline long fromString(const char * str)
    { return atol(str); }

#ifdef WIN32
    RADIANT_API QString getLastErrorMessage();
#endif

    /// Demangle names used by the compiler
    /// for example "N12MultiWidgets11ImageWidgetE" -> "MultiWidgets::ImageWidget"
    /// If the name can't be parsed, the original string is returned
    /// @param name Mangled symbol name
    /// @returns Demangled symbol name
    RADIANT_API QString demangle(const char * name);

  }
}

#endif
