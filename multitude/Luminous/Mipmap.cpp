#include "Mipmap.hpp"

#include "Luminous/Image.hpp"
#include "Luminous/Texture2.hpp"
#include "Luminous/BGThread.hpp"
#include "Luminous/MipMapGenerator.hpp"

#include <Radiant/PlatformUtils.hpp>

#include <QFileInfo>
#include <QCryptographicHash>
#include <QDir>
#include <QSemaphore>
#include <QSettings>
#include <QDateTime>

namespace
{
  enum {
    New = 0,
    Loading = 1
  };

  struct ImageTex3
  {
    ImageTex3() : image(nullptr) {}

    ImageTex3(ImageTex3 && t);
    ImageTex3 & operator=(ImageTex3 && t);

    std::unique_ptr<Luminous::CompressedImage> cimage;
    std::unique_ptr<Luminous::Image> image;
    Luminous::Texture texture;
    QAtomicInt lastUsed;
  };

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

  class LoadImageTask : public Luminous::Task
  {
  public:
    LoadImageTask(Luminous::MipmapPtr mipmap, ImageTex3 & tex,
                  Luminous::Priority priority, const QString & filename, int level);

  protected:
    virtual void doTask() OVERRIDE;

  protected:
    Luminous::MipmapPtr m_mipmap;
    ImageTex3 & m_tex;
    const QString & m_filename;
    int m_level;
  };

  class LoadCompressedImageTask : public LoadImageTask
  {
  public:
    LoadCompressedImageTask(Luminous::MipmapPtr mipmap, ImageTex3 & tex,
                            Luminous::Priority priority, const QString & filename, int level);

  protected:
    virtual void doTask() OVERRIDE;
  };

  class MipmapReleaseTask : public Luminous::Task
  {
  public:
  };

  LoadImageTask::LoadImageTask(Luminous::MipmapPtr mipmap, ImageTex3 & tex,
                               Luminous::Priority priority, const QString & filename, int level)
    : Task(priority)
    , m_mipmap(mipmap)
    , m_tex(tex)
    , m_filename(filename)
    , m_level(level)
  {}

  LoadCompressedImageTask::LoadCompressedImageTask(
      Luminous::MipmapPtr mipmap, ImageTex3 & tex, Luminous::Priority priority,
      const QString & filename, int level)
    : LoadImageTask(mipmap, tex, priority, filename, level)
  {}

  void LoadImageTask::doTask()
  {
  }

  void LoadCompressedImageTask::doTask()
  {
    std::unique_ptr<Luminous::CompressedImage> im(new Luminous::CompressedImage);
    if(!im->read(m_filename, m_level)) {
      Radiant::error("LoadCompressedImageTask::doTask # Could not read %s level %d", m_filename.toUtf8().data(), m_level);
    } else {
      m_tex.texture.setData(im->width(), im->height(), im->compression(), im->data());
      m_tex.cimage = std::move(im);
      int time = 100; /// @todo
      int now = std::max(2, time);
      m_tex.lastUsed.testAndSetOrdered(Loading, now);
    }
    setFinished();
  }
}

namespace Luminous
{
  class PingTask : public Luminous::Task
  {
  public:
    PingTask(Luminous::Mipmap::D & mipmap, bool compressedMipmaps);

    void finishAndWait();

  protected:
    virtual void doTask() OVERRIDE;

  private:
    bool ping();

  private:
    bool m_preferCompressedMipmaps;
    Mipmap::D & m_mipmap;
    QSemaphore m_users;
  };

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
    Priority m_loadingPriority;

    ImageInfo m_sourceInfo;
    ImageInfo m_compressedMipmapInfo;

    std::shared_ptr<PingTask> m_ping;

    QString m_mipmapFormat;

    std::vector<ImageTex3> m_levels;

    bool m_ready;
  };
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
namespace
{
  // after first resize modify the dimensions so that we can resize
  // 5 times with quarterSize
  const unsigned int s_resizes = 5;
  // default save sizes
  const unsigned int s_defaultSaveSize1 = 64;
  const unsigned int s_defaultSaveSize2 = 512;
  const unsigned int s_smallestImage = 32;

  bool s_dxtSupported = true;

  std::map<QPair<QByteArray, bool>, std::weak_ptr<Luminous::Mipmap>> s_mipmapStore;
  Radiant::Mutex s_mipmapStoreMutex;

  Luminous::ImageInfo fromMap(const QVariantMap & map)
  {
    Luminous::ImageInfo img;
    img.width = map.value("width").toInt();
    img.height = map.value("height").toInt();
    img.mipmaps = map.value("mipmaps").toInt();
    if(map.contains("pf.compression")) {
      img.pf = Luminous::PixelFormat::Compression(map.value("pf.compression").toInt());
    } else {
      img.pf = Luminous::PixelFormat(
            Luminous::PixelFormat::ChannelLayout(map.value("pf.layout").toInt()),
            Luminous::PixelFormat::ChannelType(map.value("pf.type").toInt()));
    }
    return img;
  }

  QVariantMap toMap(const Luminous::ImageInfo & info)
  {
    QVariantMap map;
    map["width"] = info.width;
    map["height"] = info.height;
    map["mipmaps"] = info.mipmaps;
    if(info.pf.compression() != Luminous::PixelFormat::COMPRESSION_NONE) {
      map["pf.compression"] = info.pf.compression();
    } else {
      map["pf.layout"] = info.pf.layout();
      map["pf.type"] = info.pf.type();
    }
    return map;
  }

  Luminous::ImageInfo pingImage(const QString & absFilename, const QDateTime & fileModified)
  {
    if(absFilename.isEmpty()) return Luminous::ImageInfo();

    QSettings settings("MultiTouch", "ImageInfo");
    QVariantMap map = settings.value(absFilename).toMap();

    if(!map.isEmpty()) {
      QDateTime cacheModified = map.value("lastModified").toDateTime();
      if(cacheModified.isValid() && cacheModified >= fileModified)
        return fromMap(map);
    }

    Luminous::ImageInfo info;
    if(Luminous::Image::ping(absFilename, info))
      settings.setValue(absFilename, toMap(info));
    return info;
  }
}

namespace Luminous
{
  PingTask::PingTask(Mipmap::D & mipmap, bool compressedMipmaps)
    : Task(PRIORITY_HIGH+2)
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
    if(!m_users.tryAcquire()) {
      // The only explanation for this is that Mipmap already called finishAndWait()

      // BGThread keeps one copy of shared_ptr to this alive during doTask(),
      // so we can manually remove this from Mipmap::D
      m_mipmap.m_ping.reset();
      return;
    }

    ping();

    m_mipmap.m_ping.reset();
    m_users.release();
  }

  bool PingTask::ping()
  {
    QFileInfo fi(m_mipmap.m_filenameAbs);
    QDateTime lastModified = fi.lastModified();
    m_mipmap.m_fileModified = lastModified;
    m_mipmap.m_sourceInfo = pingImage(m_mipmap.m_filenameAbs, lastModified);

    if(m_mipmap.m_sourceInfo.width <= 0) {
      Radiant::error("PingTask::doPing # failed to query image size for %s",
                     m_mipmap.m_filenameAbs.toUtf8().data());
      return false;
    }

    if(!s_dxtSupported && m_mipmap.m_sourceInfo.pf.compression() != Luminous::PixelFormat::COMPRESSION_NONE) {
      Radiant::error("PingTask::doPing # Image %s has unsupported format",
                     m_mipmap.m_filenameAbs.toUtf8().data());
      return false;
    }

    m_mipmap.m_nativeSize.make(m_mipmap.m_sourceInfo.width, m_mipmap.m_sourceInfo.height);
    m_mipmap.m_level1Size = m_mipmap.m_nativeSize / 2;
    m_mipmap.m_maxLevel = 0;
    for(int s = m_mipmap.m_nativeSize.maximum(); s > 4; s >>= 1)
      ++m_mipmap.m_maxLevel;

    // Use DXT compression if it is requested and supported
    m_mipmap.m_useCompressedMipmaps = m_preferCompressedMipmaps && s_dxtSupported;

#ifndef LUMINOUS_OPENGLES

    if(m_mipmap.m_sourceInfo.pf.compression() && (
         m_mipmap.m_sourceInfo.mipmaps > 1 ||
         (m_mipmap.m_sourceInfo.width < 5 && m_mipmap.m_sourceInfo.height < 5))) {
      // We already have compressed image with mipmaps, no need to generate more
      m_mipmap.m_useCompressedMipmaps = false;
    }

    Luminous::MipMapGenerator * gen = 0;
    if(m_mipmap.m_useCompressedMipmaps) {
      m_mipmap.m_compressedMipmapFile = Luminous::Mipmap::cacheFileName(m_mipmap.m_filenameAbs, -1, "dds");
      QFileInfo compressedMipmap(m_mipmap.m_compressedMipmapFile);
      QDateTime compressedMipmapTs;
      if(compressedMipmap.exists())
        compressedMipmapTs = compressedMipmap.lastModified();

      if(compressedMipmapTs.isValid() && compressedMipmapTs < m_mipmap.m_fileModified) {
        m_mipmap.m_compressedMipmapInfo = pingImage(m_mipmap.m_compressedMipmapFile, compressedMipmapTs);
        if(m_mipmap.m_compressedMipmapInfo.width <= 0)
          compressedMipmapTs = QDateTime();
      }
      if(!compressedMipmapTs.isValid()) {
        gen = new MipMapGenerator(m_mipmap.m_filenameAbs, m_mipmap.m_compressedMipmapFile);
        auto ptr = m_mipmap.m_mipmap.shared_from_this();
        gen->setListener([=] (const ImageInfo & imginfo) { ptr->mipmapReady(imginfo); } );
      }
    }
    else
#endif // LUMINOUS_OPENGLES
    if(m_mipmap.m_sourceInfo.pf.compression() == Luminous::PixelFormat::COMPRESSION_NONE) {
      // Make sure that we can make "s_resizes" amount of resizes with quarterSize
      // after first resize
      const int mask = (1 << s_resizes) - 1;
      m_mipmap.m_level1Size.x += ((~(m_mipmap.m_level1Size.x & mask) & mask) + 1) & mask;
      m_mipmap.m_level1Size.y += ((~(m_mipmap.m_level1Size.y & mask) & mask) + 1) & mask;

      // m_maxLevel, m_firstLevelSize and m_nativeSize have to be set before running level()
      m_mipmap.m_maxLevel = m_mipmap.m_mipmap.level(Nimble::Vector2f(s_smallestImage, s_smallestImage));

      m_mipmap.m_shouldSave.insert(m_mipmap.m_mipmap.level(Nimble::Vector2f(s_smallestImage, s_smallestImage)));
      m_mipmap.m_shouldSave.insert(m_mipmap.m_mipmap.level(Nimble::Vector2f(s_defaultSaveSize1, s_defaultSaveSize1)));
      m_mipmap.m_shouldSave.insert(m_mipmap.m_mipmap.level(Nimble::Vector2f(s_defaultSaveSize2, s_defaultSaveSize2)));
      // Don't save the original image as mipmap
      m_mipmap.m_shouldSave.erase(0);
    }

    m_mipmap.m_levels.resize(m_mipmap.m_maxLevel+1);

#ifndef LUMINOUS_OPENGLES
    if(gen) {
      Luminous::BGThread::instance()->addTask(gen);
    } else
#endif // LUMINOUS_OPENGLES
    {
      m_mipmap.m_ready = true;
      // preload the maximum level mipmap image
      m_mipmap.m_mipmap.texture(m_mipmap.m_maxLevel);
    }

    return true;
  }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace Luminous
{
  Mipmap::D::D(Mipmap & mipmap, const QString & filenameAbs)
    : m_mipmap(mipmap)
    , m_filenameAbs(filenameAbs)
    , m_nativeSize(0, 0)
    , m_level1Size(0, 0)
    , m_maxLevel(0)
    , m_useCompressedMipmaps(false)
    , m_loadingPriority(Task::PRIORITY_NORMAL)
    , m_mipmapFormat("png")
    , m_ready(false)
  {}

  Mipmap::D::~D()
  {
    // Make a local copy, if PingTask is just finishing and removes m_d->m_ping
    std::shared_ptr<PingTask> ping = m_ping;
    if(ping) {
      BGThread::instance()->removeTask(ping);
      ping->finishAndWait();
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  Mipmap::Mipmap(const QString & filenameAbs)
    : m_d(new D(*this, filenameAbs))
  {}

  Mipmap::~Mipmap()
  {
    delete m_d;
  }

  Texture * Mipmap::texture(unsigned int requestedLevel, unsigned int * returnedLevel)
  {
    if(!m_d->m_ready)
      return nullptr;

    /// @todo get this from somewhere
    int time = 100;

    int req = std::min<int>(requestedLevel, m_d->m_maxLevel);
    for(int level = req, diff = -1; level <= m_d->m_maxLevel; level += diff) {
      if(level < 0) {
        level = req;
        diff = 1;
      } else {
        ImageTex3 & imageTex = m_d->m_levels[level];

        int now = std::max(2, time);
        int old = imageTex.lastUsed;
        while(true) {
          if(now == old) {
            if(returnedLevel)
              *returnedLevel = level;
            return &imageTex.texture;
          }

          if(old == Loading)
            break;

          // Only start loading new images if this is the correct level
          if(old == New && level != req)
            break;

          if(old == New)
            now = Loading;

          if(imageTex.lastUsed.testAndSetOrdered(old, now)) {
            if(now == Loading) {
              if(m_d->m_useCompressedMipmaps) {
                BGThread::instance()->addTask(std::make_shared<LoadCompressedImageTask>(
                                                shared_from_this(), imageTex, m_d->m_loadingPriority,
                                                m_d->m_compressedMipmapFile, level));
              } else if(m_d->m_sourceInfo.pf.compression() != PixelFormat::COMPRESSION_NONE) {
                BGThread::instance()->addTask(std::make_shared<LoadCompressedImageTask>(
                                                shared_from_this(), imageTex, m_d->m_loadingPriority,
                                                m_d->m_filenameAbs, level));
              } else {
                BGThread::instance()->addTask(std::make_shared<LoadImageTask>(
                                                shared_from_this(), imageTex, m_d->m_loadingPriority,
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

#if 0
  Image * Mipmap::image(unsigned int level) const
  {
    if(m_d->m_haveCompressedImage)
      return nullptr;
  }

#ifndef LUMINOUS_OPENGLES
  CompressedImage * Mipmap::compressedImage(unsigned int level)
  {
  }
#endif
#endif
  unsigned int Mipmap::level(const Nimble::Matrix4 & transform, Nimble::Vector2f pixelSize,
                             float * trilinearBlending)
  {
    // Assume that the view matrix is ortho projection with no scaling
    // we can ignore Z and just look X/Y vector projections to determine the maximum scaling
    float sx = Nimble::Vector2f(transform[0][0], transform[0][1]).length();
    float sy = Nimble::Vector2f(transform[1][0], transform[1][1]).length();
    return level(std::max(sx, sy) * pixelSize, trilinearBlending);
  }

  unsigned int Mipmap::level(Nimble::Vector2f pixelSize, float * trilinearBlending)
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

    return bestlevel;
  }

  const Nimble::Vector2i & Mipmap::nativeSize() const
  {
    return m_d->m_nativeSize;
  }

  void Mipmap::setLoadingPriority(Priority priority)
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

    MULTI_ONCE_BEGIN
    QString basePath = Radiant::PlatformUtils::getModuleUserDataPath("MultiTouch", false) + "/imagecache";
    if(!QDir().mkpath(basePath)) {
      basePath = QDir::tempPath() + "/cornerstone-imagecache";
      QDir().mkpath(basePath);
    }
    s_basePath = basePath;
    MULTI_ONCE_END

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

  void Mipmap::startLoading(bool compressedMipmaps)
  {
    assert(!m_d->m_ping);
    m_d->m_ping = std::make_shared<PingTask>(*m_d, compressedMipmaps);
    BGThread::instance()->addTask(m_d->m_ping);
  }
}
