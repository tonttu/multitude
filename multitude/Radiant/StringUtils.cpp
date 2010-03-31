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

#include "StringUtils.hpp"

#include <sstream>
#include <cassert>

#include <string.h>
#include <strings.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// #include <WinPort.h>
#endif

namespace Radiant
{

  namespace StringUtils
  {

    using namespace std;

    bool beginsWith(const std::wstring & a,
                    const std::wstring & b)
    {
      if(a.size() < b.size())
        return false;

      for(unsigned i = 0; i < b.size(); i++)
        if(a[i] != b[i])
          return false;

      return true;
    }

    bool beginsWith(const std::string & a,
                    const std::string & b)
    {
      if(a.size() < b.size())
        return false;

      for(unsigned i = 0; i < b.size(); i++)
        if(a[i] != b[i])
          return false;

      return true;
    }


    void eraseNonVisibles(string & s)
    {
      for(unsigned i = 0; i < s.size();) {
        if(s[i] < 23)
          s.erase(i, 1);
        else
          i++;
      }
    }

    void eraseNonVisibles(wstring & s)
    {
      for(unsigned i = 0; i < s.size();) {
        if(s[i] < 23)
          s.erase(i, 1);
        else
          i++;
      }
    }

    double stdStringToDouble(const std::string & str, const int precision)
    {
      std::stringstream   ss;
      ss.precision(precision);

      ss << str;
      double  value = 0.0f;
      ss >> value;

      return value;
    }

    std::string doubleToStdString(const double value, const int precision)
    {
      std::stringstream   ss;
      ss.precision(precision);

      ss << value;
      std::string   str;
      ss >> str;

      return str;
    }

    wstring stdStringToStdWstring(const string & str)
    {
      wstring  wstr;

      const size_t l = str.length();
      wstr.resize(l);
      for(size_t i = 0; i < l; i++)
      {
        wstr[i] = wchar_t(str[i]);
      }

      return wstr;
    }

    string stdWstringToStdString(const wstring & wstr)
    {
      string   str;

      const size_t l = wstr.length();
      str.resize(l);
      for(size_t i = 0; i < l; i++)
      {
        str[i] = char(wstr[i]);
      }

      return str;
    }

    void split(const string & s, const string & delim, StringList & out, bool skipEmpty)
    {
      string::size_type offset = 0;
      string::size_type index = 0;

      index = s.find(delim, offset);

      while(index != string::npos) {
        string piece = s.substr(offset, index - offset);

        if(skipEmpty && piece != string(""))
          out.push_back(piece);

        offset += index - offset + delim.length();
        index = s.find(delim, offset);
      }

      string piece = s.substr(offset);

      if(skipEmpty && piece != string(""))
        out.push_back(piece);
    }

    void split(const wstring & ws, const wstring & delim, WStringList & out)
    {
      out.clear();

      if(ws.empty())
      {
        return;
      }

      // Find first a delimiter
      wstring   wscopy(ws);
      size_t    pos = wscopy.find_first_of(delim);

      // Loop until no delimiters left
      while(pos != wscopy.npos)
      {
        out.push_back(wscopy.substr(0, pos + 1));
        wscopy.erase(0, pos + 1);
        pos = wscopy.find_first_of(delim);
      }

      // Push remainder of wstring onto list
      if(!wscopy.empty())
      {
        out.push_back(wscopy);
      }
    }

    void merge(std::wstring & dest, const WStringList & src)
    {
      dest.clear();

      for(WStringList::const_iterator it = src.begin();
      it != src.end(); it++) {
    dest += (*it);
      }
    }

    const char * strchrnul(const char * str, int c)
    {
      while(true) {
    if(str[0] == 0)
      return str;
    else if(str[0] == c)
      return str;
    str++;
      }
    }

    int lineCount(const char * s)
    {
      if(!s)
        return 0;

      int n = s[0] ? 1 : 0;

      for(; s[0] != 0; s++) {
        if(s[0] == '\n')
          n++;
      }

      return n;
    }

    void utf8ToStdWstring(wstring & dest, const string & src)
    {
      int len = utf8DecodedLength(src);

      dest.resize(len);

      const uint8_t * ptr = (const uint8_t *) src.c_str();

      const uint8_t * sentinel = ptr + src.size();

      int characters = 0;

      // http://en.wikipedia.org/wiki/UTF-8

      while(ptr < sentinel) {
        unsigned c0 = *ptr++;

    unsigned left = sentinel - ptr;

    /* trace("utf8ToStdWstring # 0x%x 0x%x (%c)",
           (int) c0, (int) (c0 & 0xE0), (char) c0); */

        if((c0 & 0x80) == 0)
          dest[characters] = c0;
        else if((c0 & 0xE0) == 0xC0) {
      if(left < 1) {
        error("utf8ToStdWstring # Lacking 1 byte from a UTF8 string");
        return;
      }
          unsigned c1 = *ptr++;
          dest[characters] = (c1 & 0x3F) + ((c0 & 0x1F) << 6);
      /*trace("utf8ToStdWstring # Got 2 = 0x%x (%x %x)",
            (int) dest[characters], c0, c1); */
        }
        else if((c0 & 0xF0) == 0xE0) {
      if(left < 2) {
        error("utf8ToStdWstring # Lacking 2 bytes from a UTF8 string");
        return;
      }

          unsigned c1 = *ptr++;
          unsigned c2 = *ptr++;
          dest[characters] = (c2 & 0x3F) + ((((unsigned) c1) & 0x3F) << 6) +
            ((((unsigned) c0) & 0x0F) << 12);
        }
        else if((c0 & 0xF8) == 0xF0) {
      if(left < 3) {
        error("utf8ToStdWstring # Lacking 3 bytes from a UTF8 string");
        return;
      }

          unsigned c1 = *ptr++;
          unsigned c2 = *ptr++;
          unsigned c3 = *ptr++;
          dest[characters] = (c3 & 0x3) + ((c2 & 0x3F) << 6) +
            ((c1 & 0x3F) << 12) + ((c0 & 0x07) << 18);
        }
    else {
      Radiant::error("utf8ToStdWstring # Bad character 0x%x", (int) c0);
          return;
    }

        characters++;
      }
    }

    std::wstring utf8AsStdWstring(const std::string & src)
    {
      std::wstring res;
      utf8ToStdWstring(res, src);
      return res;
    }

    void stdWstringToUtf8(string & dest, const wstring & src)
    {
      int bytes = utf8EncodedLength(src);
      dest.resize(bytes);

      char * ptr = & dest[0];

      for(unsigned i = 0; i < src.size(); i++) {
        unsigned c = src[i];
        if(c <= 0x7F)
          *ptr++ = c;
        else if(c < 0x7FF) {
          *ptr++ = (c >> 6) && 0x1F;
          *ptr++ = c & 0x03F;
        }
        else if((c >= 0x800 && c <= 0xD7FF) ||
            (c >= 0xE000 && c <= 0xFFFF)) {
          *ptr++ = (c >> 12) && 0xF;
          *ptr++ = (c >> 6) && 0x3F;
          *ptr++ = c & 0x03F;
        }
        else if(c >= 0x10000 && c <= 0x10FFFF) {
          *ptr++ = (c >> 18) && 0x7;
          *ptr++ = (c >> 12) && 0x3F;
          *ptr++ = (c >> 6) && 0x3F;
          *ptr++ = c & 0x03F;
        }
        else {
          Radiant::error("stdWstringToUtf8 # Bad Unicode character %x", c);
        }
      }
    }

    std::string stdWstringAsUtf8(const std::wstring & src)
    {
      std::string res;
      stdWstringToUtf8(res, src);
      return res;
    }

    int utf8DecodedLength(const string & src)
    {
      int characters = 0;

      const uint8_t * ptr = (const uint8_t *) src.c_str();

      const uint8_t * sentinel = ptr + src.size();

      while(ptr < sentinel) {
        uint8_t c0 = *ptr++;

    int left = sentinel - ptr;

        characters++;
        if((c0 & 0x80) == 0)
          ;
        else if((c0 & 0xE0) == 0xC0) {
      if(left < 1) {
        error("utf8DecodedLength # Lacking 1 byte from a UTF8 string");
        return characters;
      }

          ptr++;
        }
        else if((c0 & 0xF0) == 0xE0) {
      if(left < 2) {
        error("utf8DecodedLength # Lacking 2 bytes from a UTF8 string");
        return characters;
      }

          ptr += 2;
        }
        else if((c0 & 0xF8) == 0xF0) {
      if(left < 3) {
        error("utf8DecodedLength # Lacking 3 bytes from a UTF8 string");
        return characters;
      }

          ptr += 3;
        }
      }

      return characters;
    }

    int utf8EncodedLength(const wstring & src)
    {
      int bytes = 0;

      size_t n = src.size();

      for(unsigned i = 0; i < n; i++) {
        unsigned c = src[0];

        if(c <= 0x7f)
          bytes++;
        else if(c <= 0x7FF)
          bytes += 2;
        else {
          if((c >= 0x800 && c <= 0xD7FF) ||
              (c >= 0xE000 && c <= 0xFFFF))
            bytes += 3;
          else if(c >= 0x10000 && c <= 0x10FFFF)
            bytes += 4;
        }
      }

      return bytes;
    }

    std::string lowerCase(const std::string & src)
    {
        std::string res(src);

        for(unsigned i = 0; i < res.size(); i++) {
            int c = res[i];

            if(c >= 'A' && c <= 'Z')
                res[i] = c + ('a' - 'A');
        }

        return res;
    }

    char upperCaseASCII(char c)
    {
      if(c < 'a' || c > 'z')
    return c;

      return c + ('A' - 'a');
    }

    int which(const char ** strings, const char * str)
    {
      const char * tmp;
      for(int i = 0; (tmp = strings[i]) != 0; i++) {
    if(strcmp(tmp, str) == 0)
      return i;
      }

      return -1;
    }

    int which(const StringList & strings, const std::string & str)
    {
      int index = 0;

      for(StringList::const_iterator it = strings.begin();
      it != strings.end(); it++) {
    if((*it) == str)
      return index;

    index++;
      }

      return -1;
    }

    const char * yesNo(bool yes)
    {
      return yes ? "yes" : "no";
    }

#ifdef WIN32
    string getLastErrorMessage()
    {
      const int   errStrSize = 1024;
      char  szErrStr[errStrSize] = "";
      FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0,
        GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), szErrStr, errStrSize, 0);

      return string(szErrStr);
    }
#endif

  }

}

