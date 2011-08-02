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

  namespace StringUtils
  {

    /// A list of strings
    typedef std::list<std::string>    StringList;
    /// A list of wide-byte strings
    typedef std::list<std::wstring>   WStringList;

    /// Check if wstring a begins with b
    /// @param a String to check
    /// @param b Substring to search for
    /// @returns True if string a begins with string b
    RADIANT_API bool beginsWith(const std::wstring & a,
                                const std::wstring & b);

    /// @copydoc beginsWith
    RADIANT_API bool beginsWith(const std::string & a,
                                const std::string & b);

    /// Remove non-visible characters from std::string.
    /// @param s String to erase from
    RADIANT_API void eraseNonVisibles(std::string & s);
    /// @copydoc eraseNonVisibles
    RADIANT_API void eraseNonVisibles(std::wstring & s);

    /// Convert std::string to double.
    /// @todo Is duplicate to the stuff at the bottom?
    /// @param str String to convert
    /// @param precision Precision of returned double
    /// @returns Converted value
    RADIANT_API double stdStringToDouble(const std::string & str, const int precision = 12);

    /// Convert double to std::string.
    /// @todo Is duplicate to the stuff at the bottom?
    /// @param value Double to convert
    /// @param precision Precision of returned string
    /// @returns Value as a string
    RADIANT_API std::string doubleToStdString(const double value, const int precision = 12);

    /// Convert std::string to std::wstring.
    /// @param str String to convert
    /// @returns Value as a wstring
    RADIANT_API std::wstring stdStringToStdWstring(const std::string & str);

    /// Convert std::wstring to std::string
    /// @warning: non-ASCII characters may be lost in conversion.
    /// @param wstr String to convert
    /// @returns Wstring as a string
    RADIANT_API std::string stdWstringToStdString(const std::wstring & wstr);

    /// Split std::string into sub-strings.
    /// @param s String to split
    /// @param delims String of delimiters
    /// @param out List of strings with results
    /// @param skipEmpty Skip empty results if true
    RADIANT_API void split(const std::string & s, const std::string & delims, StringList & out, bool skipEmpty = true);
    /// Split std::wstring into sub-strings.
    /// @param ws String to split
    /// @param delims String of delimiters
    /// @param out List of strings with results
    /// @param skipEmpty Skip empty results if true
    RADIANT_API void split(const std::wstring & ws, const std::wstring & delims, WStringList & out, bool skipEmpty = true);

    /// Merges a list of strings into a single string
    /// @param dest Destination string
    /// @param src List of strings to merge
    RADIANT_API void merge(std::wstring & dest, const WStringList & src);

    /// Locate char in string
    /// @param str String to search through
    /// @param c character to search for
    /// @returns Pointer to position of found character or pointer to the terminating NULL-char if the character hasn't been found
    RADIANT_API const char * strchrnul(const char * str, int c);

    /// Count the number of lines in the string.
    /// @param s Text to count lines of
    /// @returns Number of lines found in s
    RADIANT_API int lineCount(const char * s);

    /// Convert utf8 string to wide string.
    /// @param dest Destination wstring
    /// @param src Source UTF8 string
    RADIANT_API void utf8ToStdWstring(std::wstring & dest, const std::string & src);
    /// Convert utf8 string to wide string
    /** This function is effectively the same as #utf8ToStdWstring,
        but the this function is usually slightly easier to use, and
        it is slightly slower.

    */
    /// @param src UTF8 string to convert
    /// @returns Converted wstring
    RADIANT_API std::wstring utf8AsStdWstring(const std::string & src);

    /// Convert wide string to utf8 string.
    /// @param dest Destination UTF8 string
    /// @param src Source string
    RADIANT_API void stdWstringToUtf8(std::string & dest, const std::wstring & src);
    /// Convert wide string to utf8 string.
    /// @param src wstring to convert
    /// @returns Converted wstring
    RADIANT_API std::string stdWstringAsUtf8(const std::wstring & src);

    /// Count the number of decoded unicode characters in a utf8 string.
    /// @param src UTF8 string
    /// @returns Number of unicode characters
    RADIANT_API int utf8DecodedLength(const std::string & src);
    /// Count the number of encoded utf8 bytes characters in a wide string.
    /// @param src UTF8 string
    /// @returns Number of unicode characters
    RADIANT_API int utf8EncodedLength(const std::wstring & src);
    /// Returns the lower-case version of the ascii string
    /// @param src String to convert to lower-case
    /// @returns src converted to lowercase
    RADIANT_API std::string lowerCase(const std::string & src);

    /// Converts ASCII string to uppercase
    /// @param c Character to convert
    /// @returns Uppercase character
    RADIANT_API char upperCaseASCII(char c);

    /// Replaces given characters of the string with other characters
    /// @param str String to change
    /// @param from Character to change
    /// @param to Character to change into
    RADIANT_API void replace(std::string & str, char from, char to);


    /** Finds the str in strings and return the index. The
    strings-variable is terminated by null string. If the str is
    not found in the strings, then -1 is returned. */
    /// @param strings List of strings to search through
    /// @param str String to search for
    /// @returns index of the item in the stringlist, or -1 if not found
    RADIANT_API int which(const char ** strings, const char * str);

    /** Finds the str in strings and return the index. */
    /// @copydoc which
    RADIANT_API int which(const StringList & strings, const std::string & str);

    /// Converts to string
    /// @todo Rename to toString
    /// @param x Value to convert
    /// @returns Value as a string
    template<class T>
    inline std::string stringify(const T & x) {
        std::ostringstream os;
        os << x;
        return os.str();
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
    RADIANT_API std::string getLastErrorMessage();
#endif

    /// Demangle names used by the compiler
    /// for example "N12MultiWidgets11ImageWidgetE" -> "MultiWidgets::ImageWidget"
    /// If the name can't be parsed, the original string is returned
    /// @param name Mangled symbol name
    /// @returns Demangled symbol name
    RADIANT_API std::string demangle(const char * name);

  }
}

#endif
