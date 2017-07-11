/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Mipmap.hpp"

#include "Luminous/Image.hpp"
#include "Luminous/Texture.hpp"
#include "Luminous/MemoryManager.hpp"
#include "Luminous/MipMapGenerator.hpp"
#include "Luminous/RenderManager.hpp"

#include <Radiant/FileUtils.hpp>
#include <Radiant/PlatformUtils.hpp>
#include <Radiant/Sleep.hpp>
#include <Radiant/BGThread.hpp>

#include <QFileInfo>
#include <QCryptographicHash>
#include <QDir>
#include <QSemaphore>
#include <QSettings>
#include <QDateTime>

#include <atomic>

namespace
{
  typedef std::map<QPair<QByteArray, bool>, std::weak_ptr<Luminous::Mipmap>> MipmapStore;
  MipmapStore s_mipmapStore;
  Radiant::Mutex s_mipmapStoreMutex;

  /// The smallest mipmap is always kept in CPU memory, but not on GPU memory.
  /// However, on GPU the smallest mipmap level uses hardware trilinear mipmaps.
  /// 1000 128x128 RGBA images will consume total 83 MB GPU memory and 63 MB CPU memory
  const unsigned int s_smallestImage = 128;
  const Radiant::Priority s_defaultPingPriority = Radiant::Task::PRIORITY_HIGH + 2;

  /// Update this when there is incompatible change in imagecache
  const unsigned int s_imageCacheVersion = 1;

  bool s_dxtSupported = true;

  /// Special time values in MipmapLevel::lastUsed
  enum LoadState {
    New,
    Loading,
    LoadError,
    StateCount
  };

  /// Current time, unit is the same as in RenderManager::frameTime.
  /// This file excludes LoadState times.
  /// See also MipmapLevel::lastUsed
  inline int frameTime();

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /// One mipmap level, Mipmap has a vector of these. By default objects are
  /// "empty", meaning that the texture is invalid (!isValid()) and images
  /// are null. LoadTasks will load this when needed, and MipmapReleaseTask
  /// will expire these (set to empty state)
  struct MipmapLevel
  {
    MipmapLevel() : loadingPriority(0) {}

    MipmapLevel(MipmapLevel && t);
    MipmapLevel & operator=(MipmapLevel && t);

    /// Only one of the image types is defined at once
    std::unique_ptr<Luminous::CompressedImage> cimage;
    std::unique_ptr<Luminous::Image> image;

    Luminous::Texture texture;

    int loadingPriority;
    std::weak_ptr<Radiant::Task> loader;

    /// Either LoadState enum value, or time when this class was last used.
    /// These need to be in the same atomic int, or alternatively we could do
    /// this nicer by using separate enum and timestamp, but this way we have
    /// fast lockless synchronization between all threads
    QAtomicInt lastUsed;

    /// During expiration, this will be 1. If you are doing something with this
    /// MipmapLevel without updating lastUsed, you can lock the MipmapLevel from
    /// being deleted by setting locked to 1 from 0.
    QAtomicInt locked;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  int frameTime()
  {
    /// first ones are reserved
    return StateCount + Luminous::RenderManager::frameTime();
  }

  // This frame has already been rendered
  int lastFrameTime()
  {
    return StateCount + Luminous::RenderManager::lastFrameTime();
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  MipmapLevel::MipmapLevel(MipmapLevel && t)
    : cimage(std::move(t.cimage))
    , image(std::move(t.image))
    , texture(std::move(t.texture))
    , loadingPriority(t.loadingPriority)
    , loader(std::move(t.loader))
    , lastUsed(t.lastUsed)
    , locked(t.locked)
  {
  }

  MipmapLevel & MipmapLevel::operator=(MipmapLevel && t)
  {
    cimage = std::move(t.cimage);
    image = std::move(t.image);
    texture = std::move(t.texture);
    loadingPriority = t.loadingPriority;
    loader = std::move(t.loader);
    lastUsed = t.lastUsed;
    locked = t.locked;
    return *this;
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace Luminous
{
  /// Loads uncompressed mipmaps from file to MipmapLevel / create them if necessary
  class LoadImageTask : public Radiant::Task
  {
  public:
    LoadImageTask(Luminous::MipmapPtr mipmap, Radiant::Priority priority,
                  const QString & filename, int level, const QString & cacheFileFormat);

  protected:
    virtual void doTask() OVERRIDE;
    void setState(Luminous::Mipmap & mipmap, Valuable::LoadingEnum state);

  private:
    bool recursiveLoad(Luminous::Mipmap & mipmap, int level);
    bool recursiveLoad(Luminous::Mipmap & mipmap, MipmapLevel & imageTex, int level);
    bool tryLock(Luminous::Mipmap & mipmap, int level);
    void lock(Luminous::Mipmap & mipmap, int level);
    void unlock(Luminous::Mipmap & mipmap, int level);

  protected:
    std::weak_ptr<Luminous::Mipmap> m_mipmap;
    const QString & m_filename;
    const QString m_cacheFileFormat;
    int m_level;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /// Loads existing compressed mipmaps from file to MipmapLevel
  class LoadCompressedImageTask : public LoadImageTask
  {
  public:
    LoadCompressedImageTask(Luminous::MipmapPtr mipmap, MipmapLevel & tex,
                            Radiant::Priority priority, const QString & filename, int level);

  protected:
    virtual void doTask() OVERRIDE;

  private:
    MipmapLevel & m_tex;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class PingTask : public Radiant::Task
  {
  public:
    PingTask(const MipmapPtr & mipmap, bool compressedMipmaps);

    void finishAndWait();

  protected:
    virtual void doTask() OVERRIDE;

  private:
    bool ping(Luminous::Mipmap::D & mipmap);

  private:
    bool m_preferCompressedMipmaps;
    std::weak_ptr<Luminous::Mipmap> m_mipmap;
    QSemaphore m_users;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /// Iterates all mipmaps and their mipmap levels and expires unused images.
  /// This one task takes care of the whole expiration process for all images.
  /// It locks s_mipmapStoreMutex for a really short period at a time, so it
  /// doesn't slow down the application if the main thread is creating new
  /// Mipmap instances. The expiration checking is atomic operation without
  /// need to lock anything, so this will have no impact on rendering threads.
  class MipmapReleaseTask : public Radiant::Task, public Valuable::Node
  {
  public:
    MipmapReleaseTask();

    static void check(float wait);

  protected:
    virtual void doTask() OVERRIDE;

  private:
    MemoryManagerPtr m_memoryManager = MemoryManager::instance();
    bool m_outOfMemory = false;
  };
  std::weak_ptr<MipmapReleaseTask> s_releaseTask;

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class Mipmap::D
  {
  public:
    D(Mipmap & mipmap, const QString & filenameAbs);
    ~D();

    MipmapLevel * find(unsigned int level, unsigned int * returnedLevel,
                       int priorityChange);

    bool isLevelAvailable(unsigned int level) const;

  public:
    Mipmap & m_mipmap;

    const QString m_filenameAbs;
    Nimble::Size m_nativeSize;
    int m_maxLevel;

    QDateTime m_fileModified;

    QString m_compressedMipmapFile;
    bool m_useCompressedMipmaps;
    Radiant::Priority m_loadingPriority;

    ImageInfo m_sourceInfo;
    ImageInfo m_compressedMipmapInfo;

    std::shared_ptr<PingTask> m_ping;
    std::shared_ptr<MipMapGenerator> m_mipmapGenerator;

    QString m_mipmapFormat;

    std::vector<MipmapLevel> m_levels;

    std::atomic<int> m_expireDeciSeconds;

    Valuable::LoadingState m_state;

    std::atomic<bool> m_isObsolete{false};
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  LoadImageTask::LoadImageTask(Luminous::MipmapPtr mipmap, Radiant::Priority priority,
                               const QString & filename, int level, const QString & cacheFileFormat)
    : Task(priority)
    , m_mipmap(mipmap)
    , m_filename(filename)
    , m_cacheFileFormat(cacheFileFormat)
    , m_level(level)
  {}

  void LoadImageTask::doTask()
  {
    Luminous::MipmapPtr mipmap = m_mipmap.lock();
    if (!mipmap) {
      setFinished();
      return;
    }
    if (!tryLock(*mipmap, m_level)) {
      scheduleFromNowSecs(0.005);
      return;
    }
    if (recursiveLoad(*mipmap, m_level)) {
      setState(*mipmap, Valuable::STATE_READY);
    } else {
      setState(*mipmap, Valuable::STATE_ERROR);
    }
    unlock(*mipmap, m_level);
    setFinished();
  }

  void LoadImageTask::setState(Luminous::Mipmap & mipmap, Valuable::LoadingEnum state)
  {
    mipmap.m_d->m_state = state;
  }

  bool LoadImageTask::tryLock(Luminous::Mipmap & mipmap, int level)
  {
    MipmapLevel & imageTex = mipmap.m_d->m_levels[level];

    return imageTex.locked.testAndSetOrdered(0, 1);
  }

  void LoadImageTask::lock(Luminous::Mipmap & mipmap, int level)
  {
    MipmapLevel & imageTex = mipmap.m_d->m_levels[level];

    /// If this fails, then another task is just creating a mipmap for this level,
    /// or MipmapReleaseTask is releasing it
    int i = 0;
    while (!imageTex.locked.testAndSetOrdered(0, 1))
      Radiant::Sleep::sleepMs(std::min(20, ++i));
  }

  void LoadImageTask::unlock(Luminous::Mipmap & mipmap, int level)
  {
    MipmapLevel & imageTex = mipmap.m_d->m_levels[level];
    imageTex.locked = 0;
  }

  bool LoadImageTask::recursiveLoad(Luminous::Mipmap & mipmap, int level)
  {
    MipmapLevel & imageTex = mipmap.m_d->m_levels[level];

    int lastUsed = imageTex.lastUsed.load();
    if (lastUsed == LoadError) {
      return false;
    }

    if (lastUsed >= StateCount) {
      if (level == m_level)
        imageTex.lastUsed = frameTime();
      return true;
    }

    bool ok = recursiveLoad(mipmap, imageTex, level);
    if (ok) {
      /// @todo use Image::texture
      imageTex.texture.setData(imageTex.image->width(), imageTex.image->height(),
                               imageTex.image->pixelFormat(), imageTex.image->data());
      imageTex.texture.setLineSizePixels(0);
      int now = frameTime();
      if (m_level == level) {
        imageTex.lastUsed = now;
      } else {
        /// FIXME: why +0.3f and 0.31f?
        imageTex.lastUsed = Nimble::Math::Clamp<int>(now - mipmap.m_d->m_expireDeciSeconds + 0.3f, StateCount, now);
        MipmapReleaseTask::check(0.31f);
      }
      if (level == mipmap.m_d->m_maxLevel) {
        imageTex.texture.setMinFilter(Texture::FILTER_LINEAR_MIPMAP_LINEAR);
        imageTex.texture.setMipmapsEnabled(true);
      }
    } else {
      imageTex.image.reset();
      imageTex.lastUsed = LoadError;
    }
    return ok;
  }

  bool LoadImageTask::recursiveLoad(Luminous::Mipmap & mipmap, MipmapLevel & imageTex, int level)
  {
    if (level == 0) {
      // Load original
      if (!imageTex.image)
        imageTex.image.reset(new Image());

      if (!imageTex.image->read(m_filename, true)) {
        Radiant::error("LoadImageTask::recursiveLoad # Could not read '%s' [original image]", m_filename.toUtf8().data());
        return false;
      } else {
        return true;
      }
    }

    // Try loading a pre-generated smaller-scale mipmap
    const QString filename = Mipmap::cacheFileName(m_filename, level, m_cacheFileFormat);

    const Radiant::TimeStamp origTs = Radiant::FileUtils::lastModified(m_filename);
    if (origTs > Radiant::TimeStamp(0) && Radiant::FileUtils::fileReadable(filename) &&
        Radiant::FileUtils::lastModified(filename) > origTs) {

      if (!imageTex.image)
        imageTex.image.reset(new Image());

      Nimble::Size expectedSize = mipmap.mipmapSize(level);
      if (!imageTex.image->read(filename, true)) {
        Radiant::error("LoadImageTask::recursiveLoad # Could not read '%s' [mipmap level %d/%d of image %s, expected size: (%d, %d)]",
                       filename.toUtf8().data(), level, mipmap.m_d->m_maxLevel,
                       m_filename.toUtf8().data(), expectedSize.width(), expectedSize.height());
      } else if (expectedSize != imageTex.image->size()) {
        // unexpected size (corrupted or just old image)
        Radiant::error("LoadImageTask::recursiveLoad # Cache image '%s'' size was (%d, %d), expected (%d, %d)",
              filename.toUtf8().data(), imageTex.image->width(), imageTex.image->height(),
                       expectedSize.width(), expectedSize.height());
      } else {
        return true;
      }
    }


    {
      lock(mipmap, level - 1);

      // Load the bigger image from lower level, and scale down from that:
      if (!recursiveLoad(mipmap, level - 1)) {
        unlock(mipmap, level - 1);
        return false;
      }

      Image & imsrc = *mipmap.m_d->m_levels[level - 1].image;

      // Scale down from bigger mipmap
      if (!imageTex.image)
        imageTex.image.reset(new Image());

      const Nimble::Size is = mipmap.mipmapSize(level);

      imageTex.image->minify(imsrc, is.width(), is.height());
      unlock(mipmap, level - 1);
    }

    QDir().mkpath(Radiant::FileUtils::path(filename));
    imageTex.image->write(filename);

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  LoadCompressedImageTask::LoadCompressedImageTask(
      Luminous::MipmapPtr mipmap, MipmapLevel & tex, Radiant::Priority priority,
      const QString & filename, int level)
    : LoadImageTask(mipmap, priority, filename, level, "dds")
    , m_tex(tex)
  {}

  void LoadCompressedImageTask::doTask()
  {
    Luminous::MipmapPtr mipmap = m_mipmap.lock();
    if (!mipmap) {
      setFinished();
      return;
    }

    std::unique_ptr<Luminous::CompressedImage> im(new Luminous::CompressedImage);
    if(!im->read(m_filename, m_level)) {
      Radiant::error("LoadCompressedImageTask::doTask # Could not read %s level %d", m_filename.toUtf8().data(), m_level);
      m_tex.lastUsed = LoadError;
      setState(*mipmap, Valuable::STATE_ERROR);
    } else {
      m_tex.texture.setData(im->width(), im->height(), im->compression(), im->data());
      m_tex.cimage = std::move(im);
      int now = frameTime();
      m_tex.lastUsed.testAndSetOrdered(Loading, now);
      setState(*mipmap, Valuable::STATE_READY);
    }
    setFinished();
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  PingTask::PingTask(const MipmapPtr & mipmap, bool compressedMipmaps)
    : Task(s_defaultPingPriority)
    , m_preferCompressedMipmaps(compressedMipmaps)
    , m_mipmap(mipmap)
    , m_users(1)
  {}

  void PingTask::finishAndWait()
  {
    setFinished();
    m_users.acquire();
  }

  void PingTask::doTask()
  {
    auto mipmap = m_mipmap.lock();
    if (!mipmap) {
      setFinished();
      return;
    }

    if(!m_users.tryAcquire()) {
      // The only explanation for this is that Mipmap already called finishAndWait()

      // BGThread keeps one copy of shared_ptr to this alive during doTask(),
      // so we can manually remove this from Mipmap::D
      mipmap->m_d->m_ping.reset();
      setFinished();
      return;
    }

    ping(*mipmap->m_d);

    mipmap->m_d->m_ping.reset();
    m_users.release();
    setFinished();
  }

  bool PingTask::ping(Luminous::Mipmap::D & mipmap)
  {
    QFileInfo fi(mipmap.m_filenameAbs);
    QDateTime lastModified = fi.lastModified();
    mipmap.m_fileModified = lastModified;

    if(!Luminous::Image::ping(mipmap.m_filenameAbs, mipmap.m_sourceInfo)) {
      Radiant::error("PingTask::doPing # failed to query image size for %s",
                     mipmap.m_filenameAbs.toUtf8().data());
      mipmap.m_state = Valuable::STATE_ERROR;
      return false;
    }

    if(!s_dxtSupported && mipmap.m_sourceInfo.pf.compression() != Luminous::PixelFormat::COMPRESSION_NONE) {
      Radiant::error("PingTask::doPing # Image %s has unsupported format",
                     mipmap.m_filenameAbs.toUtf8().data());
      mipmap.m_state = Valuable::STATE_ERROR;
      return false;
    }

    mipmap.m_nativeSize.make(mipmap.m_sourceInfo.width, mipmap.m_sourceInfo.height);
    mipmap.m_maxLevel = 0;
    for (unsigned int w = mipmap.m_nativeSize.width(), h = mipmap.m_nativeSize.height();
        std::max(w, h) > s_smallestImage && w > 1 && h > 1; w >>= 1, h >>= 1)
      ++mipmap.m_maxLevel;

    // Use DXT compression if it is requested and supported
    mipmap.m_useCompressedMipmaps = m_preferCompressedMipmaps && s_dxtSupported;


#ifndef LUMINOUS_OPENGLES

    if(mipmap.m_sourceInfo.pf.compression() && (
         mipmap.m_sourceInfo.mipmaps > 1 ||
         (mipmap.m_sourceInfo.width < 5 && mipmap.m_sourceInfo.height < 5))) {
      // We already have compressed image with mipmaps, no need to generate more
      mipmap.m_useCompressedMipmaps = false;
    }

    if(mipmap.m_useCompressedMipmaps) {
      mipmap.m_compressedMipmapFile = Luminous::Mipmap::cacheFileName(mipmap.m_filenameAbs, -1, "dds");
      QFileInfo compressedMipmap(mipmap.m_compressedMipmapFile);
      QDateTime compressedMipmapTs;
      if(compressedMipmap.exists())
        compressedMipmapTs = compressedMipmap.lastModified();

      if(compressedMipmapTs.isValid() && compressedMipmapTs < mipmap.m_fileModified) {
        if(!Luminous::Image::ping(mipmap.m_compressedMipmapFile, mipmap.m_compressedMipmapInfo))
          compressedMipmapTs = QDateTime();
      }
      if(!compressedMipmapTs.isValid()) {
        mipmap.m_mipmapGenerator.reset(new MipMapGenerator(mipmap.m_filenameAbs, mipmap.m_compressedMipmapFile));
        auto weak = m_mipmap;
        mipmap.m_mipmapGenerator->setListener([=] (bool ok, const ImageInfo & imginfo) {
          auto ptr = weak.lock();
          if (ptr) {
            if (ok) {
              ptr->setMipmapReady(imginfo);
            } else {
              ptr->m_d->m_mipmapGenerator.reset();
              ptr->m_d->m_state = Valuable::STATE_ERROR;
            }
          }
        });
      }
    }
#endif // LUMINOUS_OPENGLES

    mipmap.m_levels.resize(mipmap.m_maxLevel+1);
    mipmap.m_state = Valuable::STATE_HEADER_READY;

#ifndef LUMINOUS_OPENGLES
    if(mipmap.m_mipmapGenerator) {
      Radiant::BGThread::instance()->addTask(mipmap.m_mipmapGenerator);
    } else
#endif // LUMINOUS_OPENGLES
    {
      // preload the maximum level mipmap image
      mipmap.m_mipmap.texture(mipmap.m_maxLevel);
    }
    return true;
  }

  ///////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////

  MipmapReleaseTask::MipmapReleaseTask()
    : Task(PRIORITY_URGENT)
  {
    scheduleFromNowSecs(10.0f);
    m_memoryManager->eventAddListener("out-of-memory", this, [this] {
      if (!m_outOfMemory) {
        m_outOfMemory = true;
        check(0);
      }
    });
  }

  void MipmapReleaseTask::doTask()
  {
    // This is how much we should try to release memory
    const uint64_t todoBytes = m_memoryManager->overallocatedBytes();

    // Nothing to do, there is already enough available memory
    if (todoBytes == 0) {
      scheduleFromNowSecs(60.0);
      m_outOfMemory = false;
      return;
    }

    float delay = 10;

    // lastUsed => (mipmap, level)
    std::multimap<int, std::pair<MipmapPtr, int>> queue;

    Radiant::Guard g(s_mipmapStoreMutex);
    const int now = lastFrameTime();

    for(MipmapStore::iterator it = s_mipmapStore.begin(); it != s_mipmapStore.end();) {
      Luminous::MipmapPtr ptr = it->second.lock();
      if(ptr) {
        if(ptr->isHeaderReady()) {
          const int expire = ptr->m_d->m_expireDeciSeconds;
          std::vector<MipmapLevel> & levels = ptr->m_d->m_levels;
          // do not expire the last mipmap level (smallest image)
          for(int level = 0, s = static_cast<int>(levels.size()) - 1; level < s; ++level) {
            MipmapLevel & imageTex = levels[level];
            int lastUsed = imageTex.lastUsed.load();
            if (lastUsed <= Loading)
              continue;

            if(now >= lastUsed + expire) {
              queue.emplace(lastUsed, std::make_pair(ptr, level));
            } else {
              delay = std::min(delay, (lastUsed + expire - now) / 10.0f);
            }
          }
        }
        ++it;
      } else {
        it = s_mipmapStore.erase(it);
      }

      // Do not reserve the store, other might want to access to it also
      s_mipmapStoreMutex.unlock();
      s_mipmapStoreMutex.lock();
    }

    uint64_t releasedBytes = 0;
    for (auto & p: queue) {
      const int lastUsed = p.first;
      MipmapPtr & mipmap = p.second.first;
      const int level = p.second.second;

      MipmapLevel & imageTex = mipmap->m_d->m_levels[level];

      if (imageTex.locked.testAndSetOrdered(0, 1)) {
        if (imageTex.lastUsed.testAndSetOrdered(lastUsed, Loading)) {
          imageTex.texture.reset();
          if (imageTex.cimage) {
            releasedBytes += imageTex.cimage->datasize() + sizeof(*imageTex.cimage);
            imageTex.cimage.reset();
          }
          if (imageTex.image) {
            releasedBytes += imageTex.image->width() * imageTex.image->height() * imageTex.image->pixelFormat().bytesPerPixel()
                + sizeof(*imageTex.image);
            imageTex.image.reset();
          }
          imageTex.lastUsed = New;
        }
        imageTex.locked = 0;
      } else {
        delay = 0;
      }

      if (releasedBytes >= todoBytes) {
        break;
      }
    }

    if (releasedBytes >= todoBytes) {
      m_outOfMemory = false;
      scheduleFromNowSecs(60);
    } else {
      scheduleFromNowSecs(std::max(delay, 0.5f));
    }
  }

  void MipmapReleaseTask::check(float wait)
  {
    auto task = s_releaseTask.lock();
    /// @todo thread safety, task might be running/rescheduling concurrently
    if (task && task->m_outOfMemory && (task->secondsUntilScheduled() > wait)) {
      task->scheduleFromNowSecs(wait);
      Radiant::BGThread::instance()->reschedule(task);
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  Mipmap::D::D(Mipmap & mipmap, const QString & filenameAbs)
    : m_mipmap(mipmap)
    , m_filenameAbs(filenameAbs)
    , m_nativeSize(0, 0)
    , m_maxLevel(0)
    , m_useCompressedMipmaps(false)
    , m_loadingPriority(Radiant::Task::PRIORITY_NORMAL)
    , m_mipmapFormat("csimg")
    , m_expireDeciSeconds(30)
    , m_state(Valuable::STATE_NEW)
  {
    MULTI_ONCE {
      auto releaseTask = std::make_shared<MipmapReleaseTask>();
      s_releaseTask = releaseTask;
      Radiant::BGThread::instance()->addTask(releaseTask);
    }
  }

  Mipmap::D::~D()
  {
    // Make a local copy, if PingTask is just finishing and removes m_d->m_ping
    std::shared_ptr<PingTask> ping = m_ping;
    if(ping) {
      Radiant::BGThread::instance()->removeTask(ping, true, true);
      ping->finishAndWait();
    }
    if (m_mipmapGenerator)
      Radiant::BGThread::instance()->removeTask(m_mipmapGenerator);
  }

  MipmapLevel * Mipmap::D::find(unsigned int requestedLevel, unsigned int * returnedLevel, int priorityChange)
  {
    // If a mipmap is invalid, it means that there is no way ever to read this file
    if (!m_mipmap.isValid())
      return nullptr;

    // If we haven't pinged the image yet, and it seems that this is (un)important image,
    // reschedule the ping task with updated priority
    if (!m_mipmap.isHeaderReady()) {
      if (priorityChange != 0) {
        auto ping = m_ping;

        const int newPriority = s_defaultPingPriority + priorityChange;
        if (ping && newPriority != ping->priority())
          Radiant::BGThread::instance()->reschedule(ping, newPriority);
      }
      return nullptr;
    }

    const int req = std::min<int>(requestedLevel, m_maxLevel);

    // If the image isn't yet loaded, lets check if we could reschedule mipmap
    // generator task or the correct Load(Compressed)ImageTask.
    if (!m_mipmap.isReady()) {
      auto gen = m_mipmapGenerator;
      if (gen) {
        const int newGenPriority = MipMapGenerator::defaultPriority() + priorityChange;
        if (newGenPriority != gen->priority())
          Radiant::BGThread::instance()->reschedule(gen, newGenPriority);
        // We are still generating mipmaps, nothing to do here
        return nullptr;
      }
    }

    int time = frameTime();
    const int newLoadPriority = m_loadingPriority + priorityChange;

    for(int level = req, diff = -1; level <= m_maxLevel; level += diff) {
      if(level < 0) {
        level = req;
        diff = 1;
      } else {
        MipmapLevel & imageTex = m_levels[level];

        int old = imageTex.lastUsed.load();

        while(true) {
          int now = time;
          if(now == old) {
            if(returnedLevel)
              *returnedLevel = level;
            return &imageTex;
          }

          // Reschedule loader tasks
          if (old == Loading && level == req && imageTex.loadingPriority != newLoadPriority) {
            imageTex.loadingPriority = newLoadPriority;
            auto loader = imageTex.loader.lock();
            if (loader)
              Radiant::BGThread::instance()->reschedule(loader, newLoadPriority);
          }

          if(old == Loading || old == LoadError)
            break;

          // Only start loading new images if this is the correct level
          if(old == New && level != req)
            break;

          if(old == New)
            now = Loading;

          if(imageTex.lastUsed.testAndSetOrdered(old, now)) {
            if(now == Loading) {
              Radiant::TaskPtr task;
              if(m_useCompressedMipmaps) {
                task = std::make_shared<LoadCompressedImageTask>(m_mipmap.shared_from_this(), imageTex,
                                                                 m_loadingPriority + priorityChange,
                                                                 m_compressedMipmapFile, level);
              } else if(m_sourceInfo.pf.compression() != PixelFormat::COMPRESSION_NONE) {
                task = std::make_shared<LoadCompressedImageTask>(m_mipmap.shared_from_this(), imageTex,
                                                                 m_loadingPriority + priorityChange,
                                                                 m_filenameAbs, level);
              } else {
                task = std::make_shared<LoadImageTask>(m_mipmap.shared_from_this(),
                                                       m_loadingPriority + priorityChange,
                                                       m_filenameAbs, level, m_mipmapFormat);
              }
              Radiant::BGThread::instance()->addTask(task);
              imageTex.loadingPriority = task->priority();
              imageTex.loader = task;
              break;
            }
            if(returnedLevel)
              *returnedLevel = level;
            return &imageTex;
          } else {
            old = imageTex.lastUsed.load();
          }
        }
      }
    }

    return nullptr;
  }

  bool Mipmap::D::isLevelAvailable(unsigned int level) const
  {
    if (!m_mipmap.isValid())
      return false;

    if(!m_mipmap.isReady())
      return false;

    if(int(level) > m_maxLevel)
      return false;

    const MipmapLevel & imageTex = m_levels[level];
    if(imageTex.lastUsed <= Loading)
      return false;

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  Mipmap::Mipmap(const QString & filenameAbs)
    : m_d(new D(*this, filenameAbs))
  {
    eventAddOut("reloaded");
  }

  Mipmap::~Mipmap()
  {
    delete m_d;
  }

  Texture * Mipmap::texture(unsigned int requestedLevel, unsigned int * returnedLevel, int priorityChange)
  {
    MipmapLevel * level = m_d->find(requestedLevel, returnedLevel, priorityChange);
    return level ? &level->texture : nullptr;
  }

  Image * Mipmap::image(unsigned int requestedLevel, unsigned int * returnedLevel, int priorityChange)
  {
    MipmapLevel * level = m_d->find(requestedLevel, returnedLevel, priorityChange);
    return level ? level->image.get() : nullptr;
  }

  Texture * Mipmap::texture(const Nimble::Matrix4 & transform, Nimble::SizeF pixelSize, unsigned int maxSize)
  {
    int idealLevel = level(transform, pixelSize, maxSize);
    return texture(idealLevel);
  }

  bool Mipmap::isLevelAvailable(unsigned int level) const
  {
    return m_d->isLevelAvailable(level);
  }

  CompressedImage * Mipmap::compressedImage(unsigned int requestedLevel, unsigned int * returnedLevel, int priorityChange)
  {
    MipmapLevel * level = m_d->find(requestedLevel, returnedLevel, priorityChange);
    return level ? level->cimage.get() : nullptr;
  }

  unsigned int Mipmap::level(const Nimble::Matrix4 & transform, Nimble::SizeF pixelSize,
                             unsigned int maxSize, float * trilinearBlending) const
  {
    // Assume that the view matrix is ortho projection with no scaling
    // we can ignore Z and just look X/Y vector projections to determine the maximum scaling
    float sx = Nimble::Vector2f(transform[0][0], transform[0][1]).length();
    float sy = Nimble::Vector2f(transform[1][0], transform[1][1]).length();
    unsigned int l = level(std::max(sx, sy) * pixelSize, trilinearBlending);
    for (; maxSize > 0 && l < (unsigned)m_d->m_maxLevel; ++l) {
      const Nimble::Size size = mipmapSize(l);
      if (size.maximum() <= (int)maxSize) {
        break;
      }
      if (trilinearBlending)
        *trilinearBlending = 0;
    }
    return l;
  }

  unsigned int Mipmap::level(Nimble::SizeF pixelSize, float * trilinearBlending) const
  {
    const float ask = pixelSize.maximum();

    // We could try to calculate correct blending and level parameter with
    // log(ask/size) / log(0.5), but that doesn't take into account the
    // rounding errors we get by >> 1

    int size = m_d->m_nativeSize.maximum();
    if (ask >= size) {
      if (trilinearBlending)
        *trilinearBlending = 0;
      return 0;
    }

    for (int level = 1; level <= m_d->m_maxLevel; ++level) {
      int newsize = size >> 1;
      if (ask > newsize) {
        if (trilinearBlending)
          *trilinearBlending = std::max(0.f, 1.0f-(ask-newsize)/(size-newsize));
        return level - 1;
      }
      size = newsize;
    }

    if (trilinearBlending)
      *trilinearBlending = 0;
    return m_d->m_maxLevel;
  }

  unsigned int Mipmap::maxLevel() const
  {
    return m_d->m_maxLevel;
  }

  const Nimble::Size & Mipmap::nativeSize() const
  {
    return m_d->m_nativeSize;
  }

  float Mipmap::aspect() const
  {
    Nimble::Size native = m_d->m_nativeSize;

    return native.height() ? native.width() / (float) native.height() : 1.0f;
  }

  bool Mipmap::isReady() const
  {
    return m_d->m_state.state() == Valuable::STATE_READY;
  }

  bool Mipmap::isHeaderReady() const
  {
    auto s = m_d->m_state.state();
    return s == Valuable::STATE_READY || s == Valuable::STATE_HEADER_READY;
  }

  bool Mipmap::isValid() const
  {
    return m_d->m_state != Valuable::STATE_ERROR;
  }

  bool Mipmap::hasAlpha() const
  {
    return m_d->m_sourceInfo.pf.hasAlpha();
  }

  float Mipmap::pixelAlpha(Nimble::Vector2 relLoc) const
  {
    if(!isHeaderReady() || !isValid()) return 1.0f;

    int time = frameTime();
    for(int level = 0; level <= m_d->m_maxLevel; ) {
      MipmapLevel & imageTex = m_d->m_levels[level];
      int old = imageTex.lastUsed.load();
      if(old == New || old == Loading || old == LoadError) {
        ++level;
        continue;
      }

      // We try to reserve the imagetex for us, so that it won't be deleted at
      // the same time. If that fails, we are then forced to atomically update
      // lastUsed to current time. In practise that means that there is no
      // waiting in this function, but it's still perfectly thread-safe.
      bool locked = imageTex.locked.testAndSetOrdered(0, 1);

#ifndef LUMINOUS_OPENGLES
      if(imageTex.cimage) {
        if(locked || old == time || imageTex.lastUsed.testAndSetOrdered(old, time)) {
          Nimble::Vector2i pixel(relLoc.x * imageTex.cimage->width(), relLoc.y * imageTex.cimage->height());
          float v = imageTex.cimage->readAlpha(pixel);
          if(locked) imageTex.locked = 0;
          return v;
        } else {
          // try again
          continue;
        }
      }
#endif // LUMINOUS_OPENGLES

      if(imageTex.image) {
        if(locked || old == time || imageTex.lastUsed.testAndSetOrdered(old, time)) {
          Nimble::Vector2i pixel(relLoc.x * imageTex.image->width(), relLoc.y * imageTex.image->height());
          float v = imageTex.image->safePixel(pixel.x, pixel.y).w;
          if(locked) imageTex.locked = 0;
          return v;
        } else {
          continue;
        }
      }
    }

    return 1.0f;
  }

  void Mipmap::setLoadingPriority(Radiant::Priority priority)
  {
    m_d->m_loadingPriority = priority;
  }

  int Mipmap::expirationTimeDeciSeconds() const
  {
    return m_d->m_expireDeciSeconds;
  }

  void Mipmap::setExpirationTimeDeciSeconds(int deciseconds)
  {
    int old = m_d->m_expireDeciSeconds;
    m_d->m_expireDeciSeconds = deciseconds;
    if(deciseconds < old)
      MipmapReleaseTask::check(0.0f);
  }

  Nimble::Size Mipmap::mipmapSize(unsigned int level) const
  {
    if(level == 0) return m_d->m_nativeSize;
    Nimble::Size size(m_d->m_nativeSize.width() >> level,
                      m_d->m_nativeSize.height() >> level);
    if (size.width() == 0 || size.height() == 0)
      return Nimble::Size(0, 0);
    return size;
  }

  const QString & Mipmap::filename() const
  {
    return m_d->m_filenameAbs;
  }

  Radiant::TaskPtr Mipmap::pingTask()
  {
    return m_d->m_ping;
  }

  Radiant::TaskPtr Mipmap::mipmapGeneratorTask()
  {
    return m_d->m_mipmapGenerator;
  }

  Radiant::TaskPtr Mipmap::loadingTask()
  {
    for (std::size_t level = 0; level < m_d->m_levels.size(); ++level) {
      MipmapLevel & imageTex = m_d->m_levels[level];
      auto loader = imageTex.loader.lock();
      if (loader)
        return loader;
    }
    return nullptr;
  }

  Valuable::LoadingState & Mipmap::state()
  {
    return m_d->m_state;
  }

  const Valuable::LoadingState & Mipmap::state() const
  {
    return m_d->m_state;
  }

  bool Mipmap::isObsolete() const
  {
    return m_d->m_isObsolete;
  }

  void Mipmap::setMipmapReady(const ImageInfo & imginfo)
  {
    m_d->m_compressedMipmapInfo = imginfo;

    m_d->m_mipmapGenerator.reset();
    m_d->m_state = Valuable::STATE_READY;
    // preload the maximum level mipmap image
    texture(m_d->m_maxLevel);
  }

  std::shared_ptr<Mipmap> Mipmap::acquire(const QString & filename,
                                          bool compressedMipmaps)
  {
    if (filename.isEmpty())
      return nullptr;

    QFileInfo fi(filename);
    QString id(fi.absoluteFilePath() + fi.lastModified().toString());
    QPair<QByteArray, bool> key = qMakePair(id.toUtf8(), compressedMipmaps);

    if(key.first.isEmpty()) {
      Radiant::warning("Mipmap::acquire # file '%s' not found", filename.toUtf8().data());
      return std::shared_ptr<Mipmap>();
    }

    std::shared_ptr<Mipmap> mipmap;

    {
      Radiant::Guard g(s_mipmapStoreMutex);
      auto & weak = s_mipmapStore[key];
      mipmap = weak.lock();
      if(!mipmap) {
        mipmap.reset(new Mipmap(fi.absoluteFilePath()));
        mipmap->startLoading(compressedMipmaps);
        weak = mipmap;
      }
    }

    return mipmap;
  }

  bool Mipmap::reload(const QString & filename)
  {
    if (filename.isEmpty())
      return false;

    const QByteArray path = QFileInfo(filename).absoluteFilePath().toUtf8();
    if (path.isEmpty())
      return false;

    const QPair<QByteArray, bool> keys[] = {qMakePair(path, true),
                                            qMakePair(path, false)};

    std::vector<std::weak_ptr<Mipmap>> mipmaps;
    mipmaps.reserve(2);

    {
      Radiant::Guard g(s_mipmapStoreMutex);
      for (auto & key: keys) {
        auto it = s_mipmapStore.find(key);
        if (it != s_mipmapStore.end()) {
          mipmaps.emplace_back(std::move(it->second));
          s_mipmapStore.erase(it);
        }
      }
    }

    bool found = false;
    for (auto & weak: mipmaps) {
      if (auto mipmap = weak.lock()) {
        found = true;
        mipmap->setObsolete();
      }
    }

    return found;
  }

  QString Mipmap::cacheFileName(const QString & src, int level, const QString & suffix)
  {
    QFileInfo fi(src);

    // Compute MD5 from the absolute path
    QCryptographicHash hash(QCryptographicHash::Md5);
    const qint64 s = fi.size();
    uint t = fi.lastModified().toTime_t();
    hash.addData(fi.absoluteFilePath().toUtf8());
    hash.addData(reinterpret_cast<const char*>(&s), sizeof(s));
    hash.addData(reinterpret_cast<const char*>(&t), sizeof(t));

    const QString md5 = hash.result().toHex();

    // Avoid putting all mipmaps into the same folder (because of OS performance)
    const QString prefix = md5.left(2);
    const QString postfix = level < 0 ? QString(".%1").arg(suffix) :
        QString("_level%1.%2").arg(level, 2, 10, QLatin1Char('0')).arg(suffix);

    const QString fullPath = imageCachePath() + QString("/%1/%2%3").arg(prefix).arg(md5).arg(postfix);

    return fullPath;
  }

  static QString s_basePath;
  QString Mipmap::imageCachePath()
  {
    MULTI_ONCE {
      QString basePath = QString("%2/imagecache-%1").arg(s_imageCacheVersion).arg(
            Radiant::PlatformUtils::getModuleUserDataPath("MultiTouch", false));
      if(!QDir().mkpath(basePath) || !QFileInfo(basePath).isWritable()) {
        basePath = QString("%2/cornerstone-imagecache-%1").arg(s_imageCacheVersion).arg(QDir::tempPath());
        QDir().mkpath(basePath);
      }
      s_basePath = basePath;
    }

    return s_basePath;
  }

  void Mipmap::startLoading(bool compressedMipmaps)
  {
    assert(!m_d->m_ping);
    m_d->m_state = Valuable::STATE_LOADING;
    m_d->m_ping = std::make_shared<PingTask>(shared_from_this(), compressedMipmaps);
    Radiant::BGThread::instance()->addTask(m_d->m_ping);
  }

  void Mipmap::setObsolete()
  {
    m_d->m_isObsolete = true;
    Radiant::info("Mipmap reloaded %s", m_d->m_filenameAbs.toUtf8().data());
    eventSend("reloaded");
  }
}
