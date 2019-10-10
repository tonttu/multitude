#include "BGThread.hpp"
#include "CacheManager.hpp"
#include "PlatformUtils.hpp"
#include "Task.hpp"
#include "Trace.hpp"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
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

  QSqlQuery execOrThrow(QSqlDatabase & db, const QString & sql)
  {
    QSqlQuery q(db);
    if (!q.exec(sql))
      throw std::runtime_error(("SQL query '" + sql + "' failed in SQLite DB '" + db.databaseName()
                                + "': " + q.lastError().text()).toStdString());
    return q;
  }

  /// Note that this function is not thread-safe, but it's only called when
  /// m_dbMutex is locked
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
    void deleteFiles(const QStringList & cacheDirs,
                     const std::vector<CachedSource> & sources,
                     QByteArrayList & deleted,
                     bool onlyRemoveInvalidItems,
                     bool & dbChanged);

  public:
    Radiant::Mutex m_dbMutex;
    QString m_root;
    std::set<QByteArray> m_cacheItems;

    std::shared_ptr<Radiant::SingleShotTask> m_saveTask;
    std::unordered_set<QByteArray> m_added;
    std::unordered_set<QByteArray> m_removed;
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
    m_cacheItems.clear();
    QSqlDatabase db = openDb();

    /// Version of the database. If the DB is older than that, we should
    /// run migrations
    const int dbVersion = 0;
    /// Oldest version of the database code that we still support with this
    /// version of DB. If we do backwards-compatible changes, like add a new
    /// optional column, we can only increase dbVersion but keep dbCompatVersion
    /// the same, so that older versions can still continue to use the same DB.
    const int dbCompatVersion = 0;

    execOrThrow(db,
                R"(CREATE TABLE IF NOT EXISTS db (
                db_version INTEGER NOT NULL,
                db_compat_version INTEGER NOT NULL))");

    QSqlQuery q = execOrThrow(db, "SELECT db_compat_version FROM db");
    if (q.next()) {
      int currentCompatVersion = q.value(0).toInt();
      if (dbVersion < currentCompatVersion)
        throw std::runtime_error(QString("Cache DB '%3' is too new version %1, we only support version %2")
                                 .arg(currentCompatVersion).arg(dbVersion).arg(db.databaseName()).toStdString());
    } else {
      execOrThrow(db, QString("INSERT INTO db (db_version, db_compat_version) VALUES (%1, %2)")
                  .arg(dbVersion).arg(dbCompatVersion));
    }

    execOrThrow(db,
                R"(CREATE TABLE IF NOT EXISTS cache_items (
                source TEXT PRIMARY KEY NOT NULL
                ))");

    q = execOrThrow(db, "SELECT source FROM cache_items");
    while (q.next())
      m_cacheItems.insert(q.value(0).toString().toUtf8());
  }

  void CacheManager::D::sync()
  {
    if (!m_saveTask) {
      // m_saveTask is deleted in ~CacheManager, so it is safe to capture `this`
      m_saveTask = std::make_shared<Radiant::SingleShotTask>([this] {
        Radiant::Guard g(m_dbMutex);
        save();
        m_saveTask.reset();
      });
      Radiant::BGThread::instance()->addTask(m_saveTask);
    }
  }

  void CacheManager::D::save()
  {
    QSqlDatabase db = openDb();
    if (!m_removed.empty()) {
      QSqlQuery query(db);
      bool ok = query.prepare("DELETE FROM cache_items WHERE source = ?");
      if (ok) {
        for (const QByteArray & src: m_removed) {
          /// Need to use QString instead of QByteArray to encode the data as TEXT and not BLOB
          query.bindValue(0, QString::fromUtf8(src));
          ok = query.exec();
          if (!ok)
            Radiant::error("Failed to execute %s: %s", query.lastQuery().toUtf8().data(),
                           query.lastError().text().toUtf8().data());
        }
      } else {
        Radiant::error("Failed to execute %s: %s", query.lastQuery().toUtf8().data(),
                       query.lastError().text().toUtf8().data());
      }
      m_removed.clear();
    }

    if (!m_added.empty()) {
      QSqlQuery query(db);
      bool ok = query.prepare("INSERT OR REPLACE INTO cache_items (source) VALUES (?)");
      if (ok) {
        for (const QByteArray & src: m_added) {
          query.bindValue(0, QString::fromUtf8(src));
          ok = query.exec();
          if (!ok)
            Radiant::error("Failed to execute %s: %s", query.lastQuery().toUtf8().data(),
                           query.lastError().text().toUtf8().data());
        }
      } else {
        Radiant::error("Failed to execute %s: %s", query.lastQuery().toUtf8().data(),
                       query.lastError().text().toUtf8().data());
      }
      m_added.clear();
    }
  }

  void CacheManager::D::deleteFiles(const QStringList & cacheDirs,
                                    const std::vector<CachedSource> & sources,
                                    QByteArrayList & deleted,
                                    bool onlyRemoveInvalidItems,
                                    bool & dbChanged)
  {
    /// onlyRemoveInvalidItems has two meanings here:
    ///  1) If true, we check the source and cache timestamp and only remove
    ///     the cache if the source is missing or if the source timestamp is
    ///     newer than the cache
    ///  2) If true, we also modify m_removed and m_cacheItems and update
    ///     dbChanged accordingly. We need to do it here, since the caller
    ///     couldn't have known which files will be deleted. If false, we
    ///     can update m_removed and m_cacheItems more efficiently in the
    ///     caller.
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

                deleted << cacheFile.toUtf8();
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
                deleted << dirIt.next().toUtf8();
              QDir(cacheEntry.absoluteFilePath()).removeRecursively();
            }
          } else {
            if (onlyRemoveInvalidItems && c.sourceTimestamp > 0 &&
                cacheEntry.lastModified().toMSecsSinceEpoch() >= c.sourceTimestamp) {
              sourceHasValidCacheItem = true;
              continue;
            }
            deleted << cacheEntry.absoluteFilePath().toUtf8();
            QFile::remove(cacheEntry.absoluteFilePath());
          }
        }
      }

      if (onlyRemoveInvalidItems && !sourceHasValidCacheItem) {
        dbChanged = true;
        m_removed.insert(c.source);
        auto it = m_cacheItems.find(c.source);
        if (it != m_cacheItems.end())
          m_cacheItems.erase(it);
      }
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
    std::shared_ptr<Radiant::SingleShotTask> saveTask;
    {
      Radiant::Guard g(m_d->m_dbMutex);
      saveTask = std::move(m_d->m_saveTask);
    }
    if (saveTask)
      saveTask->runNow(true);

    Radiant::Guard g(m_d->m_dbMutex);
    m_d->cleanDbConnections();
  }

  QString CacheManager::cacheRoot() const
  {
    return m_d->m_root;
  }

  void CacheManager::setCacheRoot(const QString & cacheRoot)
  {
    Radiant::Guard g(m_d->m_dbMutex);

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
      Radiant::Guard g(m_d->m_dbMutex);
      m_d->m_added.insert(src);
      m_d->m_cacheItems.insert(src);
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
    QByteArrayList deleted;
    QByteArray prefix = sourcePrefix.toUtf8();

    if (m_d->m_root.isEmpty()) {
      Radiant::error("CacheManager::removeFromCache # Can't have empty cacheRoot / source");
      return deleted;
    }

    QStringList cacheDirs;
    QDir root(m_d->m_root);
    for (QString component: root.entryList(QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot))
      cacheDirs << QString("%1/%2").arg(m_d->m_root, component);

    Radiant::Guard g(m_d->m_dbMutex);

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

    bool dbChanged = false;
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

      if (onlyRemoveInvalidItems) {
        ++it;
      } else {
        m_d->m_removed.insert(*it);
        it = m_d->m_cacheItems.erase(it);
        dbChanged = true;
      }
      sources.push_back(std::move(c));
    }

    m_d->deleteFiles(cacheDirs, sources, deleted, onlyRemoveInvalidItems, dbChanged);

    /// It's possible to have cached files of other cache files. For instance
    /// a PDF file as a source can have the individual PDF pages as images in
    /// the cache, and those page images could have mipmaps. However, we assume
    /// that we don't have unlimited recursion but just maximum one level of
    /// nested caches. Therefore we don't call removeFromCache recursively,
    /// since it could be expensive, instead we do a second pass where we check
    /// if any of the deleted cache files had caches.

    sources.clear();
    for (int i = 0, count = deleted.size(); i < count; ++i) {
      const QByteArray & filename = deleted[i];

      it = m_d->m_cacheItems.find(filename);
      if (it == m_d->m_cacheItems.end())
        continue;

      m_d->m_removed.insert(filename);
      m_d->m_cacheItems.erase(it);
      dbChanged = true;

      hash.reset();
      hash.addData(filename);

      CachedSource c;
      c.sourceHexHash = hash.result().toHex();
      sources.push_back(std::move(c));
    }

    m_d->deleteFiles(cacheDirs, sources, deleted, false, dbChanged);

    if (dbChanged)
      m_d->sync();

    return deleted;
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
    Radiant::Guard g(m_d->m_dbMutex);

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

    if (dbChanged)
      m_d->sync();

    return deleted;
  }

  DEFINE_SINGLETON(CacheManager)
}
