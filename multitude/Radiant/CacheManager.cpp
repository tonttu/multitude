#include "BGThread.hpp"
#include "CacheManager.hpp"
#include "PlatformUtils.hpp"
#include "Sleep.hpp"
#include "Task.hpp"
#include "Trace.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QReadWriteLock>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

#include <set>
#include <string_view>
#include <unordered_set>

namespace std
{
  template <>
  struct hash<QByteArray>
  {
    size_t operator()(const QByteArray & x) const
    {
      return std::hash<std::string_view>()(std::string_view(x.data(), x.size()));
    }
  };
}

namespace
{
  thread_local int t_id{0};
  int s_id{0};

  struct CachedSource
  {
    QByteArray source;
    int64_t sourceTimestamp = 0;
    QByteArray sourceHexHash;
  };

  /// See https://sqlite.org/rescode.html
  const char * s_sqliteBusyErrorCodes[] = {
    "5",   // SQLITE_BUSY
    "6",   // SQLITE_LOCKED
    "261", // SQLITE_BUSY_RECOVERY
    "262", // SQLITE_LOCKED_SHAREDCACHE
    "517", // SQLITE_BUSY_SNAPSHOT
    "518", // SQLITE_LOCKED_VTAB
  };

  /// Is the error something that can be retried
  bool isBusy(const QSqlError & error)
  {
    QByteArray code = error.nativeErrorCode().toUtf8();
    for (const char * test: s_sqliteBusyErrorCodes)
      if (code == test)
        return true;
    return false;
  }

  template <typename ...Args>
  bool execQuery(QSqlQuery & q, Args... args)
  {
    bool ok = q.exec(args...);
    if (ok || !isBusy(q.lastError()))
      return ok;

    /// Retry for max 60 seconds
    const double timeoutS = 60.0;
    Radiant::Timer t;
    for (int i = 0;; ++i) {
      Radiant::Sleep::sleepMs(i);
      ok = q.exec(args...);
      if (ok || !isBusy(q.lastError()) || t.time() >= timeoutS)
        return ok;
    }

    return false;
  }

  QSqlQuery execOrThrow(QSqlDatabase & db, const QString & sql)
  {
    QSqlQuery q(db);
    if (!execQuery(q, sql))
      throw std::runtime_error(("SQL query '" + sql + "' failed in SQLite DB '" + db.databaseName()
                                + "': " + q.lastError().text()).toStdString());
    return q;
  }

  /// Note that this function is not thread-safe, but it's only called when
  /// initializing stuff or when syncing data to DB, never from two threads
  /// at the same time.
  int threadIndex()
  {
    if (t_id == 0)
      t_id = ++s_id;
    return t_id;
  }

  QString createDefaultCacheRoot()
  {
    QString basePath = Radiant::PlatformUtils::localAppPath();
    if (basePath.isEmpty())
      basePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    basePath += QString("/MultiTaction/cache");

    if (!QDir().mkpath(basePath) || !QFileInfo(basePath).isWritable()) {
      basePath = QString("%1/MultiTaction/cache").arg(QDir::tempPath());
      QDir().mkpath(basePath);
    }
    return basePath;
  }

  void rmRf(const QString & path, uint64_t & files, uint64_t & bytes)
  {
    QDir dir(path);

    for (const QFileInfo & fi: dir.entryInfoList(
           QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks)) {
      if (fi.isFile()) {
        int64_t tmp = fi.size();
        const QString f = fi.absoluteFilePath();
        if (dir.remove(fi.fileName())) {
          if (tmp > 0)
            bytes += tmp;
          ++files;
        }
      } else if (fi.isDir()) {
        rmRf(fi.absoluteFilePath(), files, bytes);
      }
    }
    QDir().rmdir(path);
  }

  void deleteObsoleteCacheDir(const QString & path)
  {
    if (path.isEmpty())
      return;

    QFileInfo fi(path);
    if (!fi.isAbsolute() || fi.isSymLink())
      return;

    uint64_t files = 0;
    uint64_t bytes = 0;
    rmRf(path, files, bytes);

    if (files > 0) {
      Radiant::info("Removed %lld obsolete cache files from %s [%.1f MB]",
                    static_cast<long long>(files), path.toUtf8().data(), bytes / 1024.0 / 1024.0);
    }
  }

  void deleteObsoleteCaches()
  {
    /// PDF cache used in Canvus 1.7.3 and older
    const QString pdfCache1 = Radiant::PlatformUtils::getModuleUserDataPath("CornerstonePDFPageCache", false);
    deleteObsoleteCacheDir(pdfCache1);

    /// PDF cache used in Cornerstone 2.7.x and older
    const QString localAppPath = Radiant::PlatformUtils::localAppPath();
    if (!localAppPath.isEmpty()) {
      const QString pdfCache2 = localAppPath + "/MultiTaction/cornerstone/cache/pdfs";
      deleteObsoleteCacheDir(pdfCache2);
    }

    const QString multiTouchDir = Radiant::PlatformUtils::getModuleUserDataPath("MultiTouch", false);
    if (!multiTouchDir.isEmpty()) {
      /// Video preview cache used in Cornerstone 2.7.x and older
      const QString previewCache = multiTouchDir + "/previewcache";
      deleteObsoleteCacheDir(previewCache);

      /// Mipmap cache used in Cornerstone 2.0.3 and older
      const QString mipmapCache1 = multiTouchDir + "/imagecache";
      deleteObsoleteCacheDir(mipmapCache1);

      /// Mipmap cache used in Cornerstone 2.0.4 .. 2.7.5
      const QString mipmapCache2 = multiTouchDir + "/imagecache-1";
      deleteObsoleteCacheDir(mipmapCache2);
    }
  }
}

namespace Radiant
{
  class CacheManager::D
  {
  public:
    QSqlDatabase openDb();

    void cleanDbConnections();
    void initializeDb();
    void sync();
    void save();
    /// @param deletedFiles absolute paths to deleted cache files
    /// @param deletedItems deleted source files that should be deleted from the DB
    void deleteFiles(const QStringList & cacheDirs,
                     const std::vector<CachedSource> & sources,
                     QByteArrayList & deletedFiles,
                     bool onlyRemoveInvalidItems,
                     QByteArrayList & deletedItems);

  public:
    QString m_root;

    /// Protectes m_cacheItems, m_added and m_removed
    QReadWriteLock m_itemLock;
    std::set<QByteArray> m_cacheItems;
    std::unordered_set<QByteArray> m_added;
    std::unordered_set<QByteArray> m_removed;

    Radiant::Mutex m_saveTaskLock;
    std::shared_ptr<Radiant::FunctionTask> m_saveTask;
  };

  /////////////////////////////////////////////////////////////////////////////

  QSqlDatabase CacheManager::D::openDb()
  {
    QString name = QString("CacheManager-%1").arg(threadIndex());
    if (!QSqlDatabase::contains(name)) {
      QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", name);
      db.setDatabaseName(m_root + "/cache.db");

      if (!db.open())
        throw std::runtime_error(("Failed to open SQLite DB '" + db.databaseName()
                                  + "': " + db.lastError().text()).toStdString());
      return db;
    }

    return QSqlDatabase::database(name);
  }

  void CacheManager::D::cleanDbConnections()
  {
    for (int i = 1; i <= s_id; ++i)
      QSqlDatabase::removeDatabase(QString("CacheManager-%1").arg(i));
  }

  void CacheManager::D::initializeDb()
  {
    {
      QWriteLocker g(&m_itemLock);
      m_cacheItems.clear();
    }
    QSqlDatabase db = openDb();

    /// Version of the database. If the DB is older than that, we should
    /// run migrations
    const int dbVersion = 1;
    /// Oldest version of the database code that we still support with this
    /// version of DB. If we do backwards-compatible changes, like add a new
    /// optional column, we can only increase dbVersion but keep dbCompatVersion
    /// the same, so that older versions can still continue to use the same DB.
    const int dbCompatVersion = 0;

    execOrThrow(db,
                R"(CREATE TABLE IF NOT EXISTS db (
                db_version INTEGER NOT NULL,
                db_compat_version INTEGER NOT NULL))");

    int currentDbVersion = 0;
    QSqlQuery q = execOrThrow(db, "SELECT db_version, db_compat_version FROM db");
    if (q.next()) {
      currentDbVersion = q.value(0).toInt();
      const int currentCompatVersion = q.value(1).toInt();
      if (dbVersion < currentCompatVersion)
        throw std::runtime_error(QString("Cache DB '%3' is too new version %1, we only support version %2")
                                 .arg(currentCompatVersion).arg(dbVersion).arg(db.databaseName()).toStdString());
    } else {
      execOrThrow(db, QString("INSERT INTO db (db_version, db_compat_version) VALUES (%1, %2)")
                  .arg(0).arg(dbCompatVersion));
    }

    execOrThrow(db,
                R"(CREATE TABLE IF NOT EXISTS cache_items (
                source TEXT PRIMARY KEY NOT NULL
                ))");

    if (currentDbVersion < 1) {
      // Migration 1 is to delete all old caches, see Q9 in
      // Canvus "Cache Management Specification" document
      deleteObsoleteCaches();

      execOrThrow(db, "UPDATE db SET db_version = 1");
    }

    q = execOrThrow(db, "SELECT source FROM cache_items");

    QWriteLocker g(&m_itemLock);
    while (q.next())
      m_cacheItems.insert(q.value(0).toString().toUtf8());
  }

  void CacheManager::D::sync()
  {
    Radiant::Guard g(m_saveTaskLock);
    if (!m_saveTask) {
      // m_saveTask is deleted in ~CacheManager, so it is safe to capture `this`
      m_saveTask = std::make_shared<Radiant::FunctionTask>([this] (Radiant::Task & task) {
        save();

        {
          QWriteLocker g(&m_itemLock);
          if (!m_added.empty() || !m_removed.empty())
            return;

          Radiant::Guard g2(m_saveTaskLock);
          m_saveTask.reset();
        }
        task.setFinished();
      });
      Radiant::BGThread::instance()->addTask(m_saveTask);
    }
  }

  void CacheManager::D::save()
  {
    QSqlDatabase db = openDb();
    std::unordered_set<QByteArray> added, removed;
    {
      QWriteLocker g(&m_itemLock);
      std::swap(added, m_added);
      std::swap(removed, m_removed);
    }
    if (!removed.empty()) {
      QSqlQuery query(db);
      bool ok = query.prepare("DELETE FROM cache_items WHERE source = ?");
      if (ok) {
        for (const QByteArray & src: removed) {
          /// Need to use QString instead of QByteArray to encode the data as TEXT and not BLOB
          query.bindValue(0, QString::fromUtf8(src));
          ok = execQuery(query);
          if (!ok)
            Radiant::error("Failed to execute %s: %s", query.lastQuery().toUtf8().data(),
                           query.lastError().text().toUtf8().data());
        }
      } else {
        Radiant::error("Failed to execute %s: %s", query.lastQuery().toUtf8().data(),
                       query.lastError().text().toUtf8().data());
      }
    }

    if (!added.empty()) {
      QSqlQuery query(db);
      bool ok = query.prepare("INSERT OR REPLACE INTO cache_items (source) VALUES (?)");
      if (ok) {
        for (const QByteArray & src: added) {
          query.bindValue(0, QString::fromUtf8(src));
          ok = execQuery(query);
          if (!ok)
            Radiant::error("Failed to execute %s: %s", query.lastQuery().toUtf8().data(),
                           query.lastError().text().toUtf8().data());
        }
      } else {
        Radiant::error("Failed to execute %s: %s", query.lastQuery().toUtf8().data(),
                       query.lastError().text().toUtf8().data());
      }
    }
  }

  void CacheManager::D::deleteFiles(const QStringList & cacheDirs,
                                    const std::vector<CachedSource> & sources,
                                    QByteArrayList & deletedFiles,
                                    bool onlyRemoveInvalidItems,
                                    QByteArrayList & deletedItems)
  {
    /// If onlyRemoveInvalidItems is true, we check the source and cache
    /// timestamp and only remove the cache if the source is missing or
    /// if the source timestamp is newer than the cache
    for (const CachedSource & c: sources) {
      bool sourceHasValidCacheItem = false;
      for (const QString & cacheDir: cacheDirs) {
        const QString prefix = c.sourceHexHash.left(2);
        QDir deleteGlob(cacheDir + "/" + prefix, c.sourceHexHash + "*", QDir::NoSort, QDir::Files | QDir::Dirs | QDir::NoSymLinks);

        if (c.sourceHexHash.size() != 40 || prefix.size() != 2) {
          Radiant::error("CacheManager # Failed to generate SHA1 hash");
          return;
        }

        for (const QFileInfo & cacheEntry: deleteGlob.entryInfoList()) {
          if (cacheEntry.isDir()) {
            if (onlyRemoveInvalidItems && c.sourceTimestamp > 0) {
              QDirIterator dirIt(cacheEntry.absoluteFilePath(),
                                 QDir::Files | QDir::NoSymLinks,
                                 QDirIterator::Subdirectories);
              bool hasValidFiles = false;
              while (dirIt.hasNext()) {
                QString cacheFile = dirIt.next();
                if (QFileInfo(cacheFile).lastModified().toMSecsSinceEpoch() >= c.sourceTimestamp) {
                  hasValidFiles = true;
                  continue;
                }

                deletedFiles << cacheFile.toUtf8();
                QFile::remove(cacheFile);
              }

              if (hasValidFiles) {
                sourceHasValidCacheItem = true;
              } else {
                QDir(cacheEntry.absoluteFilePath()).removeRecursively();
              }
            } else {
              QDirIterator dirIt(cacheEntry.absoluteFilePath(),
                                 QDir::Files | QDir::NoSymLinks,
                                 QDirIterator::Subdirectories);
              while (dirIt.hasNext())
                deletedFiles << dirIt.next().toUtf8();
              QDir(cacheEntry.absoluteFilePath()).removeRecursively();
            }
          } else {
            if (onlyRemoveInvalidItems && c.sourceTimestamp > 0 &&
                cacheEntry.lastModified().toMSecsSinceEpoch() >= c.sourceTimestamp) {
              sourceHasValidCacheItem = true;
              continue;
            }
            deletedFiles << cacheEntry.absoluteFilePath().toUtf8();
            QFile::remove(cacheEntry.absoluteFilePath());
          }
        }
      }

      if (!sourceHasValidCacheItem)
        deletedItems << c.source;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  CacheManager::CacheManager()
    : m_d(new D())
  {
    m_d->m_root = createDefaultCacheRoot();
    m_d->initializeDb();
  }

  CacheManager::~CacheManager()
  {
    std::shared_ptr<Radiant::FunctionTask> saveTask;
    {
      Radiant::Guard g(m_d->m_saveTaskLock);
      saveTask = std::move(m_d->m_saveTask);
    }
    if (saveTask)
      saveTask->runNow(true);

    m_d->cleanDbConnections();
  }

  QString CacheManager::cacheRoot() const
  {
    return m_d->m_root;
  }

  void CacheManager::setCacheRoot(const QString & cacheRoot)
  {
    if (cacheRoot == this->cacheRoot())
      return;

    m_d->cleanDbConnections();
    if (cacheRoot.isEmpty())
      m_d->m_root = createDefaultCacheRoot();
    else
      m_d->m_root = cacheRoot;
    m_d->initializeDb();
  }

  QString CacheManager::createCacheDir(const QString & component)
  {
    QString dir(QString("%1/%2").arg(cacheRoot(), component));
    QDir().mkdir(dir);
    return dir;
  }

  CacheManager::CacheItem CacheManager::cacheItem(
      const QString & cacheDir, const QString & source,
      const QString & options, const QString & suffix,
      CacheManager::CreateFlags flags)
  {
    // Compute a hash from the original source. It might not be a file,
    // so we don't try to resolve it to an absolute path. Do not include
    // timestamp or other information to this hash so that we can easily
    // remove items from the cache. SHA1 seems to be the fastest somewhat
    // reliable hash so it works here well.
    //
    // We will also compare the source file and cache file timestamps and fill
    // CacheItem::isValid field based on that.
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(source.toUtf8());

    CacheItem item;

    const QString hashTxt = hash.result().toHex();
    item.path = QString("%1/%2/%3").arg(cacheDir, hashTxt.left(2), hashTxt);
    if (!options.isEmpty())
      item.path += QString(".%1").arg(options);
    if (!suffix.isEmpty())
      item.path += QString(".%1").arg(suffix);

    if (flags & FLAG_CREATE_PATH)
      QDir().mkpath(QString("%1/%2").arg(cacheDir, hashTxt.left(2)));

    if (flags & FLAG_ADD_TO_DB) {
      QByteArray src = source.toUtf8();

      bool sync = false;
      {
        QReadLocker g(&m_d->m_itemLock);
        if (!m_d->m_cacheItems.count(src)) {
          g.unlock();
          QWriteLocker g2(&m_d->m_itemLock);
          m_d->m_cacheItems.insert(src);
          sync = m_d->m_added.insert(src).second;
        }
      }
      if (sync)
        m_d->sync();
    }

    QDateTime cacheModified = QFileInfo(item.path).lastModified();
    QDateTime srcModified = QFileInfo(source).lastModified();
    if (cacheModified.isValid())
      item.isValid = !srcModified.isValid() || cacheModified >= srcModified;

    return item;
  }

  QByteArrayList CacheManager::removeFromCache(const QString & sourcePrefix, bool onlyRemoveInvalidItems)
  {
    QCryptographicHash hash(QCryptographicHash::Sha1);
    std::vector<CachedSource> sources;
    QByteArrayList deletedItems;
    QByteArrayList deletedFiles;
    QByteArray prefix = sourcePrefix.toUtf8();

    if (m_d->m_root.isEmpty()) {
      Radiant::error("CacheManager::removeFromCache # Can't have empty cacheRoot / source");
      return deletedFiles;
    }

    QStringList cacheDirs;
    QDir root(m_d->m_root);
    for (QString component: root.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot))
      cacheDirs << QString("%1/%2").arg(m_d->m_root, component);

    QReadLocker g(&m_d->m_itemLock);

    auto it = m_d->m_cacheItems.lower_bound(prefix);

    /// If we call removeFromCache("/foo/img.png") and img.png had a mipmap cache
    /// from time before we had a cache database, we would still like to remove it.
    if ((it == m_d->m_cacheItems.end() || prefix != *it) && !sourcePrefix.isEmpty()) {
      CachedSource c;
      if (onlyRemoveInvalidItems) {
        QDateTime dt = QFileInfo(sourcePrefix).lastModified();
        if (dt.isValid())
          c.sourceTimestamp = dt.toMSecsSinceEpoch();
      }
      hash.addData(prefix);
      c.sourceHexHash = hash.result().toHex();
      c.source = prefix;
      sources.push_back(std::move(c));
    }

    while (it != m_d->m_cacheItems.end()) {
      if (!it->startsWith(prefix))
        break;

      CachedSource c;
      if (onlyRemoveInvalidItems) {
        c.source = *it;
        QDateTime dt = QFileInfo(QString::fromUtf8(*it)).lastModified();
        if (dt.isValid())
          c.sourceTimestamp = dt.toMSecsSinceEpoch();
      }

      hash.reset();
      hash.addData(*it);
      c.sourceHexHash = hash.result().toHex();

      sources.push_back(std::move(c));
      ++it;
    }

    m_d->deleteFiles(cacheDirs, sources, deletedFiles, onlyRemoveInvalidItems, deletedItems);

    /// It's possible to have cached files of other cache files. For instance
    /// a PDF file as a source can have the individual PDF pages as images in
    /// the cache, and those page images could have mipmaps. However, we assume
    /// that we don't have unlimited recursion but just maximum one level of
    /// nested caches. Therefore we don't call removeFromCache recursively,
    /// since it could be expensive, instead we do a second pass where we check
    /// if any of the deleted cache files had caches.

    sources.clear();
    for (int i = 0, count = deletedFiles.size(); i < count; ++i) {
      const QByteArray & filename = deletedFiles[i];

      it = m_d->m_cacheItems.find(filename);
      if (it == m_d->m_cacheItems.end())
        continue;

      deletedItems << filename;

      hash.reset();
      hash.addData(filename);

      CachedSource c;
      c.sourceHexHash = hash.result().toHex();
      sources.push_back(std::move(c));
    }

    m_d->deleteFiles(cacheDirs, sources, deletedFiles, false, deletedItems);

    g.unlock();

    if (!deletedItems.isEmpty()) {
      {
        QWriteLocker g2(&m_d->m_itemLock);
        for (const QByteArray & src: deletedItems) {
          m_d->m_removed.insert(src);
          auto it = m_d->m_cacheItems.find(src);
          if (it != m_d->m_cacheItems.end())
            m_d->m_cacheItems.erase(it);
        }
      }
      m_d->sync();
    }

    return deletedFiles;
  }

  QByteArrayList CacheManager::clearCacheDir(const QString & cacheDir)
  {
    QByteArrayList deleted;
    QFileInfo fi(cacheDir);
    if (!fi.isAbsolute() || cacheDir.isEmpty()) {
      Radiant::error("CacheManager::clearCacheDir # Invalid cache dir '%s'",
                     cacheDir.toUtf8().data());
      return deleted;
    }

    bool dbChanged = false;
    QWriteLocker g(&m_d->m_itemLock);

    QDir glob(cacheDir, "??", QDir::NoSort, QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for (const QFileInfo & entry: glob.entryInfoList()) {
      if (entry.fileName().size() != 2) {
        Radiant::error("CacheManager::clearCacheDir # Invalid filename");
        continue;
      }

      QDirIterator dirIt(entry.absoluteFilePath(), QDir::Files | QDir::NoSymLinks,
                         QDirIterator::Subdirectories);
      while (dirIt.hasNext()) {
        const QByteArray & filename = dirIt.next().toUtf8();
        deleted << filename;

        auto it = m_d->m_cacheItems.find(filename);
        if (it == m_d->m_cacheItems.end())
          continue;

        m_d->m_removed.insert(filename);
        m_d->m_cacheItems.erase(it);
        dbChanged = true;
      }
      QDir(entry.absoluteFilePath()).removeRecursively();
    }
    g.unlock();

    if (dbChanged)
      m_d->sync();

    return deleted;
  }

  DEFINE_SINGLETON(CacheManager)
}
