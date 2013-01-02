#include "Mipmap.hpp"

#include "Luminous/Image.hpp"
#include "Luminous/Texture2.hpp"
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

namespace
{
  typedef std::map<QPair<QByteArray, bool>, std::weak_ptr<Luminous::Mipmap>> MipmapStore;
  MipmapStore s_mipmapStore;
  Radiant::Mutex s_mipmapStoreMutex;

  // after first resize modify the dimensions so that we can resize
  // 5 times with quarterSize
  const unsigned int s_resizes = 5;
  // default save sizes
  const unsigned int s_defaultSaveSize1 = 64;
  const unsigned int s_defaultSaveSize2 = 512;
  const unsigned int s_defaultSaveSize3 = 2048;
  const unsigned int s_smallestImage = 32;
  const Radiant::Priority s_defaultPingPriority = Radiant::Task::PRIORITY_HIGH + 2;

  bool s_dxtSupported = true;

  /// Special time values in ImageTex3::lastUsed
  enum LoadState {
    New,
    Loading,
    LoadError,
    StateCount
  };

  /// Current time, unit is the same as in RenderManager::frameTime.
  /// This file excludes LoadState times.
  /// See also ImageTex3::lastUsed
  inline int frameTime();

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /// One mipmap level, Mipmap has a vector of these. By default objects are
  /// "empty", meaning that the texture is invalid (!isValid()) and images
  /// are null. LoadTasks will load this when needed, and MipmapReleaseTask
  /// will expire these (set to empty state)
  struct ImageTex3
  {
    ImageTex3() {}

    ImageTex3(ImageTex3 && t);
    ImageTex3 & operator=(ImageTex3 && t);

    /// Only one of the image types is defined at once
    std::unique_ptr<Luminous::CompressedImage> cimage;
    std::unique_ptr<Luminous::Image> image;

    Luminous::Texture texture;

    /// Either LoadState enum value, or time when this class was last used.
    /// These need to be in the same atomic int, or alternatively we could do
    /// this nicer by using separate enum and timestamp, but this way we have
    /// fast lockless synchronization between all threads
    QAtomicInt lastUsed;

    /// During expiration, this will be 1. If you are doing something with this
    /// ImageTex3 without updating lastUsed, you can lock the ImageTex3 from
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

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ImageTex3::ImageTex3(ImageTex3 && t)
    : image(std::move(t.image))
    , texture(std::move(t.texture))
    , lastUsed(t.lastUsed)
  {
  }

  ImageTex3 & ImageTex3::operator=(ImageTex3 && t)
  {
    image = std::move(t.image);
    texture = std::move(t.texture);
    lastUsed = t.lastUsed;
    return *this;
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace Luminous
{
  /// Loads uncompressed mipmaps from file to ImageTex3 / create them if necessary
  class LoadImageTask : public Radiant::Task
  {
  public:
    LoadImageTask(Luminous::MipmapPtr mipmap, Radiant::Priority priority,
                  const QString & filename, int level);

  protected:
    virtual void doTask() OVERRIDE;
    void mipmapReady();

  private:
    bool recursiveLoad(int level);
    bool recursiveLoad(ImageTex3 & imageTex, int level);
    void lock(int);
    void unlock(int);
    virtual void cancel() { Radiant::Task::cancel(); m_mipmap->cancelLoading(); }

  protected:
    Luminous::MipmapPtr m_mipmap;
    const QString & m_filename;
    int m_level;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  /// Loads existing compressed mipmaps from file to ImageTex3
  class LoadCompressedImageTask : public LoadImageTask
  {
  public:
    LoadCompressedImageTask(Luminous::MipmapPtr mipmap, ImageTex3 & tex,
                            Radiant::Priority priority, const QString & filename, int level);

  protected:
    virtual void doTask() OVERRIDE;

  private:
    ImageTex3 & m_tex;
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
    virtual void cancel() OVERRIDE;

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
  class MipmapReleaseTask : public Radiant::Task
  {
  public:
    MipmapReleaseTask();

  protected:
    virtual void doTask() OVERRIDE;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class Mipmap::D
  {
  public:
    D(Mipmap & mipmap, const QString & filenameAbs);
    ~D();

  public:
    Mipmap & m_mipmap;

    const QString m_filenameAbs;
    Nimble::Vector2i m_nativeSize;
    Nimble::Vector2i m_level1Size;
    int m_maxLevel;

    // what levels should be saved to file
    std::set<int> m_shouldSave;

    QDateTime m_fileModified;

    QString m_compressedMipmapFile;
    bool m_useCompressedMipmaps;
    Radiant::Priority m_loadingPriority;

    ImageInfo m_sourceInfo;
    ImageInfo m_compressedMipmapInfo;

    std::shared_ptr<PingTask> m_ping;
    std::shared_ptr<MipMapGenerator> m_mipmapGenerator;

    Radiant::Mutex m_headerReadyCallbacksMutex;
    QList<QPair<std::function<void(void)>, ListenerType>> m_headerReadyCallbacks;
    QList<QPair<std::function<void(void)>, ListenerType>> m_headerReadyOnceCallbacks;
    int m_hasHeaderReadyListener;

    QString m_mipmapFormat;

    std::vector<ImageTex3> m_levels;

    float m_expireSeconds;

    Valuable::AttributeBool m_headerReady;
    Valuable::AttributeBool m_ready;
    bool m_valid;
    bool m_cancelled;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  LoadImageTask::LoadImageTask(Luminous::MipmapPtr mipmap, Radiant::Priority priority,
                               const QString & filename, int level)
    : Task(priority)
    , m_mipmap(mipmap)
    , m_filename(filename)
    , m_level(level)
  {}

  void LoadImageTask::doTask()
  {
    lock(m_level);
    recursiveLoad(m_level);
    mipmapReady();
    unlock(m_level);
    setFinished();
  }

  void LoadImageTask::mipmapReady()
  {
    assert(m_mipmap->m_d->m_headerReady);
    m_mipmap->m_d->m_ready = true;
  }

  void LoadImageTask::lock(int level)
  {
    ImageTex3 & imageTex = m_mipmap->m_d->m_levels[level];

    /// If this fails, then another task is just creating a mipmap for this level,
    /// or MipmapReleaseTask is releasing it
    int i = 0;
    while (!imageTex.locked.testAndSetOrdered(0, 1))
      Radiant::Sleep::sleepMs(std::min(20, ++i));
  }

  void LoadImageTask::unlock(int level)
  {
    ImageTex3 & imageTex = m_mipmap->m_d->m_levels[level];
    imageTex.locked = 0;
  }

  bool LoadImageTask::recursiveLoad(int level)
  {
    ImageTex3 & imageTex = m_mipmap->m_d->m_levels[level];

    int lastUsed = imageTex.lastUsed;
    if (lastUsed == LoadError) {
      return false;
    }

    if (lastUsed >= StateCount) {
      return true;
    }

    bool ok = recursiveLoad(imageTex, level);
    if (ok) {
      /// @todo use Image::texture
      imageTex.texture.setData(imageTex.image->width(), imageTex.image->height(),
                               imageTex.image->pixelFormat(), imageTex.image->data());
      imageTex.texture.setLineSizePixels(0);
      imageTex.lastUsed = frameTime();
    } else {
      imageTex.image.reset();
      imageTex.lastUsed = LoadError;
    }
    return ok;
  }

  bool LoadImageTask::recursiveLoad(ImageTex3 & imageTex, int level)
  {
    if (level == 0) {
      // Load original
      if (!imageTex.image)
        imageTex.image.reset(new Image());

      if (!imageTex.image->read(m_filename.toUtf8().data())) {
        Radiant::error("LoadImageTask::recursiveLoad # Could not read %s", m_filename.toUtf8().data());
        return false;
      } else {
        return true;
      }
    }

    // Could the mipmap be already saved on disk?
    if (m_mipmap->m_d->m_shouldSave.find(level) != m_mipmap->m_d->m_shouldSave.end()) {

      // Try loading a pre-generated smaller-scale mipmap
      const QString filename = Mipmap::cacheFileName(m_filename, level);

      const Radiant::TimeStamp origTs = Radiant::FileUtils::lastModified(m_filename);
      if (origTs > Radiant::TimeStamp(0) && Radiant::FileUtils::fileReadable(filename) &&
          Radiant::FileUtils::lastModified(filename) > origTs) {

        if (!imageTex.image)
          imageTex.image.reset(new Image());

        if (!imageTex.image->read(filename.toUtf8().data())) {
          Radiant::error("LoadImageTask::recursiveLoad # Could not read %s", filename.toUtf8().data());
        } else if (m_mipmap->mipmapSize(level) != imageTex.image->size()) {
          // unexpected size (corrupted or just old image)
          Radiant::error("LoadImageTask::recursiveLoad # Cache image '%s'' size was (%d, %d), expected (%d, %d)",
                filename.toUtf8().data(), imageTex.image->width(), imageTex.image->height(),
                         m_mipmap->mipmapSize(level).x, m_mipmap->mipmapSize(level).y);
        } else {
          return true;
        }
      }
    }


    {
      lock(level - 1);

      // Load the bigger image from lower level, and scale down from that:
      if (!recursiveLoad(level - 1)) {
        unlock(level - 1);
        return false;
      }

      Image & imsrc = *m_mipmap->m_d->m_levels[level - 1].image;

      // Scale down from bigger mipmap
      if (!imageTex.image)
        imageTex.image.reset(new Image());

      Nimble::Vector2i ss = imsrc.size();
      Nimble::Vector2i is = m_mipmap->mipmapSize(level);

      if (is * 2 == ss) {
        if (!imageTex.image->quarterSize(imsrc)) {
          Radiant::error("LoadImageTask::recursiveLoad # failed to resize image");
          unlock(level - 1);
          return false;
        }
      } else {
        imageTex.image->minify(imsrc, is.x, is.y);
      }
      unlock(level - 1);
    }

    if (m_mipmap->m_d->m_shouldSave.find(level) != m_mipmap->m_d->m_shouldSave.end()) {
      const QString filename = Mipmap::cacheFileName(m_filename, level);
      QDir().mkpath(Radiant::FileUtils::path(filename));
      imageTex.image->write(filename.toUtf8().data());
    }

    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  LoadCompressedImageTask::LoadCompressedImageTask(
      Luminous::MipmapPtr mipmap, ImageTex3 & tex, Radiant::Priority priority,
      const QString & filename, int level)
    : LoadImageTask(mipmap, priority, filename, level)
    , m_tex(tex)
  {}

  void LoadCompressedImageTask::doTask()
  {
    std::unique_ptr<Luminous::CompressedImage> im(new Luminous::CompressedImage);
    if(!im->read(m_filename, m_level)) {
      Radiant::error("LoadCompressedImageTask::doTask # Could not read %s level %d", m_filename.toUtf8().data(), m_level);
    } else {
      m_tex.texture.setData(im->width(), im->height(), im->compression(), im->data());
      m_tex.cimage = std::move(im);
      int now = frameTime();
      m_tex.lastUsed.testAndSetOrdered(Loading, now);
      mipmapReady();
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

  // Mipmap guarantees that m_mipmap wont get deleted during doTask()
  void PingTask::doTask()
  {
    setFinished();
    auto mipmap = m_mipmap.lock();
    assert(mipmap);

    if(!m_users.tryAcquire()) {
      // The only explanation for this is that Mipmap already called finishAndWait()

      // BGThread keeps one copy of shared_ptr to this alive during doTask(),
      // so we can manually remove this from Mipmap::D
      mipmap->m_d->m_ping.reset();
      return;
    }

    ping(*mipmap->m_d);

    mipmap->m_d->m_ping.reset();
    m_users.release();
  }

  void PingTask::cancel()
  {
    Radiant::Task::cancel();

    auto mipmap = m_mipmap.lock();
    if(mipmap)
      mipmap->cancelLoading();
  }

  bool PingTask::ping(Luminous::Mipmap::D & mipmap)
  {
    QFileInfo fi(mipmap.m_filenameAbs);
    QDateTime lastModified = fi.lastModified();
    mipmap.m_fileModified = lastModified;

    if(!Luminous::Image::ping(mipmap.m_filenameAbs, mipmap.m_sourceInfo)) {
      Radiant::error("PingTask::doPing # failed to query image size for %s",
                     mipmap.m_filenameAbs.toUtf8().data());
      mipmap.m_valid = false;
      mipmap.m_headerReady = true;
      // Images failed to load are considered to be ready
      mipmap.m_ready = true;
      return false;
    }

    if(!s_dxtSupported && mipmap.m_sourceInfo.pf.compression() != Luminous::PixelFormat::COMPRESSION_NONE) {
      Radiant::error("PingTask::doPing # Image %s has unsupported format",
                     mipmap.m_filenameAbs.toUtf8().data());
      mipmap.m_valid = false;
      mipmap.m_headerReady = true;
      mipmap.m_ready = true;
      return false;
    }

    mipmap.m_nativeSize.make(mipmap.m_sourceInfo.width, mipmap.m_sourceInfo.height);
    mipmap.m_level1Size = mipmap.m_nativeSize / 2;
    mipmap.m_maxLevel = 0;
    for(int s = mipmap.m_nativeSize.maximum(); s > 4; s >>= 1)
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
        auto ptr = mipmap.m_mipmap.shared_from_this();
        mipmap.m_mipmapGenerator->setListener([=] (const ImageInfo & imginfo) { ptr->mipmapReady(imginfo); } );
      }
    }
    else
#endif // LUMINOUS_OPENGLES
    if(mipmap.m_sourceInfo.pf.compression() == Luminous::PixelFormat::COMPRESSION_NONE) {
      // Make sure that we can make "s_resizes" amount of resizes with quarterSize
      // after first resize
      const int mask = (1 << s_resizes) - 1;
      mipmap.m_level1Size.x += ((~(mipmap.m_level1Size.x & mask) & mask) + 1) & mask;
      mipmap.m_level1Size.y += ((~(mipmap.m_level1Size.y & mask) & mask) + 1) & mask;

      // m_maxLevel, m_firstLevelSize and m_nativeSize have to be set before running level()
      mipmap.m_maxLevel = mipmap.m_mipmap.level(Nimble::Vector2f(s_smallestImage, s_smallestImage));

/*      for (int i = 1; i <= m_mipmap.m_mipmap.level(Nimble::Vector2f(s_smallestImage, s_smallestImage)); ++i)
        m_mipmap.m_shouldSave.insert(i);*/

      mipmap.m_shouldSave.insert(mipmap.m_mipmap.level(Nimble::Vector2f(s_smallestImage, s_smallestImage)));
      mipmap.m_shouldSave.insert(mipmap.m_mipmap.level(Nimble::Vector2f(s_defaultSaveSize1, s_defaultSaveSize1)));
      mipmap.m_shouldSave.insert(mipmap.m_mipmap.level(Nimble::Vector2f(s_defaultSaveSize2, s_defaultSaveSize2)));
      mipmap.m_shouldSave.insert(mipmap.m_mipmap.level(Nimble::Vector2f(s_defaultSaveSize3, s_defaultSaveSize3)));
      // Don't save the original image as mipmap
      mipmap.m_shouldSave.erase(0);
    }

    mipmap.m_levels.resize(mipmap.m_maxLevel+1);
    // Send the event "header-ready"-event
    mipmap.m_valid = true;
    mipmap.m_headerReady = true;

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
  {
    scheduleFromNowSecs(5.0);
  }

  void MipmapReleaseTask::doTask()
  {
    Radiant::Guard g(s_mipmapStoreMutex);
    const int now = frameTime();
    for(MipmapStore::iterator it = s_mipmapStore.begin(); it != s_mipmapStore.end();) {
      Luminous::MipmapPtr ptr = it->second.lock();
      if(ptr) {
        if(ptr->isHeaderReady()) {
          const int expire = ptr->m_d->m_expireSeconds * 10;
          std::vector<ImageTex3> & levels = ptr->m_d->m_levels;
          // do not expire the last mipmap level (smallest image)
          for(int level = 0, s = levels.size() - 1; level < s; ++level) {
            ImageTex3 & imageTex = levels[level];
            int lastUsed = imageTex.lastUsed;
            if(lastUsed > Loading && now > lastUsed + expire) {
              if(imageTex.locked.testAndSetOrdered(0, 1)) {
                if(imageTex.lastUsed.testAndSetOrdered(lastUsed, Loading)) {
                  imageTex.texture.reset();
                  imageTex.cimage.reset();
                  imageTex.image.reset();
                  imageTex.lastUsed = New;
                }
                imageTex.locked = 0;
              }
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

    scheduleFromNowSecs(5.0);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  Mipmap::D::D(Mipmap & mipmap, const QString & filenameAbs)
    : m_mipmap(mipmap)
    , m_filenameAbs(filenameAbs)
    , m_nativeSize(0, 0)
    , m_level1Size(0, 0)
    , m_maxLevel(0)
    , m_useCompressedMipmaps(false)
    , m_loadingPriority(Radiant::Task::PRIORITY_NORMAL)
    , m_hasHeaderReadyListener(0)
    , m_mipmapFormat("png")
    , m_expireSeconds(3.0f)
    , m_headerReady(nullptr, "", false)
    , m_ready(nullptr, "", false)
    , m_valid(true)
    , m_cancelled(false)
  {
    MULTI_ONCE { Radiant::BGThread::instance()->addTask(std::make_shared<MipmapReleaseTask>()); }
  }

  Mipmap::D::~D()
  {
    {
      Radiant::Guard g(m_headerReadyCallbacksMutex);
      m_headerReadyCallbacks.clear();
      m_headerReadyOnceCallbacks.clear();
    }
    // Make a local copy, if PingTask is just finishing and removes m_d->m_ping
    std::shared_ptr<PingTask> ping = m_ping;
    if(ping) {
      Radiant::BGThread::instance()->removeTask(ping);
      ping->finishAndWait();
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  Mipmap::Mipmap(const QString & filenameAbs)
    : m_d(new D(*this, filenameAbs))
  {
    eventAddOut("ready");
    eventAddOut("header-ready");
    eventAddOut("cancel-loading");
    m_d->m_headerReady.addListener([&] { if(m_d->m_headerReady) eventSend("header-ready"); });
    m_d->m_ready.addListener([&] { if(m_d->m_ready) eventSend("ready"); });
  }

  Mipmap::~Mipmap()
  {
    delete m_d;
  }

  Texture * Mipmap::texture(unsigned int requestedLevel, unsigned int * returnedLevel, int priorityChange)
  {
    if(!isHeaderReady()) {
      if(!isValid())
        return nullptr;

      if(priorityChange > 0) {
        auto ping = m_d->m_ping;
        auto gen = m_d->m_mipmapGenerator;

        int newPriority = s_defaultPingPriority + priorityChange;
        if(ping && newPriority != ping->priority())
          Radiant::BGThread::instance()->reschedule(ping, newPriority);

        if(gen && newPriority != gen->priority())
          Radiant::BGThread::instance()->reschedule(gen, newPriority);
      }
      return nullptr;
    }
    if(!isValid())
      return nullptr;

    int time = frameTime();

    int req = std::min<int>(requestedLevel, m_d->m_maxLevel);
    for(int level = req, diff = -1; level <= m_d->m_maxLevel; level += diff) {
      if(level < 0) {
        level = req;
        diff = 1;
      } else {
        ImageTex3 & imageTex = m_d->m_levels[level];

        int now = time;
        int old = imageTex.lastUsed;
        while(true) {
          if(now == old) {
            if(returnedLevel)
              *returnedLevel = level;
            return &imageTex.texture;
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
              if(m_d->m_useCompressedMipmaps) {
                Radiant::BGThread::instance()->addTask(std::make_shared<LoadCompressedImageTask>(
                                                shared_from_this(), imageTex,
                                                m_d->m_loadingPriority + priorityChange,
                                                m_d->m_compressedMipmapFile, level));
              } else if(m_d->m_sourceInfo.pf.compression() != PixelFormat::COMPRESSION_NONE) {
                Radiant::BGThread::instance()->addTask(std::make_shared<LoadCompressedImageTask>(
                                                shared_from_this(), imageTex,
                                                m_d->m_loadingPriority + priorityChange,
                                                m_d->m_filenameAbs, level));
              } else {
                Radiant::BGThread::instance()->addTask(std::make_shared<LoadImageTask>(
                                                shared_from_this(),
                                                m_d->m_loadingPriority + priorityChange,
                                                m_d->m_filenameAbs, level));
              }
              break;
            }
            if(returnedLevel)
              *returnedLevel = level;
            return &imageTex.texture;
          } else {
            old = imageTex.lastUsed;
          }
        }
      }
    }

    return nullptr;
  }

  unsigned int Mipmap::level(const Nimble::Matrix4 & transform, Nimble::Vector2f pixelSize,
                             float * trilinearBlending) const
  {
    // Assume that the view matrix is ortho projection with no scaling
    // we can ignore Z and just look X/Y vector projections to determine the maximum scaling
    float sx = Nimble::Vector2f(transform[0][0], transform[0][1]).length();
    float sy = Nimble::Vector2f(transform[1][0], transform[1][1]).length();
    return level(std::max(sx, sy) * pixelSize, trilinearBlending);
  }

  void Mipmap::onHeaderReady(std::function<void(void)> callback, bool once, ListenerType type)
  {
    std::weak_ptr<Mipmap> weak = shared_from_this();

    Radiant::Guard g(m_d->m_headerReadyCallbacksMutex);

    if((m_d->m_hasHeaderReadyListener & type) == 0) {
      m_d->m_hasHeaderReadyListener |= type;
      eventAddListener("header-ready", [=] {
        //This might get called after dtor
        std::shared_ptr<Mipmap> ptr = weak.lock();
        if(!ptr) return;
        assert(ptr->isHeaderReady());
        Radiant::Guard g(ptr->m_d->m_headerReadyCallbacksMutex);
        for (auto p : ptr->m_d->m_headerReadyCallbacks)
          if (p.second == type)
            p.first();
        for (auto it = ptr->m_d->m_headerReadyOnceCallbacks.begin(); it != ptr->m_d->m_headerReadyOnceCallbacks.end();) {
          if (it->second == type) {
            it->first();
            it = ptr->m_d->m_headerReadyOnceCallbacks.erase(it);
          } else ++it;
        }
      }, type);
    }

    if(isHeaderReady()) {
      callback();
    } else if(once) {
      m_d->m_headerReadyOnceCallbacks << qMakePair(callback, type);
    }
    if(!once) m_d->m_headerReadyCallbacks << qMakePair(callback, type);
  }

  unsigned int Mipmap::level(Nimble::Vector2f pixelSize, float * trilinearBlending) const
  {
    const float ask = pixelSize.maximum();

    // Dimension of the first mipmap level (quarter-size from original)
    const float first = m_d->m_level1Size.maximum();

    // The size of mipmap level 0 might be anything between (level1, level1*2)
    // need to handle that as a special case
    if(ask >= first) {
      const float native = m_d->m_nativeSize.maximum();
      if(trilinearBlending)
        *trilinearBlending = std::max(0.0f, 1.0f - (ask - first) / (native - first));
      return 0;
    }

    // if the size is really small, the calculation later does funny things
    if(ask <= (int(first) >> m_d->m_maxLevel)) {
      if(trilinearBlending)
        *trilinearBlending = 0;
      return m_d->m_maxLevel;
    }

    float blending = log(ask / first) / log(0.5);
    int bestlevel = blending;
    blending -= bestlevel;
    bestlevel++;

    if(bestlevel > m_d->m_maxLevel) {
      bestlevel = m_d->m_maxLevel;
      if(trilinearBlending)
        *trilinearBlending = 0.0f;
    } else if(trilinearBlending) {
      *trilinearBlending = blending;
    }

    assert(bestlevel >= 0 && bestlevel <= m_d->m_maxLevel);

    return bestlevel;
  }

  const Nimble::Vector2i & Mipmap::nativeSize() const
  {
    return m_d->m_nativeSize;
  }

  float Mipmap::aspect() const
  {
    Nimble::Vector2i native = m_d->m_nativeSize;

    return native.y ? native.x / (float) native.y : 1.0f;
  }

  bool Mipmap::isReady(bool) const
  {
    return m_d->m_ready;
  }

  bool Mipmap::isHeaderReady() const
  {
    return m_d->m_headerReady;
  }

  bool Mipmap::isValid() const
  {
    return m_d->m_valid;
  }

  bool Mipmap::hasAlpha() const
  {
    return m_d->m_sourceInfo.pf.hasAlpha();
  }

  bool Mipmap::isLoadCancelled() const
  {
    return m_d->m_cancelled;
  }

  float Mipmap::pixelAlpha(Nimble::Vector2 relLoc)
  {
    if(!isHeaderReady() || !isValid()) return 1.0f;

    int time = frameTime();
    for(int level = 0; level <= m_d->m_maxLevel; ) {
      ImageTex3 & imageTex = m_d->m_levels[level];
      int old = imageTex.lastUsed;
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

  Nimble::Vector2i Mipmap::mipmapSize(unsigned int level)
  {
    if(level == 0) return m_d->m_nativeSize;
    if(level <= s_resizes+1) {
      return Nimble::Vector2i(m_d->m_level1Size.x >> (level-1),
                              m_d->m_level1Size.y >> (level-1));
    } else {
      Nimble::Vector2i v(m_d->m_level1Size.x >> s_resizes,
                         m_d->m_level1Size.y >> s_resizes);
      level -= s_resizes+1;
      while(level-- > 0) {
        v = v / 2;
        if (v.x == 0 || v.y == 0)
          return Nimble::Vector2i(0, 0);
      }
      return v;
    }
  }

  const QString & Mipmap::filename() const
  {
    return m_d->m_filenameAbs;
  }

  void Mipmap::mipmapReady(const ImageInfo & imginfo)
  {
    m_d->m_compressedMipmapInfo = imginfo;

    m_d->m_mipmapGenerator.reset();
    m_d->m_ready = true;
    // preload the maximum level mipmap image
    texture(m_d->m_maxLevel);
  }

  std::shared_ptr<Mipmap> Mipmap::acquire(const QString & filename,
                                          bool compressedMipmaps)
  {
    QFileInfo fi(filename);
    QPair<QByteArray, bool> key = qMakePair(fi.absoluteFilePath().toUtf8(), compressedMipmaps);

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

  QString Mipmap::cacheFileName(const QString & src, int level, const QString & suffix)
  {
    QFileInfo fi(src);

    static QString s_basePath;

    MULTI_ONCE {
      QString basePath = Radiant::PlatformUtils::getModuleUserDataPath("MultiTouch", false) + "/imagecache";
      if(!QDir().mkpath(basePath)) {
        basePath = QDir::tempPath() + "/cornerstone-imagecache";
        QDir().mkpath(basePath);
      }
      s_basePath = basePath;
    }

    // Compute MD5 from the absolute path
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(fi.absoluteFilePath().toUtf8());

    const QString md5 = hash.result().toHex();

    // Avoid putting all mipmaps into the same folder (because of OS performance)
    const QString prefix = md5.left(2);
    const QString postfix = level < 0 ? QString(".%1").arg(suffix) :
        QString("_level%1.%2").arg(level, 2, 10, QLatin1Char('0')).arg(suffix);

    const QString fullPath = s_basePath + QString("/%1/%2%3").arg(prefix).arg(md5).arg(postfix);

    return fullPath;
  }

  bool Mipmap::startLoading(bool compressedMipmaps)
  {
    m_d->m_cancelled = false;
    assert(!m_d->m_ping);
    m_d->m_ping = std::make_shared<PingTask>(shared_from_this(), compressedMipmaps);
    Radiant::BGThread::instance()->addTask(m_d->m_ping);

    return (!m_d->m_ping || m_d->m_ping->state() != Radiant::Task::CANCELLED);
  }

  void Mipmap::cancelLoading()
  {
    m_d->m_cancelled = true;
    eventSend("cancel-loading");
  }
}
