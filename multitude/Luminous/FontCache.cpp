/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "FontCache.hpp"
#include "SimpleTextLayout.hpp"

#include <Luminous/Texture.hpp>

#include <Luminous/Image.hpp>
#include <Luminous/DistanceFieldGenerator.hpp>
#include <Luminous/RenderManager.hpp>
#include <Luminous/ImageCodecCS.hpp>

#include <Radiant/BGThread.hpp>
#include <Radiant/PlatformUtils.hpp>
#include <Radiant/CacheManager.hpp>

#include <QRawFont>
#include <QDir>
#include <QPainter>
#include <QPainterPath>
#include <QSettings>
#include <QThread>

#include <cstdlib>

namespace
{
  std::map<QString, std::unique_ptr<Luminous::FontCache>> s_fontCache;
  Radiant::Mutex s_fontCacheMutex;

  static Luminous::PixelFormat s_pixelFormat(Luminous::PixelFormat::LAYOUT_RED,
                                             Luminous::PixelFormat::TYPE_USHORT);
  Luminous::TextureAtlasGroup<Luminous::FontCache::Glyph> s_atlas(s_pixelFormat);
  Radiant::Mutex s_atlasMutex;
  int s_atlasGeneration = 0;

  const int s_distanceFieldPixelSize = 128;
  const float s_padding = 60;
  int s_maxHiresSize = 3072;
  bool s_persistGlyphs = true;

  // space character etc
  static Luminous::FontCache::Glyph s_emptyGlyph;

  QString makeKey(const QRawFont & rawFont, int stretch)
  {
    return QString("%4.%5.%1.%2.%3").arg(rawFont.weight()).arg(stretch).
        arg(rawFont.style()).arg(rawFont.familyName(), rawFont.styleName());
  }

  const QString & cacheBasePath()
  {
    static QString s_basePath;

    MULTI_ONCE s_basePath = Radiant::CacheManager::instance()->createCacheDir("fonts");

    return s_basePath;
  }

  QString cacheFileName(QString fontKey, quint32 glyphIndex)
  {
    const QString path = cacheBasePath() + "/" + fontKey.replace('/', '_');
    QDir().mkdir(path);

    return QString("%1/%2.glyph").arg(path).arg(glyphIndex);
  }

  QString indexFileName()
  {
    return cacheBasePath() + "/index.ini";
  }

  /// for now, we use our own image format hack, since Luminous::Image doesn't support
  /// saving or loading 16 bit grayscale images.
  bool saveImage(const Luminous::Image & image, const QString & filename)
  {
    QSaveFile file(filename);
    if (file.open(QFile::WriteOnly)) {
      Luminous::ImageCodecCS codec;
      bool ok = codec.write(image, file);
      if (ok)
        return file.commit();
      return false;
    } else {
      Radiant::error("saveImage # Failed to open '%s': %s", filename.toUtf8().data(),
                     file.errorString().toUtf8().data());
      return false;
    }
  }

  bool loadImage(Luminous::Image & image, const QString & filename)
  {
    QFile file(filename);
    if (file.open(QFile::ReadOnly)) {
      Luminous::ImageCodecCS codec;
      return codec.read(image, file);
    } else {
      Radiant::error("loadImage # Failed to open '%s': %s", filename.toUtf8().data(),
                     file.errorString().toUtf8().data());
      return false;
    }
  }

  Luminous::FontCache::Glyph * makeGlyph(const Luminous::Image & img)
  {
    Luminous::FontCache::Glyph * glyph;
    {
      Radiant::Guard g(s_atlasMutex);
      glyph = &s_atlas.insert(img.size());
    }

    Luminous::Image & target = glyph->m_atlas->image();
    for (int y = 0; y < img.height(); ++y) {
      const unsigned char * from = img.line(y);
      if (glyph->m_node->m_rotated) {
        const float toFloat = 1.0f / ((1l << (target.pixelFormat().bytesPerPixel() * 8)) - 1);
        for (int x = 0; x < img.width(); ++x) {
          target.setPixel(glyph->m_node->m_location.x+y, glyph->m_node->m_location.y+x,
                          Nimble::Vector4f(from[x] * toFloat, 0, 0, 0));
        }
      } else {
        unsigned char * to = target.line(glyph->m_node->m_location.y+y) + glyph->m_node->m_location.x * target.pixelFormat().bytesPerPixel();
        std::copy(from, from + img.width() * target.pixelFormat().bytesPerPixel(), to);
      }
    }

    Luminous::Texture & texture = glyph->m_atlas->texture();

    Luminous::RenderResource::Id texId = texture.resourceId();
    QRect rect(glyph->m_node->m_location.x, glyph->m_node->m_location.y,
               glyph->m_node->m_size.width(), glyph->m_node->m_size.height());

    Valuable::Node::invokeAfterUpdate([texId, rect] {
      Luminous::Texture * tex = Luminous::RenderManager::getResource<Luminous::Texture>(texId);
      if (tex)
        tex->addDirtyRect(rect);
    });

    return glyph;
  }
}

namespace Luminous
{
  class FontCache::GlyphGenerator : public Radiant::Task
  {
  public:
    GlyphGenerator(FontCache::D & cache);

  protected:
    virtual void doTask() OVERRIDE;

  private:
    Glyph * generateGlyph(quint32 glyphIndex, QPainterPath path);

  private:
    FontCache::D & m_cache;
    Luminous::Image m_src;
    // This is the backend used by QPainter, created lazily
    std::unique_ptr<QImage> m_painterImg;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class FontCache::FileCacheIndexLoader : public Radiant::Task
  {
  public:
    FileCacheIndexLoader(FontCache::D & cache);

  protected:
    virtual void doTask() OVERRIDE;

  private:
    FontCache::D & m_cache;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class FontCache::FileCacheLoader : public Radiant::Task
  {
  public:
    FileCacheLoader(FontCache::D & cache);

  protected:
    virtual void doTask() OVERRIDE;

  private:
    FontCache::D & m_cache;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class FontCache::D
  {
  public:
    D(const QRawFont & rawFont, int stretch);

  public:
    struct FileCacheItem
    {
      FileCacheItem() {}
      FileCacheItem(const QString & src, const QRectF & rect)
        : src(src), rect(rect) {}

      /// Filename (our own format)
      QString src;
      /// Glyph::m_location, Glyph::m_size
      QRectF rect;
    };

  public:
    const QString m_rawFontKey;

    /// This locks m_cache, m_fileCache
    Radiant::Mutex m_cacheMutex;

    std::map<quint32, Glyph*> m_cache;

    std::unique_ptr<std::map<quint32, FileCacheItem> > m_fileCacheIndex;
    std::shared_ptr<FileCacheIndexLoader> m_fileCacheIndexLoader;

    std::list<std::pair<quint32, FileCacheItem> > m_fileCacheRequests;
    std::shared_ptr<FileCacheLoader> m_fileCacheLoader;

    std::list<std::pair<quint32, QPainterPath> > m_glyphGenerationRequests;
    std::shared_ptr<GlyphGenerator> m_glyphGenerator;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  FontCache::GlyphGenerator::GlyphGenerator(FontCache::D & cache)
    : Radiant::Task(PRIORITY_HIGH)
    , m_cache(cache)
  {
  }

  void FontCache::GlyphGenerator::doTask()
  {
    std::pair<quint32, QPainterPath> p;
    bool ok = false;
    {
      Radiant::Guard g(m_cache.m_cacheMutex);
      if (m_cache.m_glyphGenerationRequests.empty()) {
        m_cache.m_glyphGenerator.reset();
      } else {
        p = m_cache.m_glyphGenerationRequests.front();
        m_cache.m_glyphGenerationRequests.pop_front();
        ok = true;
      }
    }

    if (ok) {
      FontCache::Glyph * glyph = generateGlyph(p.first, p.second);
      assert(glyph);
      Radiant::Guard g(m_cache.m_cacheMutex);
      m_cache.m_cache[p.first] = glyph;
    } else {
      // delete this in this thread
      m_painterImg.reset();

      setFinished();
    }
  }

  FontCache::Glyph * FontCache::GlyphGenerator::generateGlyph(quint32 glyphIndex, QPainterPath path)
  {
    if (path.isEmpty()) {
      /// @todo sometimes glyph generation seems to fail, do not ever cache empty glyphs
      /*QSettings settings(indexFileName(), QSettings::IniFormat);

      settings.beginGroup(m_cache.m_rawFontKey);
      settings.beginGroup(QString::number(glyphIndex));
      settings.setValue("rect", QRectF());
      settings.endGroup();
      settings.endGroup();*/

      return &s_emptyGlyph;
    }

    const Nimble::Rectf br = path.boundingRect();

    const float glyphSize = std::max(br.width(), br.height());
    const float distanceFieldSize = glyphSize + 2.0f * s_padding;
    const float hiresSize = std::min((float)s_maxHiresSize, s_maxHiresSize * glyphSize / s_distanceFieldPixelSize);
    const float hiresPadding = hiresSize * s_padding / distanceFieldSize;
    const float hiresFactor = hiresSize / distanceFieldSize;

    const float hiresContentSize = hiresSize - hiresPadding * 2.0f;
    const float hiresContentScale = hiresContentSize / glyphSize;

    const Nimble::Vector2f translate(hiresPadding - br.low().x * hiresContentScale,
                                     hiresPadding - br.low().y * hiresContentScale);

    const Nimble::Vector2i sdfSize(Nimble::Math::Round(br.width() + 2.0f * s_padding),
                                   Nimble::Math::Round(br.height() + 2.0f * s_padding));

    const Nimble::Vector2i srcSize(Nimble::Math::Round((br.width() + 2.0f * s_padding) * hiresFactor),
                                   Nimble::Math::Round((br.height() + 2.0f * s_padding) * hiresFactor));

    // Scale & transform the path to fill image of the size (hiresSize x hiresSize)
    // while keeping the correct aspect ratio and having hiresPadding on every edge.
    // Also move the path to origin
    for (int i = 0; i < path.elementCount(); ++i) {
      const QPainterPath::Element & e = path.elementAt(i);
      path.setElementPositionAt(i, e.x * hiresContentScale + translate.x, e.y * hiresContentScale + translate.y);
    }

    if (!m_painterImg)
      m_painterImg.reset(new QImage(s_maxHiresSize, s_maxHiresSize, QImage::Format_ARGB32_Premultiplied));

    if (m_src.width() != s_maxHiresSize)
      m_src.allocate(s_maxHiresSize, s_maxHiresSize, Luminous::PixelFormat::alphaUByte());

    QImage & img = *m_painterImg;
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(Qt::black));

    img.fill(Qt::transparent);
    painter.drawPath(path);

    /*static QAtomicInt id;
    img.save(QString("glyph-bg-%1.png").arg(id.fetchAndAddOrdered(1)));*/

    for (int y = 0; y < s_maxHiresSize; ++y) {
      const QRgb * from = reinterpret_cast<const QRgb*>(img.constScanLine(y));
      unsigned char * to = m_src.line(y);
      for (int x = 0; x < s_maxHiresSize; ++x)
        to[x] = qAlpha(from[x]);
    }

    Image sdf;
    sdf.allocate(sdfSize.x, sdfSize.y, s_pixelFormat);
    DistanceFieldGenerator::generate(m_src, srcSize, sdf, static_cast<int>(hiresPadding));

    FontCache::Glyph * glyph = makeGlyph(sdf);
    glyph->setSize(Nimble::Vector2f(2.0f * s_padding + br.width(),
                                    2.0f * s_padding + br.height()));
    glyph->setLocation(Nimble::Vector2f(br.low().x - s_padding,
                                        br.low().y - s_padding));

    if(!s_persistGlyphs)
      return glyph;

    const QString file = cacheFileName(m_cache.m_rawFontKey, glyphIndex);

    if (saveImage(sdf, file)) {
      FontCache::D::FileCacheItem item(file, QRectF(glyph->location().x, glyph->location().y,
                                       glyph->size().x, glyph->size().y));
      {
        Radiant::Guard g(m_cache.m_cacheMutex);
        (*m_cache.m_fileCacheIndex)[glyphIndex] = item;
      }

      QSettings settings(indexFileName(), QSettings::IniFormat);

      settings.beginGroup(m_cache.m_rawFontKey);
      settings.beginGroup(QString::number(glyphIndex));
      settings.setValue("rect", item.rect);
      settings.setValue("src", file);
      settings.endGroup();
      settings.endGroup();
    }

    return glyph;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  FontCache::FileCacheIndexLoader::FileCacheIndexLoader(FontCache::D & cache)
    : Radiant::Task(PRIORITY_HIGH+1),
      m_cache(cache)
  {}

  void FontCache::FileCacheIndexLoader::doTask()
  {
    std::unique_ptr<std::map<quint32, FontCache::D::FileCacheItem> > fileCacheIndex(
          new std::map<quint32, FontCache::D::FileCacheItem>());

    QSettings settings(indexFileName(), QSettings::IniFormat);

    settings.beginGroup(m_cache.m_rawFontKey);

    for (const QString & index : settings.childGroups()) {
      settings.beginGroup(index);
      const quint32 glyphIndex = index.toUInt();
      const QRectF r = settings.value("rect").toRectF();
      const QString src = settings.value("src").toString();
      fileCacheIndex->insert(std::make_pair(glyphIndex, FontCache::D::FileCacheItem(src, r)));
      settings.endGroup();
    }

    settings.endGroup();

    {
      Radiant::Guard g(m_cache.m_cacheMutex);
      m_cache.m_fileCacheIndexLoader = nullptr;
      m_cache.m_fileCacheIndex = std::move(fileCacheIndex);
    }
    setFinished();
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  FontCache::FileCacheLoader::FileCacheLoader(FontCache::D & cache)
    : Radiant::Task(PRIORITY_HIGH+1),
      m_cache(cache)
  {}

  void FontCache::FileCacheLoader::doTask()
  {
    std::pair<quint32, FontCache::D::FileCacheItem> p;
    bool ok = false;
    {
      Radiant::Guard g(m_cache.m_cacheMutex);
      if (m_cache.m_fileCacheRequests.empty()) {
        m_cache.m_fileCacheLoader.reset();
      } else {
        p = m_cache.m_fileCacheRequests.front();
        m_cache.m_fileCacheRequests.pop_front();
        ok = true;
      }
    }

    if (ok) {
      Luminous::Image img;
      if (loadImage(img, p.second.src)) {
        FontCache::Glyph * glyph = makeGlyph(img);
        assert(glyph);
        glyph->setLocation(Nimble::Vector2d(p.second.rect.left(), p.second.rect.top()).cast<float>());
        glyph->setSize(Nimble::Vector2d(p.second.rect.width(), p.second.rect.height()).cast<float>());

        Radiant::Guard g(m_cache.m_cacheMutex);
        m_cache.m_cache[p.first] = glyph;
      } else {
        Radiant::Guard g(m_cache.m_cacheMutex);
        m_cache.m_fileCacheIndex->erase(p.first);
        m_cache.m_cache.erase(p.first);
      }
    } else {
      setFinished();
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  FontCache::D::D(const QRawFont & rawFont, int stretch)
    : m_rawFontKey(makeKey(rawFont, stretch))
  {
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  FontCache & FontCache::acquire(const QRawFont & rawFont, int stretch)
  {
    MULTI_ONCE {
      QSettings settings(indexFileName(), QSettings::IniFormat);
      /// Update this when something is changed with the generation code so that
      /// the old cache needs to be invalidated
      const int version = 4;
      if (settings.value("cache-version").toInt() != version) {
        settings.clear();
        settings.setValue("cache-version", version);
      }

      // Ensure correct deinitialization order
      atexit(deinitialize);
    }

    /// QRawFont doesn't work as a key, since it doesn't have operator== that works as expected.
    /// Also the pixelSize shouldn't matter
    const QString fontKey = makeKey(rawFont, stretch);

    Radiant::Guard g(s_fontCacheMutex);
    std::unique_ptr<Luminous::FontCache> & ptr = s_fontCache[fontKey];
    if (!ptr)
      ptr.reset(new Luminous::FontCache(rawFont, stretch));
    return *ptr;
  }

  void FontCache::init()
  {
    ++s_atlasGeneration;
  }

  void FontCache::deinitialize()
  {
    {
      Radiant::Guard g(s_fontCacheMutex);
      s_fontCache.clear();
    }
    {
      Radiant::Guard g(s_atlasMutex);
      s_atlas.clear();
    }
    SimpleTextLayout::clearCache();
  }

  int FontCache::generation()
  {
    return s_atlasGeneration;
  }

  TextureAtlasGroup<FontCache::Glyph> & FontCache::atlas()
  {
    return s_atlas;
  }

  Radiant::Mutex & FontCache::atlasMutex()
  {
    return s_atlasMutex;
  }

  void FontCache::setMaximumGlyphHighResSize(int size)
  {
    s_maxHiresSize = size;
  }

  void FontCache::setGlyphPersistenceEnabled(bool enabled)
  {
    s_persistGlyphs = enabled;
  }

  FontCache::Glyph * FontCache::glyph(const QRawFont & rawFont, quint32 glyph)
  {
    Radiant::Guard g(m_d->m_cacheMutex);

    // 1. Check if the glyph is already in the cache
    //    It might be a nullptr, meaning that it is queued to be loaded from
    //    a disk or generated from scratch
    auto cacheIt = m_d->m_cache.find(glyph);
    if (cacheIt != m_d->m_cache.end())
      return cacheIt->second;

    // 2. Check if we have file cache loaded yet, if not, wait and create the
    //    task if necessary
    if (!m_d->m_fileCacheIndex) {
      if (!m_d->m_fileCacheIndexLoader) {
        m_d->m_fileCacheIndexLoader = std::make_shared<FileCacheIndexLoader>(*m_d);
        Radiant::BGThread::instance()->addTask(m_d->m_fileCacheIndexLoader);
      }
      return nullptr;
    }

    // 3. Check if the glyph is in file cache, and create a new request for
    //    our glyph. Create FileCacheLoader if it's not running already
    auto fileCacheIt = m_d->m_fileCacheIndex->find(glyph);
    if (fileCacheIt != m_d->m_fileCacheIndex->end() && !fileCacheIt->second.rect.isEmpty()) {
      m_d->m_cache.insert(std::make_pair(glyph, nullptr));
      m_d->m_fileCacheRequests.push_back(std::make_pair(glyph, fileCacheIt->second));

      if (!m_d->m_fileCacheLoader) {
        m_d->m_fileCacheLoader = std::make_shared<FileCacheLoader>(*m_d);
        Radiant::BGThread::instance()->addTask(m_d->m_fileCacheLoader);
      }

      return nullptr;
    }

    // 4. Glyph isn't in cache or filecache, we actually need to generate it
    //    Because of some font engines in Qt aren't thread-safe, we need
    //    to generate QPainterPath in this same thread that generated the
    //    QRawFont and other Qt objects, there is no way to do that in the bg
    //    thread
    m_d->m_cache.insert(std::make_pair(glyph, nullptr));
    // No need to keep the lock during pathForGlyph, since we have already
    // reserved this glyph by setting nullptr to m_cache
    m_d->m_cacheMutex.unlock();
    /// @todo there probably needs to be a limit how many of these we want
    ///       to generate in one frame
    // We can't change the pixelsize in QRawFont with some fonts on Windows,
    // so we just scale the path manually here

    QPainterPath path;
    if (rawFont.isValid())
      path = rawFont.pathForGlyph(glyph);

    /// It would be ideal to somehow generate QRawFont that has pixelSize that
    /// is very close to s_distanceFieldPixelSize. That is because smaller font
    /// size might actually use different hinting values and even totally different
    /// glyphs than the same font with larger size. However, it is not possible:
    /// * We can't always change QRawFont pixelSize, it sometimes crashes, and
    ///   invalidates glyph number, so we might get wrong character with it.
    /// * Same problem with QRawFont::fromFont, even when using the original QFont
    ///   with it

    const float scaleFactor = static_cast<float>(s_distanceFieldPixelSize / rawFont.pixelSize());
    for (int i = 0; i < path.elementCount(); ++i) {
      const QPainterPath::Element & e = path.elementAt(i);
      path.setElementPositionAt(i, e.x * scaleFactor, e.y * scaleFactor);
    }

    m_d->m_cacheMutex.lock();
    m_d->m_glyphGenerationRequests.push_back(std::make_pair(glyph, path));

    if (!m_d->m_glyphGenerator) {
      m_d->m_glyphGenerator = std::make_shared<GlyphGenerator>(*m_d);
      Radiant::BGThread::instance()->addTask(m_d->m_glyphGenerator);
    }

    return nullptr;
  }

  float FontCache::pixelSize() const
  {
    return s_distanceFieldPixelSize;
  }

  FontCache::FontCache(const QRawFont & rawFont, int stretch)
    : m_d(new D(rawFont, stretch))
  {}

  FontCache::~FontCache()
  {
    {
      std::shared_ptr<FileCacheIndexLoader> indexLoader;
      std::shared_ptr<FileCacheLoader> loader;
      std::shared_ptr<GlyphGenerator> glyphGenerator;

      {
        Radiant::Guard g(m_d->m_cacheMutex);
        m_d->m_fileCacheRequests.clear();
        m_d->m_glyphGenerationRequests.clear();
        indexLoader = std::move(m_d->m_fileCacheIndexLoader);
        loader = std::move(m_d->m_fileCacheLoader);
        glyphGenerator = std::move(m_d->m_glyphGenerator);
      }
      if (indexLoader)
        Radiant::BGThread::instance()->removeTask(indexLoader, true, true);

      if (loader)
        Radiant::BGThread::instance()->removeTask(loader, true, true);

      if (glyphGenerator)
        Radiant::BGThread::instance()->removeTask(glyphGenerator, true, true);
    }

    delete m_d;
  }
} // namespace Luminous
