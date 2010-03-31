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

#include <algorithm>

namespace Radiant
{

  template<class T> class NotInList : public std::unary_function<T, bool>
  {
    typedef std::vector<std::string> Arg;
    const Arg & m_arg;

    public:
    NotInList(const Arg & arg) : m_arg(arg) {}

    bool operator() (const T & x) const {
      const T & suffix = FileUtils::suffix(x);

      for(Arg::const_iterator it = m_arg.begin(); it != m_arg.end(); it++) 
        if(*it == suffix) 
          return false;

      return true;
    }

  };

  bool Directory::mkdir(const char * dirname)
  {
    QDir dir;
    return dir.mkdir(dirname);
  }

  bool Directory::mkdir(const std::string & dirname)
  {
    return mkdir(dirname.c_str());
  }

  bool Directory::exists(const std::string & path)
  {
    QDir dir(path.c_str());
    return dir.exists();
  }

  void Directory::populate()
  {
    QDir::SortFlags sf = (m_sortFlags == Name) ? QDir::Name : QDir::Unsorted;
    QDir::Filters ff = 0;

    if(m_filterFlags & Dirs) ff |= QDir::Dirs;
    if(m_filterFlags & Files) ff |= QDir::Files;
    if(m_filterFlags & NoDotAndDotDot) ff |= QDir::NoDotAndDotDot;
    if(m_filterFlags & Hidden) ff |= QDir::Hidden;
    if(m_filterFlags & AllEntries) ff |= QDir::AllEntries;

    QDir dir(m_path.c_str(), "", sf, ff);

    // Add entries
    QStringList list = dir.entryList();
    for(int i = 0; i < list.size(); i++) {
      QString entry = list.at(i);

      m_entries.push_back(entry.toAscii().data());
    }

    // Apply suffix filtering    
    if(!m_suffixes.empty()) {
      std::vector<std::string>::iterator from = std::remove_if(m_entries.begin(), m_entries.end(), NotInList<std::string> (m_suffixes));
      m_entries.erase(from, m_entries.end());
    }
  }

}
