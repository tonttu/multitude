/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "CSVDocument.hpp"

#include "FileUtils.hpp"
#include "StringUtils.hpp"
#include "Trace.hpp"

#include <QStringList>

namespace Radiant {

  CSVDocument::CSVDocument()
  {
  }

  CSVDocument::~CSVDocument()
  {
  }

  int CSVDocument::load(const QString &filename, const char * delimiter, bool removeQuotations)
  {
    m_rows.clear();

    QString contents = Radiant::FileUtils::loadTextFile(filename);

    if(contents.isEmpty()) {
      error("CSVParser::load # Empty file %s", filename.toUtf8().data());
      return -1;
    }

    QString delim2 = QString::fromUtf8(delimiter);

    foreach(QString line, contents.split("\n")) {
      Row r;
      foreach(QString str, line.split(delim2)) {

        if(removeQuotations && str.size() >= 2) {
          if(str[0] == '\"')
            str.remove(0, 1);
          if(str[str.size()-1] == '\"')
            str.remove(str.size()-1, 1);
        }
        r.push_back(str.trimmed());
      }
      m_rows.push_back(r);
    }

    return (int) m_rows.size();
  }


  CSVDocument::Row * CSVDocument::findRow(const QString & key, unsigned col)
  {
    for(Rows::iterator it = m_rows.begin(); it != m_rows.end(); ++it) {
      Row & r = (*it);
      if(col < r.size()) {
        if(r[col] == key) {
          return & r;
        }
      }
    }

    return 0;
  }

  int CSVDocument::findColumnOnRow(const QString &key, unsigned rowIndex)
  {
    Row * r = row(rowIndex);

    if(!r)
      return -1;

    for(size_t i = 0; i < r->size(); i++) {

      if(r->at(i) == key)
        return i;
    }

    return -1;
  }

  CSVDocument::Row * CSVDocument::row(unsigned index)
  {
    if(index >= rowCount())
      return 0;

    unsigned n = 0;
    for(Rows::iterator it = begin(); it != end(); ++it, n++) {
      if(n == index)
        return & (*it);
    }

    return 0; // Should be unreachable
  }

}
