#include "PlatformUtils.hpp"
#include "Trace.hpp"

#include <QCryptographicHash>
#include <QDir>
#include <QStandardPaths>

namespace Radiant::PlatformUtils
{
  QString cacheRoot(const QString & component)
  {
    QString basePath = localAppPath();
    if (basePath.isEmpty())
      basePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    basePath += QString("/MultiTaction/cache/%1").arg(component);

    if (!QDir().mkpath(basePath) || !QFileInfo(basePath).isWritable()) {
      basePath = QString("%1/MultiTaction.%2").arg(QDir::tempPath(), component);
      QDir().mkpath(basePath);
    }
    return basePath;
  }

  QString cacheFileName(const QString & cacheRoot, const QString & source,
                        const QString & options, const QString & suffix)
  {
    // Compute a hash from the original source. It might not be a file,
    // so we don't try to resolve it to an absolute path. Do not include
    // timestamp or other information to this hash so that we can easily
    // remove items from the cache. SHA1 seems to be the fastest somewhat
    // reliable hash so it works here well.
    //
    // The cache filename doesn't need to include timestamp, since we can
    // compare the source file and cache file timestamps. This way the old
    // cache entry gets automatically rewritten if the source content
    // changes.
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(source.toUtf8());

    const QString hashTxt = hash.result().toHex();
    QString path = QString("%1/%2/%3").arg(cacheRoot, hashTxt.left(2), hashTxt);
    if (!options.isEmpty())
      path += QString(".%1").arg(options);
    if (!suffix.isEmpty())
      path += QString(".%1").arg(suffix);
    return path;
  }

  QStringList removeFromCache(const QString & cacheRoot, const QString & source)
  {
    /// See cacheFileName
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(source.toUtf8());

    const QString hashTxt = hash.result().toHex();
    const QString prefix = hashTxt.left(2);

    QDir deleteGlob(cacheRoot + "/" + prefix,
                    hashTxt + "*", QDir::NoSort, QDir::Files | QDir::Dirs);

    QStringList out;

    /// Since we are deleting files and directories recursively, we do some
    /// extra sanity checks here

    if (cacheRoot.isEmpty() || source.isEmpty()) {
      Radiant::error("PlatformUtils::removeFromCache # Can't have empty cacheRoot / source");
      return out;
    }

    if (hashTxt.size() != 40 || prefix.size() != 2) {
      Radiant::error("PlatformUtils::removeFromCache # Failed to generate SHA1 hash");
      return out;
    }

    for (const QFileInfo & entry: deleteGlob.entryInfoList()) {
      if (entry.isDir()) {
        out << entry.absoluteFilePath();
        QDir(entry.absoluteFilePath()).removeRecursively();
      } else {
        out << entry.absoluteFilePath();
        QFile::remove(entry.absoluteFilePath());
      }
    }
    return out;
  }
}
