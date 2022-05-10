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

#include <Radiant/Mutex.hpp>
#include <QDir>
#include <QList>

#include <algorithm>

namespace Radiant
{

  bool Directory::mkdir(const QString & dirname)
  {
    return QDir().mkdir(dirname);
  }

  bool Directory::exists(const QString & path)
  {
    return QDir(path).exists();
  }

  void Directory::populate()
  {
    QDir::SortFlags sf = (m_sortFlag == NAME) ? QDir::Name : QDir::Unsorted;
    QDir::Filters ff = 0;

    if(m_filterFlags & DIRS) ff |= QDir::Dirs;
    if(m_filterFlags & FILES) ff |= QDir::Files;
    if(m_filterFlags & NO_DOT_AND_DOTDOT) ff |= QDir::NoDotAndDotDot;
    if(m_filterFlags & HIDDEN) ff |= QDir::Hidden;
    if(m_filterFlags & SYSTEM) ff |= QDir::System;
 
    QDir dir(m_path, "", sf, ff);

    // Add entries
    QStringList list = dir.entryList();
    for(int i = 0; i < list.size(); i++) {
      QString entry = list.at(i);
      if(m_suffixes.isEmpty() || m_suffixes.contains(FileUtils::suffixLowerCase(entry)))
        m_entries.push_back(entry);
    }
  }

}
