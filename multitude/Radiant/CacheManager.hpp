#pragma once

#include "Flags.hpp"
#include "Singleton.hpp"

#include <QStringList>

namespace Radiant
{
  /// Manages caches like image mipmaps, rendered PDF pages and video previews.
  /// All functions in this class are thread-safe. This class uses
  /// Radiant::BGThread::instance to perform asynchronous tasks.
  class RADIANT_API CacheManager
  {
    DECLARE_SINGLETON(CacheManager);

  public:
    struct CacheItem
    {
      /// Absolute path to the cache file
      QString path;
      /// True if the cache file already exists and the source is not newer than the cache
      bool isValid = false;
    };

    /// Controls what side-effects cacheItem will have
    enum CreateFlag
    {
      /// The function will have no side-effects
      FLAG_NONE        = 0,
      /// Creates the full path to the cache file if it doesn't already exist
      FLAG_CREATE_PATH = 1 << 0,
      /// This cache entry should be written to cache DB.
      FLAG_ADD_TO_DB   = 1 << 1,
      FLAG_DEFAULT     = FLAG_CREATE_PATH | FLAG_ADD_TO_DB
    };
    typedef Radiant::FlagsT<CreateFlag> CreateFlags;

  public:
    ~CacheManager();

    /// Global cache root. Defaults to
    /// %LOCALAPPDATA%/MultiTaction/cache or
    /// $HOME/MultiTaction/cache
    /// All cache files are normally written inside the cache root, but if it's
    /// unwriteable, system temporary directory is used instead.
    QString cacheRoot() const;

    /// Override the default cache root. Typically only used in testing.
    /// Changing this after cacheItem was called is undefined behaviour.
    /// Setting the root to empty string clears the override and restores the
    /// default cache root.
    void setCacheRoot(const QString & cacheRoot);

    /// Returns path to user-specific local cache dir for the given component.
    /// Also creates the path if it doesn't exist.
    /// Component can be "pdf" or "mipmaps" etc.
    /// Returns <cacheRoot>/<component>
    QString createCacheDir(const QString & component);

    /// Creates a cache entry that can be used to write a cache file of the
    /// given source. Also checks if the cache file already exists and can
    /// be used by comparing the file timestamp to the source. If the source
    /// is not a file, the timestamp check is ignored.
    /// @param source filename or similar that uniquely identifies the asset
    ///        where the cache is based on. This could be for instance an
    ///        absolute path to a PDF file.
    /// @param options Optional extra string that is appended to the filename.
    ///        This could be for instance a hash of some PDF rendering parameters.
    /// @param suffix Optional file suffix without the dot.
    /// Path component in the returned cache item has the following form:
    /// <cache root>/<beginning of source hash>/<source hash>.<options>.<suffix>
    CacheItem cacheItem(const QString & cacheDir, const QString & source,
                        const QString & options = QString(),
                        const QString & suffix = QString(),
                        CreateFlags flags = FLAG_DEFAULT);

    /// Remove all cache files and directories from disk cache that were
    /// generated from a source that started with the given source prefix.
    /// Returns absolute filenames of all deleted files.
    QByteArrayList removeFromCache(const QString & sourcePrefix, bool onlyRemoveInvalidItems);

    /// Deletes all cached files inside the given cache directory recursively.
    /// Returns absolute filenames of all deleted files.
    QByteArrayList clearCacheDir(const QString & cacheDir);

  private:
    CacheManager();

  private:
    class D;
    std::unique_ptr<D> m_d;
  };

  MULTI_FLAGS(CacheManager::CreateFlag)
}
