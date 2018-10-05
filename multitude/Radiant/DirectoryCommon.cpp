/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Directory.hpp"

#include "FileUtils.hpp"
#include "StringUtils.hpp"
#include "Trace.hpp"
#include "Mime.hpp"

#include <cassert>

#include <algorithm>

#include <QDir>

namespace Radiant
{
  Directory::Directory(const QString & pathname,
                       int filters, SortFlag sortFlag)
    : m_path(pathname),
      m_filterFlags(filters),
      m_sortFlag(sortFlag)
  {
    populate();
  }


  Directory::Directory(const QString & pathname, const QString & suffixlist,
                       int filters, SortFlag sortFlag)
  : m_path(pathname),
    m_filterFlags(filters),
    m_sortFlag(sortFlag)
  {
    m_suffixes = suffixlist.toLower().split(",", QString::SkipEmptyParts);
    populate();
  }

  Directory::~Directory()
  {
  }

  int Directory::count() const
  {
    return (int)m_entries.size();
  }

  QString Directory::fileName(int i) const
  {
    assert(i >= 0 && i < count());
    return m_entries[i];
  }

  QString Directory::fileNameWithPath(int n) const
  {
    return path() + "/" + fileName(n);
  }

  bool Directory::mkdirRecursive(const QString & dirname)
  {
    return QDir().mkpath(dirname);
  }

  Directory Directory::findByMimePattern(const QString & pathname, const QString & mimePattern,
                                         int filters, SortFlag sortFlag)
  {
    MimeManager mime;
    QStringList tmp;
    Q_FOREACH(const QString & t, mimePattern.split('*'))
      tmp << QRegExp::escape(t);
    /// @todo implement real mime loading. it shouldn't have anything to do with file extensions
    return Directory(pathname, mime.extensionsByMimeRegexp(tmp.join(".*")).join(","),
                     filters, sortFlag);
  }

}
