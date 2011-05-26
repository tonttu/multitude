/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#include "FileWatcher.hpp"

#include <Radiant/FileUtils.hpp>

#include <QFileInfo>

namespace Valuable
{

FileWatcher::FileWatcher()
{
  eventAdd("changed");
}

FileWatcher & FileWatcher::instance()
{
  static FileWatcher s_watcher;
  return s_watcher;
}

void FileWatcher::add(QString filename)
{
/*  QFileInfo fi(filename);
  QString abs = fi.absoluteFilePath();
  if(!abs.isEmpty()) filename = abs;*/
  Radiant::info("Watching file %s", filename.toUtf8().data());
  m_files[filename] = Radiant::FileUtils::lastModified(filename);
}

void FileWatcher::update()
{
  if(!m_queue.isEmpty()) {
    Radiant::TimeStamp ts = Radiant::TimeStamp::getTime();
    Radiant::TimeStamp diff = Radiant::TimeStamp::createSecondsD(0.3);
    for(QSet<QString>::iterator it = m_queue.begin(); it != m_queue.end();) {
      QMap<QString, Radiant::TimeStamp>::iterator it2 = m_files.find(*it);
      if(it2 == m_files.end()) {
        it = m_queue.erase(it);
      } else if(*it2 + diff < ts) {
        Radiant::info("Changed %s", it->toUtf8().data());
        Radiant::BinaryData bd;
        bd.writeString(*it);
        eventSend("changed", bd);
        it = m_queue.erase(it);
      } else ++it;
    }
  }
  for(QMap<QString, Radiant::TimeStamp>::iterator it = m_files.begin(); it != m_files.end(); ++it) {
    Radiant::TimeStamp ts = Radiant::FileUtils::lastModified(it.key());
    if(ts > *it) {
      *it = ts;
      Radiant::info("Putting %s to queue", it.key().toUtf8().data());
      m_queue << it.key();
    }
  }
}

}
