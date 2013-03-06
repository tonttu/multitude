/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "FontCache.hpp"

#include <Luminous/Texture2.hpp>

#include <Luminous/Image.hpp>
#include <Luminous/DistanceFieldGenerator.hpp>
#include <Luminous/RenderManager.hpp>

#include <Radiant/BGThread.hpp>
#include <Radiant/PlatformUtils.hpp>

#include <QRawFont>
#include <QDir>
#include <QPainter>
#include <QSettings>
#include <QThread>

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
  const int s_maxHiresSize = 3072;

  // space character etc
  static Luminous::FontCache::Glyph s_emptyGlyph;

  // QPainterPaths of glyphs. New requests (null items) are added in FontGenerator thread
  // actual QPainterPaths are rendered in main thread
  static Radiant::Mutex s_glyphPainterPathMutex;
  bool s_glyphPainterScheduled = false;
  std::set<std::pair<quint32, QRawFont*> > s_glyphPainterPathQueue;
  std::map<std::pair<quint32, QRawFont*>, QPainterPath> s_glyphPainterPathReady;

  void processGlyphPainterPaths()
  {
    std::pair<quint32, QRawFont*> key;

    {
      Radiant::Guard g(s_glyphPainterPathMutex);
      if (s_glyphPainterPathQueue.empty()) {
        s_glyphPainterScheduled = false;
        return;
      }

      key = *s_glyphPainterPathQueue.begin();
    }

    QPainterPath path = key.second->pathForGlyph(key.first);

    Radiant::Guard g(s_glyphPainterPathMutex);
    s_glyphPainterPathQueue.erase(key);
    s_glyphPainterPathReady.insert(std::make_pair(key, path));

    if (s_glyphPainterPathQueue.empty()) {
      s_glyphPainterScheduled = false;
    } else {
      Valuable::Node::invokeAfterUpdate(processGlyphPainterPaths);
    }
  }

  QString makeKey(const QRawFont & rawFont)
  {
    return QString("%3!%4!%1!%2").arg(rawFont.weight()).
        arg(rawFont.style()).arg(rawFont.familyName(), rawFont.styleName());
  }

  const QString & cacheBasePath()
  {
    static QString s_basePath;

    MULTI_ONCE {
      QString basePath = Radiant::PlatformUtils::getModuleUserDataPath("MultiTouch", false) + "/fontcache";
      if(!QDir().mkpath(basePath)) {
        basePath = QDir::tempPath() + "/cornerstone-fontcache";
        QDir().mkpath(basePath);
      }
      s_basePath = basePath;
    }

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
    Radiant::BinaryData bd;
    bd.write("cornerstone img");
    bd.writeInt32(0); // version
    bd.writeInt32(1); // compression
    bd.writeInt32(image.width());
    bd.writeInt32(image.height());
    bd.writeInt32(image.pixelFormat().layout());
    bd.writeInt32(image.pixelFormat().type());

    QByteArray data = qCompress(image.data(), image.lineSize() * image.height());
    bd.writeInt32(data.size());

    QFile file(filename);
    if (file.open(QFile::WriteOnly)) {
      const int offset = bd.pos();
      file.write((const char*)&offset, sizeof(offset));
      qint64 total = file.write(bd.data(), offset);
      total += file.write(data);
      return total == offset + data.size();
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
      Radiant::BinaryData bd;
      int offset = 0;
      file.read((char*)&offset, sizeof(offset));

      QByteArray buffer = file.read(offset);
      bd.linkTo(buffer.data(), buffer.size());
      bd.setTotal(buffer.size());

      QString hdr;
      if (bd.readString(hdr) && hdr != "cornerstone img") {
        Radiant::warning("loadImage # header error: '%s'", hdr.toUtf8().data());
        return false;
      }

      int version = bd.readInt32();
      (void)version;
      int compression = bd.readInt32();
      int width = bd.readInt32();
      int height = bd.readInt32();
      int layout = bd.readInt32();
      int type = bd.readInt32();
      int dataSize = bd.readInt32();

      image.allocate(width, height, Luminous::PixelFormat(Luminous::PixelFormat::ChannelLayout(layout),
                                                          Luminous::PixelFormat::ChannelType(type)));
      const int size = image.lineSize() * image.height();

      if (compression == 0) {
        return size == dataSize && size == file.read((char*)image.data(), size);
      } else {
        QByteArray data = qUncompress(file.read(dataSize));
        if (data.size() != size) {
          Radiant::warning("loadImage # uncompressed data size: %d (should be %d)", data.size(), size);
          return false;
        }
        memcpy(image.data(), data.data(), size);
        return true;
      }
    } else {
      Radiant::error("loadImage # Failed to open '%s': %s", filename.toUtf8().data(),
                     file.errorString().toUtf8().data());
      return false;
    }
  }
}

namespace Luminous
{
  class FontCache::FontGenerator : public Radiant::Task, public Valuable::Node
  {
  public:
    FontGenerator(FontCache::D & cache);
    virtual ~FontGenerator();

  protected:
    virtual void initialize() OVERRIDE;
    virtual void doTask() OVERRIDE;
    virtual void finished() OVERRIDE;

  private:
    Glyph * generateGlyph(QPainterPath path, quint32 glyphIndex);
    Glyph * getGlyph(quint32 glyphIndex);
    Glyph * makeGlyph(const Image & img);
    void loadFileCache();

    QPainter & createPainter();
    QRawFont & createRawFont();

  private:
    FontCache::D & m_cache;
    Luminous::Image m_src;
    /// These need to be created in the correct thread, and if all glyphs are
    /// found in file cache, these aren't created at all
    std::unique_ptr<QPainter> m_painter;
    std::unique_ptr<QImage> m_painterImg;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class FontCache::D
  {
  public:
    D(const QRawFont & rawFont);

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
    QRawFont m_rawFont;
    const QString m_rawFontKey;

    /// This locks m_cache, m_request, and m_taskRunning
    Radiant::Mutex m_cacheMutex;
    /// Needed for proper destruction
    Radiant::Condition m_cacheCondition;

    std::map<quint32, Glyph*> m_cache;
    std::set<quint32> m_request;
    bool m_taskCreated;

    bool m_fileCacheLoaded;
    std::map<quint32, FileCacheItem> m_fileCache;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  FontCache::FontGenerator::FontGenerator(FontCache::D & cache)
    : Radiant::Task(PRIORITY_HIGH)
    , m_cache(cache)
  {
    eventAddOut("glyph-ready");
    eventAddListenerBd("glyph-ready", [&] (Radiant::BinaryData & bd) {
      Texture * tex = RenderManager::getResource<Texture>(bd.read<int64_t>());
      TextureAtlas::Node node;
      node.m_location = bd.read<Nimble::Vector2i>();
      node.m_size = bd.read<Nimble::Vector2i>();
      if (tex) {
        tex->addDirtyRect(QRect(node.m_location.x, node.m_location.y,
                                node.m_size.x, node.m_size.y));
      }
    }, AFTER_UPDATE);
  }

  FontCache::FontGenerator::~FontGenerator()
  {
    finished();
  }

  void FontCache::FontGenerator::initialize()
  {
    if (!m_cache.m_fileCacheLoaded)
      loadFileCache();
  }

  void FontCache::FontGenerator::doTask()
  {
    for (int i = 0; i < 5; ++i) {
      quint32 request = 0;
      {
        Radiant::Guard g(m_cache.m_cacheMutex);
        if (m_cache.m_request.empty()) {
          m_cache.m_taskCreated = false;
          setFinished();
          return;
        }

        std::set<quint32>::const_iterator it = m_cache.m_request.begin();
        for (int j = 0; j < i; ++j) {
          ++it;
          if (it == m_cache.m_request.end()) {
            scheduleFromNowSecs(0.02);
            return;
          }
        }

        request = *it;
      }

      Glyph * glyph = getGlyph(request);

      if (glyph) {
        Radiant::Guard g(m_cache.m_cacheMutex);
        m_cache.m_request.erase(request);
        m_cache.m_cache.insert(std::make_pair(request, glyph));

        if (m_cache.m_request.empty()) {
          m_cache.m_taskCreated = false;
          setFinished();
        }
        return;
      }
    }
    scheduleFromNowSecs(0.02);
  }

  void FontCache::FontGenerator::finished()
  {
    // delete these in this thread
    m_painter.reset();
    m_painterImg.reset();
    {
      Radiant::Guard g(m_cache.m_cacheMutex);
      m_cache.m_request.clear();
      m_cache.m_cacheCondition.wakeAll();
      m_cache.m_taskCreated = false;
    }
  }

  FontCache::Glyph * FontCache::FontGenerator::generateGlyph(QPainterPath path, quint32 glyphIndex)
  {
    QPainter & painter = createPainter();

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

    const QRectF br = path.boundingRect();

    const float glyphSize = std::max(br.width(), br.height());
    const float distanceFieldSize = glyphSize + 2.0 * s_padding;
    const float hiresSize = std::min((float)s_maxHiresSize, s_maxHiresSize * glyphSize / s_distanceFieldPixelSize);
    const float hiresPadding = hiresSize * s_padding / distanceFieldSize;
    const float hiresFactor = hiresSize / distanceFieldSize;

    const float hiresContentSize = hiresSize - hiresPadding * 2.0f;
    const float hiresContentScale = hiresContentSize / glyphSize;

    const Nimble::Vector2f translate(hiresPadding - br.left() * hiresContentScale,
                                     hiresPadding - br.top() * hiresContentScale);

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

    QImage & img = *static_cast<QImage*>(painter.device());
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
    DistanceFieldGenerator::generate(m_src, srcSize, sdf, hiresPadding);

    FontCache::Glyph * glyph = makeGlyph(sdf);
    glyph->setSize(Nimble::Vector2f(2.0f * s_padding + br.width(),
                                    2.0f * s_padding + br.height()));
    glyph->setLocation(Nimble::Vector2f(br.left() - s_padding,
                                        br.top() - s_padding));


    const QString file = cacheFileName(m_cache.m_rawFontKey, glyphIndex);

    if (saveImage(sdf, file)) {
      FontCache::D::FileCacheItem item(file, QRectF(glyph->location().x, glyph->location().y,
                                     glyph->size().x, glyph->size().y));
      m_cache.m_fileCache[glyphIndex] = item;

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

  FontCache::Glyph * FontCache::FontGenerator::getGlyph(quint32 glyphIndex)
  {
    auto it = m_cache.m_fileCache.find(glyphIndex);
    if (it != m_cache.m_fileCache.end()) {
      const FontCache::D::FileCacheItem & item = it->second;
      if (!item.rect.isEmpty()) {
        Luminous::Image img;
        if (loadImage(img, item.src)) {
          FontCache::Glyph * glyph = makeGlyph(img);
          glyph->setLocation(Nimble::Vector2f(item.rect.left(), item.rect.top()));
          glyph->setSize(Nimble::Vector2f(item.rect.width(), item.rect.height()));
          return glyph;
        }
      }
    }

    QPainterPath path;
    {
      const std::pair<quint32, QRawFont*> key = std::make_pair(glyphIndex, &m_cache.m_rawFont);
      Radiant::Guard g(s_glyphPainterPathMutex);

      auto it = s_glyphPainterPathReady.find(key);
      if (it == s_glyphPainterPathReady.end()) {
        s_glyphPainterPathQueue.insert(key);
        if (!s_glyphPainterScheduled) {
          Node::invokeAfterUpdate(processGlyphPainterPaths);
          s_glyphPainterScheduled = true;
        }
        return nullptr;
      } else {
        path = std::move(it->second);
        s_glyphPainterPathReady.erase(it);
      }
    }

    return generateGlyph(path, glyphIndex);
  }

  FontCache::Glyph * FontCache::FontGenerator::makeGlyph(const Image & img)
  {
    Glyph * glyph;
    {
      Radiant::Guard g(s_atlasMutex);
      glyph = &s_atlas.insert(img.size());
    }

    Image & target = glyph->m_atlas->image();
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

    Texture & texture = glyph->m_atlas->texture();
    eventSend("glyph-ready", (int64_t)texture.resourceId(), glyph->m_node->m_location, glyph->m_node->m_size);

    return glyph;
  }

  void FontCache::FontGenerator::loadFileCache()
  {
    QSettings settings(indexFileName(), QSettings::IniFormat);

    settings.beginGroup(m_cache.m_rawFontKey);

    for (const QString & index : settings.childGroups()) {
      settings.beginGroup(index);
      const quint32 glyphIndex = index.toUInt();
      const QRectF r = settings.value("rect").toRectF();
      const QString src = settings.value("src").toString();
      m_cache.m_fileCache[glyphIndex] = FontCache::D::FileCacheItem(src, r);
      settings.endGroup();
    }
    settings.endGroup();
    m_cache.m_fileCacheLoaded = true;
  }

  QPainter & FontCache::FontGenerator::createPainter()
  {
    if (m_painter)
      return *m_painter;

    m_painterImg.reset(new QImage(s_maxHiresSize, s_maxHiresSize, QImage::Format_ARGB32_Premultiplied));
    m_painter.reset(new QPainter(m_painterImg.get()));

    m_painter->setRenderHint(QPainter::Antialiasing, true);
    m_painter->setRenderHint(QPainter::TextAntialiasing, true);
    m_painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
    m_painter->setPen(Qt::NoPen);
    m_painter->setBrush(QBrush(Qt::black));

    m_src.allocate(s_maxHiresSize, s_maxHiresSize, Luminous::PixelFormat::alphaUByte());

    return *m_painter;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  FontCache::D::D(const QRawFont & rawFont)
    : m_rawFont(rawFont)
    // This needs to be created before calling setPixelSize(), since after that
    // the font is in some weird state before rendering anything
    , m_rawFontKey(makeKey(m_rawFont))
    , m_taskCreated(false)
    , m_fileCacheLoaded(false)
  {
    m_rawFont.setPixelSize(s_distanceFieldPixelSize);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  FontCache & FontCache::acquire(const QRawFont & rawFont)
  {
    MULTI_ONCE {
      QSettings settings(indexFileName(), QSettings::IniFormat);
      /// Update this when something is changed with the generation code so that
      /// the old cache needs to be invalidated
      const int version = 2;
      if (settings.value("cache-version").toInt() != version) {
        settings.clear();
        settings.setValue("cache-version", version);
      }
    }

    /// QRawFont doesn't work as a key, since it doesn't have operator== that works as expected.
    /// Also the pixelSize shouldn't matter
    const QString fontKey = makeKey(rawFont);

    Radiant::Guard g(s_fontCacheMutex);
    std::unique_ptr<Luminous::FontCache> & ptr = s_fontCache[fontKey];
    if (!ptr)
      ptr.reset(new Luminous::FontCache(rawFont));
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

  FontCache::Glyph * FontCache::glyph(quint32 glyph)
  {
    bool createTask = false;
    {
      Radiant::Guard g(m_d->m_cacheMutex);
      auto it = m_d->m_cache.find(glyph);
      if (it != m_d->m_cache.end())
        return it->second;

      m_d->m_request.insert(glyph);
      if (!m_d->m_taskCreated) {
        m_d->m_taskCreated = true;
        createTask = true;
      }
    }
    if (createTask)
      Radiant::BGThread::instance()->addTask(std::make_shared<FontGenerator>(*m_d));
    return nullptr;
  }

  float FontCache::pixelSize() const
  {
    return s_distanceFieldPixelSize;
  }

  FontCache::FontCache(const QRawFont & rawFont)
    : m_d(new D(rawFont))
  {}

  FontCache::~FontCache()
  {
    {
      Radiant::Guard g(s_glyphPainterPathMutex);
      for (auto it = s_glyphPainterPathQueue.begin(); it != s_glyphPainterPathQueue.end();) {
        if (it->second == &m_d->m_rawFont) {
          it = s_glyphPainterPathQueue.erase(it);
        } else {
          ++it;
        }
      }

      for (auto it = s_glyphPainterPathReady.begin(); it != s_glyphPainterPathReady.end();) {
        if (it->first.second == &m_d->m_rawFont) {
          it = s_glyphPainterPathReady.erase(it);
        } else {
          ++it;
        }
      }
    }

    {
      Radiant::Guard g(m_d->m_cacheMutex);
      while(!m_d->m_request.empty())
        m_d->m_cacheCondition.wait(m_d->m_cacheMutex);
    }

    delete m_d;
  }
} // namespace Luminous
