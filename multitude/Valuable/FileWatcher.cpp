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
#include <QFileSystemWatcher>
#include <QMap>
#include <QSet>
#include <QDir>
#include <QTimer>

namespace {

  // Default for merging change events (milliseconds)
  const int CHANGE_EVENT_DELAY = 300;

  struct ChangeEvent
  {
    enum Type {
        CREATE = (1 << 0)
      , MODIFY = (1 << 1)
      , DELETE = (1 << 2)
    };

    ChangeEvent()
      : m_type(MODIFY)
      , m_scheduled(Radiant::TimeStamp::currentTime() + Radiant::TimeStamp::createMilliSeconds(CHANGE_EVENT_DELAY))
    {}

    QByteArray typeAsString() const
    {
      if(m_type == CREATE)
        return "file-created";
      else if(m_type == MODIFY)
        return "file-changed";
      else
        return "file-removed";
    }

    Type m_type;
    Radiant::TimeStamp m_scheduled;
  };

}

namespace Valuable
{

  class FileWatcher::D : public QObject
  {
    Q_OBJECT

  public:
    FileWatcher & m_host;
    QFileSystemWatcher m_watcher;

    QMap<QString, QSet<QString> > m_directoryFiles;

    QMap<QString, ChangeEvent> m_delayedEvents;

    QTimer m_delayTimer;

    D(FileWatcher & host)
      : m_host(host)
    {
      connect(&m_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(directoryChanged(QString)));
      connect(&m_watcher, SIGNAL(fileChanged(QString)), this, SLOT(fileChanged(QString)));
    }

    void getChanges(const QString & path, QSet<QString> & added, QSet<QString> & removed)
    {
      assert(QFileInfo(path).isAbsolute());

      auto current = cachePathFiles(path);

      auto & cache = m_directoryFiles[path];

      removed = cache - current;
      added = current - (cache & current);

      cache = current;
    }

    QSet<QString> cachePathFiles(const QString & path)
    {
      assert(QFileInfo(path).isAbsolute());
      assert(QFileInfo(path).isDir());

      QSet<QString> files;

      QDir dir(path);
      dir.setFilter(QDir::Files);

      foreach(QFileInfo fi, dir.entryInfoList())
        files.insert(fi.absoluteFilePath());

      return files;
    }

    bool isDelayed(const QString & path) const
    {
      return m_delayedEvents.contains(path);
    }

    void delayEvent(const QString & path, ChangeEvent::Type type)
    {
      assert(QFileInfo(path).isAbsolute());

      // Update the event
      ChangeEvent & e = m_delayedEvents[path];

      e.m_type = type;
      e.m_scheduled = Radiant::TimeStamp::currentTime() + Radiant::TimeStamp::createMilliSeconds(CHANGE_EVENT_DELAY);

      // Reschedule the timer if it is not already running
      if(!m_delayTimer.isActive())
        m_delayTimer.singleShot(CHANGE_EVENT_DELAY, this, SLOT(sendDelayedEvents()));
    }

  private slots:

    void directoryChanged(const QString & relativePath)
    {
      const QString path = QFileInfo(relativePath).absoluteFilePath();

      QSet<QString> add, rm;

      getChanges(path, add, rm);

      // File creation events are never delayed
      foreach(QString filename, add) {

        if(isDelayed(filename)) {
          // There is a delayed event already, merge the events to single changed event
          delayEvent(filename, ChangeEvent::MODIFY);
        } else {
          m_host.eventSend("file-created", filename);
        }
      }

      // Always delay removal (we might be able to merge events)
      foreach(QString filename, rm) {
        delayEvent(filename, ChangeEvent::DELETE);
      }
    }

    void fileChanged(const QString & relativePath)
    {
      const QString path = QFileInfo(relativePath).absoluteFilePath();

      // We can't assume that directory changed preceeds file changed event, so
      // we must always delay file changed events
      delayEvent(path, ChangeEvent::MODIFY);
    }

    void sendDelayedEvents()
    {
      Radiant::TimeStamp min(std::numeric_limits<Radiant::TimeStamp::type>::max());
      Radiant::TimeStamp now = Radiant::TimeStamp::currentTime();

      for(auto i = m_delayedEvents.begin(); i != m_delayedEvents.end();) {

        const QString & path = i.key();
        const ChangeEvent & e = i.value();

        if(e.m_scheduled <= now) {

          m_host.eventSend(e.typeAsString(), path);
          i = m_delayedEvents.erase(i);

        } else {
          min = std::min(min, e.m_scheduled);
          ++i;
        }
      }

      // Rescedule to the next event
      if(!m_delayedEvents.isEmpty()) {
        int millis = (min - now).milliseconds();
        assert(millis >= 0);
        m_delayTimer.singleShot(millis, this, SLOT(sendDelayedEvents()));
      }
    }

  };

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  FileWatcher::FileWatcher()
    : m_d(new D(*this))
  {
    eventAddOut("file-created");
    eventAddOut("file-changed");
    eventAddOut("file-removed");
  }

  FileWatcher::~FileWatcher()
  {
    delete m_d;
  }

  void FileWatcher::addPath(const QString &path)
  {
    QFileInfo fi(path);
    const QString absoluteFilePath = fi.absoluteFilePath();
    const QString absolutePath = fi.absolutePath();

    // Initialize file cache if needed
    if(!m_d->m_directoryFiles.contains(absolutePath)) {
      auto & cache = m_d->m_directoryFiles[absolutePath];
      cache = m_d->cachePathFiles(absolutePath);
    }

    m_d->m_watcher.addPath(absoluteFilePath);

    // If the path is a file, monitor it's directory as well
    if(!fi.isDir()) {

      /// @todo keep track of dependencies so these automatic paths can be removed
      // Don't add paths multiple times to avoid errors from QFileSystemWatcher
      if(!m_d->m_watcher.directories().contains(fi.absolutePath()))
        m_d->m_watcher.addPath(fi.absolutePath());
    }

  }

  void FileWatcher::addPaths(const QStringList &paths)
  {
    foreach(QString path, paths)
      addPath(path);
  }

  QStringList FileWatcher::directories() const
  {
    return m_d->m_watcher.directories();
  }

  QStringList FileWatcher::files() const
  {
    return m_d->m_watcher.files();
  }

  void FileWatcher::removePath(const QString &path)
  {
    return m_d->m_watcher.removePath(path);
  }

  void FileWatcher::removePaths(const QStringList &paths)
  {
    return m_d->m_watcher.removePaths(paths);
  }

}

#include "moc_FileWatcher.cpp"
