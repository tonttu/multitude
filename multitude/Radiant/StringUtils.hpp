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

#include <list>
#include <sstream>
#include <string>

#include <strings.h> // atoll on windows

#include <Radiant/TimeStamp.hpp>
#include <Radiant/Trace.hpp>

#include <stdlib.h>


namespace Radiant
{

  /// StringUtils is a collection of string manipulation functions.

  /// @todo Documentation
  namespace StringUtils
  {

    typedef std::list<std::string>    StringList;
    typedef std::list<std::wstring>   WStringList;

    /// Check if wstring a begins with b
    RADIANT_API bool beginsWith(const std::wstring & a,
                                const std::wstring & b);

    RADIANT_API bool beginsWith(const std::string & a,
                                const std::string & b);

    /// Remove non-visible characters from std::string.
    RADIANT_API void eraseNonVisibles(std::string & s);
    RADIANT_API void eraseNonVisibles(std::wstring & s);

    /// Convert std::string to double.
    /// @todo Is duplicate to the stuff at the bottom?
    RADIANT_API double stdStringToDouble(const std::string & str, const int precision = 12);

    /// Convert double to std::string.
    /// @todo Is duplicate to the stuff at the bottom?
    RADIANT_API std::string doubleToStdString(const double value, const int precision = 12);

    /// Convert std::string to std::wstring.
    RADIANT_API std::wstring stdStringToStdWstring(const std::string & str);

    /// Convert std::wstring to std::string
    /// @warning: non-ASCII characters may be lost in conversion.
    RADIANT_API std::string stdWstringToStdString(const std::wstring & wstr);

    /// Split std::string into sub-strings.
    RADIANT_API void split(const std::string & s, const std::string & delim, StringList & out, bool skipEmpty = true);
    /// Split std::wstring into sub-strings.
    RADIANT_API void split(const std::wstring & ws, const std::wstring & delim, WStringList & out);

    RADIANT_API void merge(std::wstring & dest, const WStringList & src);

    RADIANT_API const char * strchrnul(const char * str, int c);

    /// Count the number of lines in the string.
    RADIANT_API int lineCount(const char * s);

    /// Convert utf8 string to wide string.
    RADIANT_API void utf8ToStdWstring(std::wstring & dest, const std::string & src);
    /// Convert utf8 string to wide string
    /** This function is effectively the same as #utf8ToStdWstring,
        but the this function is usually slightly easier to use, and
        it is slightly slower.

    */
    RADIANT_API std::wstring utf8AsStdWstring(const std::string & src);

    /// Convert wide string to utf8 string.
    RADIANT_API void stdWstringToUtf8(std::string & dest, const std::wstring & src);
    /// Convert wide string to utf8 string.
    RADIANT_API std::string stdWstringAsUtf8(const std::wstring & src);

    /// Count the number of decoded unicode characters in a utf8 string.
    RADIANT_API int utf8DecodedLength(const std::string & src);
    /// Count the number of encoded utf8 bytes characters in a wide string.
    RADIANT_API int utf8EncodedLength(const std::wstring & src);
    // Returns the lower-case version of the ascii string
    RADIANT_API std::string lowerCase(const std::string & src);

    RADIANT_API char upperCaseASCII(char c);

    /** Finds the str in strings and return the index. The
    strings-variable is terminated by null string. If the str is
    not found in the strings, then -1 is returned. */
    RADIANT_API int which(const char ** strings, const char * str);

    /** Finds the str in strings and return the index. If the str is
    not found in the strings, then -1 is returned. */
    RADIANT_API int which(const StringList & strings, const std::string & str);

    /// @todo Rename to toString
    template<class T>
    inline std::string stringify(T x) {
        std::ostringstream os;
        os << x;
        return os.str();
    }

    template <class T>
    inline T fromString(const char * str)
    { return T(atoll(str)); }

    template <long>
    inline long fromString(const char * str)
    { return atol(str); }

    /* template <int64_t>
    inline int64_t fromString(const char * str)
    { return atoll(str); }
    */

    /* template <Radiant::TimeStamp>
    inline Radiant::TimeStamp fromString(const char * str)
    { return atoll(str); }
    */

    RADIANT_API const char * yesNo(bool yes);

#ifdef WIN32
    RADIANT_API std::string getLastErrorMessage();
#endif

  }
}

#endif
