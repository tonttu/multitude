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

#include <QFile>
#include <QStringList>
#include <QTextStream>

namespace Radiant {

  CSVDocument::CSVDocument()
  {
  }

  CSVDocument::~CSVDocument()
  {
  }

  int CSVDocument::loadFromString(const QString &csv, const char *delimiter, bool removeQuotations)
  {
    m_rows.clear();

    if(csv.isEmpty()) {
      error("CSVParser::loadFromString # Empty contents");
      return -1;
    }

    QString delim2 = QString::fromUtf8(delimiter);

    Q_FOREACH(QString line, csv.split("\n")) {
      Row r;
      Q_FOREACH(QString str, line.split(delim2)) {

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

  int CSVDocument::load(const QString &filename, const char * delimiter, bool removeQuotations)
  {
    QString contents = Radiant::FileUtils::loadTextFile(filename);
    return loadFromString(contents, delimiter, removeQuotations);
  }

  bool CSVDocument::save(const QString &filename, const char *delimiter, bool useQuotations)
  {
    QFile file(filename);
    if(!file.open(QIODevice::WriteOnly))
      return false;

    QTextStream stream( & file);

    for(const Row & row : m_rows) {

      for(size_t i = 0; i < row.size(); i++) {
        if(useQuotations)
          stream << "\"";
        stream << row[i];
        if(useQuotations)
          stream << "\"";
        if((i + 1) < row.size())
          stream << delimiter;
      }

      stream << "\n";
    }

    return true;
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
        return static_cast<int>(i);
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

  CSVDocument::Row *CSVDocument::appendRow()
  {
    m_rows.push_back(Row());
    auto it = m_rows.end();
    it--;
    return &*it;
  }

  void CSVDocument::clear()
  {
    m_rows.clear();
  }

}
