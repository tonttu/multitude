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

#include <assert.h>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

#define DOT     QString(".")
#define DOTDOT  QString("..")

namespace Radiant
{

  ///  @todo DT_DIR seems to incorrectly match also to symbolic links under linux
  bool applyFilters(const struct dirent * dent, int filterFlags,
                    const QStringList & suffixes) {
    bool ok = true;

    const QString name(dent->d_name);

    if(dent->d_type == DT_DIR && !(filterFlags & Directory::Dirs)) ok = false;
    else if(dent->d_type == DT_REG && !(filterFlags & Directory::Files)) ok = false;
    else if( (name == DOT || name == DOTDOT) && (filterFlags & Directory::NoDotAndDotDot))
      ok = false;
    else if( (name[0] == '.' && (name != DOT && name != DOTDOT))
        && !(filterFlags & Directory::Hidden) )
      ok = false;

    if(!suffixes.isEmpty()) {
      QString suffix = FileUtils::suffixLowerCase(name);

      ok = suffixes.contains(suffix);
    }

//    trace("Directory::applyFilters # DIR: %s FILE: %s MATCH: %d", m_path.c_str(), dent->d_name, ok);

    return ok;
  }

  bool Directory::mkdir(const char * dirname)
  {
#ifndef WIN32
    return (::mkdir(dirname, S_IRWXU) == 0);
#else
    return (::mkdir(dirname) == 0);
#endif
  }

  bool Directory::mkdir(const QString & dirname)
  {
    return mkdir(dirname.toUtf8().data());
  }

  bool Directory::exists(const QString & dir)
  {
    DIR * d = opendir(dir.toUtf8().data());
    if(!d)
      return false;

    closedir(d);
    return true;
  }

  void Directory::populate()
  {
    // Try to open the directory
    DIR * dir = opendir(m_path.toUtf8().data());
    if(!dir) {
      error("Directory::openDir # failed to open '%s'", m_path.toUtf8().data());
      return;
    }

    // Iterate through entries
    struct dirent * dent;
    while( (dent = readdir(dir)) != NULL) {
      // Apply filters
      if(applyFilters(dent, m_filterFlags, m_suffixes)) {
        m_entries.push_back(QString(dent->d_name));
      }
    }

    // Sort
    if(m_sortFlags == Name) {
      std::sort(m_entries.begin(), m_entries.end());
    }

    closedir(dir);
  }

}
