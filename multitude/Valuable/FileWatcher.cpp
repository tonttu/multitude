/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
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
  const int CHANGE_EVENT_DELAY = 1000;

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

    QMap<QString, int> m_directoryRefCounts;

    QSet<QString> m_userAddedFiles;
    QSet<QString> m_userAddedDirectories;

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
      dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

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

          // This situation corresponds to the case when we are listening to some file
          // that is created after it is registered to watcher
          if(m_userAddedFiles.contains(filename) &&
             !m_watcher.files().contains(filename)) {
            m_watcher.addPath(filename);
          }

          if (m_userAddedFiles.contains(filename) ||
              m_userAddedDirectories.contains(QFileInfo(filename).absolutePath())) {
            m_host.eventSend("file-created", filename);
          }
        }
      }

      // Always delay removal (we might be able to merge events)
      foreach(QString filename, rm) {
        if (m_userAddedFiles.contains(filename) ||
            m_userAddedDirectories.contains(QFileInfo(filename).absolutePath())) {
          delayEvent(filename, ChangeEvent::DELETE);
        }
      }
    }

    void fileChanged(const QString & relativePath)
    {
      const QString path = QFileInfo(relativePath).absoluteFilePath();
      m_watcher.removePath(path);

      // If the file was removed, don't re-add the path to avoid warnings from
      // QFileSystemWatcher
      if(QFileInfo(path).exists()) {
        m_watcher.addPath(path);

        // We can't assume that directory changed preceeds file changed event, so
        // we must always delay file changed events
        delayEvent(path, ChangeEvent::MODIFY);
      }
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
    // Files and directories behave differently
    const QString absolutePath = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();

    assert(QFileInfo(absolutePath).isDir());

    // Avoid adding the same file multiple times to suppress warnings from
    // QFileSystemWatcher
    if(fi.isDir() && directories().contains(absoluteFilePath))
      return;
    else if(files().contains(absoluteFilePath))
      return;

    // Initialize file cache if needed
    if(!m_d->m_directoryFiles.contains(absolutePath)) {
      auto & cache = m_d->m_directoryFiles[absolutePath];
      cache = m_d->cachePathFiles(absolutePath);
    }

    // Only add to QFileSystemWatcher if the file exists to avoid errors
    if(fi.exists())
      m_d->m_watcher.addPath(absoluteFilePath);

    // If the path is a file, monitor it's directory as well
    if(!fi.isDir()) {

      // Don't add paths multiple times to avoid errors from QFileSystemWatcher
      if(!m_d->m_watcher.files().contains(absolutePath))
        m_d->m_watcher.addPath(absolutePath);

      m_d->m_userAddedFiles.insert(absoluteFilePath);
    } else
      m_d->m_userAddedDirectories.insert(absoluteFilePath);

    // Increase reference count for the path
    m_d->m_directoryRefCounts[absolutePath]++;
  }

  void FileWatcher::addPaths(const QStringList &paths)
  {
    foreach(QString path, paths)
      addPath(path);
  }

  QStringList FileWatcher::allWatchedDirectories() const
  {
    return m_d->m_watcher.directories();
  }

  QStringList FileWatcher::allWatchedFiles() const
  {
    return m_d->m_watcher.files();
  }

  QStringList FileWatcher::files() const
  {
    return m_d->m_userAddedFiles.toList();
  }

  QStringList FileWatcher::directories() const
  {
    return m_d->m_userAddedDirectories.toList();
  }

  void FileWatcher::removePath(const QString &path)
  {
    QFileInfo fi(path);

    const QString absoluteFilePath = fi.absoluteFilePath();
    // Files and directories behave differently
    const QString absolutePath = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();

    if (m_d->m_directoryRefCounts.contains(absolutePath)) {

      // Decrement reference count
      int & refs = m_d->m_directoryRefCounts[absolutePath];
      assert(refs > 0);
      --refs;

      // Remove the path if ref count is zero. If the argument is a directory,
      // this will remove it
      if(refs == 0) {
        m_d->m_watcher.removePath(absolutePath);
        int removeCount = m_d->m_directoryRefCounts.remove(absolutePath);
        (void)removeCount;
        assert(removeCount == 1);
      }
    }

    if(!fi.isDir()) {
      // If the argument is not a directory, the above check will not remove it
      m_d->m_watcher.removePath(absoluteFilePath);

      m_d->m_userAddedFiles.remove(absoluteFilePath);
    } else
      m_d->m_userAddedDirectories.remove(absoluteFilePath);
  }

  void FileWatcher::removePaths(const QStringList &paths)
  {
    foreach(QString path, paths)
      removePath(path);
  }

  void FileWatcher::clear()
  {
    QStringList paths = m_d->m_watcher.files() + m_d->m_watcher.directories();
    if (!paths.isEmpty())
      m_d->m_watcher.removePaths(paths);
    m_d->m_directoryFiles.clear();
    m_d->m_delayedEvents.clear();
  }
}

#include "FileWatcher.moc"
