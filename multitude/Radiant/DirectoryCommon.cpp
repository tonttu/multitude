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

#include "Directory.hpp"

#include "FileUtils.hpp"
#include "StringUtils.hpp"
#include "Trace.hpp"

#include <cassert>

#include <algorithm>

#include <QDir>

namespace Radiant
{
  Directory::Directory(const char * pathname,
                       int filters, SortFlag sortFlag)
    : m_path(pathname),
      m_filterFlags(filters),
      m_sortFlags(sortFlag)
  {
    populate();
  }

  Directory::Directory(const QString & pathname,
                       int filters, SortFlag sortFlag)
    : m_path(pathname),
      m_filterFlags(filters),
      m_sortFlags(sortFlag)
  {
    populate();
  }


  Directory::Directory(const char * pathname, const char * suffixlist,
                       int filters, SortFlag sortFlag)
  : m_path(pathname),
    m_filterFlags(filters),
    m_sortFlags(sortFlag)
  {
    m_suffixes = QString::fromUtf8(suffixlist).split(",", QString::SkipEmptyParts);
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

  void Directory::init(const QString & pathname, const char * suffixlist,
                       const int filters, const SortFlag sortFlag) 
  {
    m_path = pathname ;
    m_filterFlags = filters ;
    m_sortFlags = sortFlag ;
    m_suffixes = QString::fromUtf8(suffixlist).split(",", QString::SkipEmptyParts);
  }

  bool Directory::mkdirRecursive(const QString & dirname)
  {
    return QDir().mkpath(dirname);
  }
}
