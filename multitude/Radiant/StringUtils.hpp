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
#include <QString>

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

    /// A list of strings
    typedef std::list<QString>    StringList;
    /// A list of wide-byte strings
    typedef std::list<QString>   WStringList;

    /// Check if wstring a begins with b
    RADIANT_API bool beginsWith(const QString & a,
                                const QString & b);

    /// @copydoc beginsWith
    RADIANT_API bool beginsWith(const QString & a,
                                const QString & b);

    /// Remove non-visible characters from QString.
    RADIANT_API void eraseNonVisibles(QString & s);
    /// @copydoc eraseNonVisibles
    RADIANT_API void eraseNonVisibles(QString & s);

    /// Convert QString to double.
    /// @todo Is duplicate to the stuff at the bottom?
    RADIANT_API double stdStringToDouble(const QString & str, const int precision = 12);

    /// Convert double to QString.
    /// @todo Is duplicate to the stuff at the bottom?
    RADIANT_API QString doubleToStdString(const double value, const int precision = 12);

    /// Convert QString to QString.
    RADIANT_API QString stdStringToStdWstring(const QString & str);

    /// Convert QString to QString
    /// @warning: non-ASCII characters may be lost in conversion.
    RADIANT_API QString stdWstringToStdString(const QString & wstr);

    /// Split QString into sub-strings.
    RADIANT_API void split(const QString & s, const QString & delim, StringList & out, bool skipEmpty = true);
    /// Split QString into sub-strings.
    RADIANT_API void split(const QString & ws, const QString & delim, WStringList & out);

    /// Merges a list of strings into a single string
    RADIANT_API void merge(QString & dest, const WStringList & src);

    /// Locate char in string
    RADIANT_API const char * strchrnul(const char * str, int c);

    /// Count the number of lines in the string.
    RADIANT_API int lineCount(const char * s);

    /// Convert utf8 string to wide string.
    RADIANT_API void utf8ToStdWstring(QString & dest, const QString & src);
    /// Convert utf8 string to wide string
    /** This function is effectively the same as #utf8ToStdWstring,
        but the this function is usually slightly easier to use, and
        it is slightly slower.

    */
    RADIANT_API QString utf8AsStdWstring(const QString & src);

    /// Convert wide string to utf8 string.
    RADIANT_API void stdWstringToUtf8(QString & dest, const QString & src);
    /// Convert wide string to utf8 string.
    RADIANT_API QString stdWstringAsUtf8(const QString & src);

    /// Count the number of decoded unicode characters in a utf8 string.
    RADIANT_API int utf8DecodedLength(const QString & src);
    /// Count the number of encoded utf8 bytes characters in a wide string.
    RADIANT_API int utf8EncodedLength(const QString & src);
    /// Returns the lower-case version of the ascii string
    RADIANT_API QString lowerCase(const QString & src);

    /// Converts ASCII string to uppercase
    RADIANT_API char upperCaseASCII(char c);

    /// Replaces given characters of the string with other characters
    RADIANT_API void replace(QString & str, char from, char to);


    /** Finds the str in strings and return the index. The
    strings-variable is terminated by null string. If the str is
    not found in the strings, then -1 is returned. */
    RADIANT_API int which(const char ** strings, const char * str);

    /** Finds the str in strings and return the index. If the str is
    not found in the strings, then -1 is returned. */
    RADIANT_API int which(const StringList & strings, const QString & str);

    /// Converts to string
    /// @todo Rename to toString
    template<class T>
    inline QString stringify(T x) {
        std::ostringstream os;
        os << x;
        return os.str();
    }

    /// Converts from string
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

    /// Returns "yes" or "no" given the parameter
    RADIANT_API const char * yesNo(bool yes);

#ifdef WIN32
    RADIANT_API QString getLastErrorMessage();
#endif

  }
}

#endif
