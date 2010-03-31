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

#include "CSVDocument.hpp"

#include "FileUtils.hpp"
#include "StringUtils.hpp"
#include "Trace.hpp"

namespace Radiant {

  CSVDocument::CSVDocument()
  {
  }

  CSVDocument::~CSVDocument()
  {
  }

  int CSVDocument::load(const char * filename, const char * delimiter)
  {
    m_rows.clear();

    std::wstring contents = Radiant::FileUtils::readTextFile(filename);

    if(contents.empty()) {
      error("CSVParser::load # Empty file %s", filename);
      return -1;
    }

    /*
    for(int i = 0; i < 200; i++) {
      info("Got %d %x %c", (int) contents[i], (int) contents[i], (char) contents[i]);
    }
    */
    std::wstring delim1; //(Radiant::StringUtils::utf8AsStdWstring("\n"));
    delim1 += (wchar_t) 8203;
    std::wstring delim2(Radiant::StringUtils::utf8AsStdWstring(delimiter));

    Radiant::StringUtils::WStringList strs;
    Radiant::StringUtils::split(contents, delim1, strs);

    info("Got %d lines", (int) strs.size());

    for(Radiant::StringUtils::WStringList::iterator it = strs.begin();
    it != strs.end(); it++) {

      Radiant::StringUtils::WStringList cells;
      Radiant::StringUtils::split((*it), delim2, cells);

      Row r;

      for(Radiant::StringUtils::WStringList::iterator it2 = cells.begin();
      it2 != cells.end(); it2++) {
        std::wstring & str = (*it2);
        // Strip leading white-space

        while(str.size() && (str[0] == ' ' || str[0] == '\"'))
          str.erase(0, 1);

        while(str.size() && ((str[str.size() - 1] == 58) ||
                             str[str.size() - 1] == 34 ||
                             str[str.size() - 1] == 8203))
          str.erase(str.size() - 1, 1);

        /*debug("CELL %d,%d = %s", (int) r.size(), (int) m_rows.size(),
                Radiant::StringUtils::stdWstringAsUtf8(str).c_str());
          */
        r.push_back(str);
      }

      m_rows.push_back(r);
    }

    return (int) m_rows.size();
  }


  CSVDocument::Row * CSVDocument::findRow(const std::wstring & key, unsigned col)
  {
    for(Rows::iterator it = m_rows.begin(); it != m_rows.end(); it++) {
      Row & r = (*it);
      if(col < r.size()) {
        if(r[col] == key) {
          return & r;
        }
      }
    }

    return 0;
  }

  CSVDocument::Row * CSVDocument::row(unsigned index)
  {
    if(index >= rowCount())
      return 0;

    unsigned n = 0;
    for(Rows::iterator it = begin(); it != end(); it++, n++) {
      if(n == index)
        return & (*it);
    }

    return 0; // Should be unreachable
  }

}
