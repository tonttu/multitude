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
    QDir::SortFlags sf = (m_sortFlag == Name) ? QDir::Name : QDir::Unsorted;
    QDir::Filters ff = 0;

    if(m_filterFlags & Dirs) ff |= QDir::Dirs;
    if(m_filterFlags & Files) ff |= QDir::Files;
    if(m_filterFlags & NoDotAndDotDot) ff |= QDir::NoDotAndDotDot;
    if(m_filterFlags & Hidden) ff |= QDir::Hidden;
    if(m_filterFlags & System) ff |= QDir::System;
 
    QDir dir(m_path, "", sf, ff);

    // Add entries
    QStringList list = dir.entryList();
    for(int i = 0; i < list.size(); i++) {
      QString entry = list.at(i);
      if(m_suffixes.isEmpty() || m_suffixes.contains(FileUtils::suffixLowerCase(entry)))
        m_entries.push_back(entry.toAscii().data());
    }
  }

}
