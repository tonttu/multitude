#include "FontCache.hpp"

#include <Luminous/Texture2.hpp>
#include <Luminous/BGThread.hpp>
#include <Luminous/Image.hpp>
#include <Luminous/DistanceFieldGenerator.hpp>

#include <Radiant/PlatformUtils.hpp>

#include <QRawFont>
#include <QDir>
#include <QPainter>
#include <QSettings>

namespace
{
  std::map<QString, std::unique_ptr<Luminous::FontCache>> s_fontCache;
  Radiant::Mutex s_fontCacheMutex;

  Luminous::TextureAtlasGroup<Luminous::FontCache::Glyph> s_atlas(Luminous::PixelFormat::redUByte());
  Radiant::Mutex s_atlasMutex;

  const int s_distanceFieldPixelSize = 160;
  const int s_maxHiresSize = 2048;
  const float s_padding = 1/16.0f;

  // space character etc
  static Luminous::FontCache::Glyph s_emptyGlyph;

  QString makeKey(const QRawFont & rawFont)
  {
    return QString("%3!%4!%1!%2").arg(rawFont.weight()).
        arg(rawFont.style()).arg(rawFont.familyName(), rawFont.styleName());
  }

  QString cacheFileName(QString fontKey, quint32 glyphIndex)
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

    const QString path = s_basePath + "/" + fontKey.replace('/', '_');
    QDir().mkdir(path);

    return QString("%1/%2.tga").arg(path).arg(glyphIndex);
  }
}

namespace Luminous
{
  class FontCache::FontGenerator : public Luminous::Task
  {
  public:
    FontGenerator(FontCache::D & cache);

  protected:
    virtual void doTask() OVERRIDE;

  private:
    Glyph * generateGlyph(quint32 glyphIndex);
    Glyph * getGlyph(quint32 glyphIndex);
    Glyph * makeGlyph(const Image & img);
    void loadFileCache();

    QPainter & createPainter();
    QRawFont & createRawFont();

  private:
    FontCache::D & m_cache;
    Luminous::Image m_src;
    const QString m_fontKey;
    /// These need to be created in the correct thread, and if all glyphs are
    /// found in file cache, these aren't created at all
    std::unique_ptr<QPainter> m_painter;
    std::unique_ptr<QImage> m_painterImg;
    /// @todo QRawFont isn't thread-safe, so we make our own copy.
    ///       However, it's unclear if even the copy constructor is thread-safe / reentrant
    std::unique_ptr<QRawFont> m_rawFont;
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

      /// Filename (TGA)
      QString src;
      /// Glyph::m_location, Glyph::m_size
      QRectF rect;
    };

  public:
    QRawFont m_rawFont;

    /// This locks m_cache, m_request, and m_taskRunning
    Radiant::Mutex m_cacheMutex;

    std::map<quint32, Glyph*> m_cache;
    std::set<quint32> m_request;
    bool m_taskCreated;

    bool m_fileCacheLoaded;
    std::map<quint32, FileCacheItem> m_fileCache;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  FontCache::FontGenerator::FontGenerator(FontCache::D & cache)
    : m_cache(cache)
    , m_fontKey(makeKey(cache.m_rawFont))
  {
  }

  void FontCache::FontGenerator::doTask()
  {
    if (!m_cache.m_fileCacheLoaded)
      loadFileCache();

    quint32 request = 0;
    bool first = true;
    while (true) {
      Glyph * ready = nullptr;
      if (!first)
        ready = getGlyph(request);

      Radiant::Guard g(m_cache.m_cacheMutex);
      if (ready) {
        m_cache.m_request.erase(request);
        m_cache.m_cache.insert(std::make_pair(request, ready));
      }

      if (m_cache.m_request.empty()) {
        m_cache.m_taskCreated = false;
        setFinished();
        break;
      } else {
        request = *m_cache.m_request.begin();
        first = false;
      }
    }

    // delete these in this thread
    m_painter.reset();
    m_painterImg.reset();
    m_rawFont.reset();
  }

  FontCache::Glyph * FontCache::FontGenerator::generateGlyph(quint32 glyphIndex)
  {
    QPainter & painter = createPainter();
    QRawFont & rawFont = createRawFont();

    QPainterPath path = rawFont.pathForGlyph(glyphIndex);
    if (path.isEmpty()) {
      QSettings settings("MultiTouch", "GlyphCache");

      settings.beginGroup(m_fontKey);
      settings.beginGroup(QString::number(glyphIndex));
      settings.setValue("rect", QRectF());
      settings.endGroup();
      settings.endGroup();

      return &s_emptyGlyph;
    }

    const QRectF br = path.boundingRect();

    const float glyphSize = std::max(br.width(), br.height());
    const float distanceFieldSize = std::min<float>(s_distanceFieldPixelSize, glyphSize * (1.0f + s_padding * 2.0f));
    const float hiresSize = std::min<float>(s_maxHiresSize, distanceFieldSize / s_distanceFieldPixelSize * s_maxHiresSize);
    const float hiresPadding = s_padding * hiresSize;
    const float dfPadding = s_padding * distanceFieldSize;

    const float hiresContentSize = (1.0 - s_padding * 2.0) * hiresSize;
    const float dfContentSize = (1.0 - s_padding * 2.0) * distanceFieldSize;

    const float hiresScale = hiresContentSize / glyphSize;
    const float dfScale = dfContentSize / glyphSize;

    const Nimble::Vector2f translate(hiresPadding - br.left() * hiresScale,
                                     hiresPadding - br.top() * hiresScale);

    const Nimble::Vector2i sdfSize(Nimble::Math::Round(br.width() * dfScale + 2.0f * dfPadding),
                                   Nimble::Math::Round(br.height() * dfScale + 2.0f * dfPadding));

    const Nimble::Vector2i srcSize(Nimble::Math::Round(br.width() * hiresScale + 2.0f * hiresPadding),
                                   Nimble::Math::Round(br.height() * hiresScale + 2.0f * hiresPadding));

    // Scale & transform the path to fill image of the size (hiresSize x hiresSize)
    // while keeping the correct aspect ratio and having hiresPadding on every edge.
    // Also move the path to origin
    for (int i = 0; i < path.elementCount(); ++i) {
      const QPainterPath::Element & e = path.elementAt(i);
      path.setElementPositionAt(i, e.x * hiresScale + translate.x, e.y * hiresScale + translate.y);
    }

    QImage & img = *static_cast<QImage*>(painter.device());
    img.fill(Qt::transparent);
    painter.drawPath(path);

    for (int y = 0; y < s_maxHiresSize; ++y) {
      const QRgb * from = reinterpret_cast<const QRgb*>(img.constScanLine(y));
      unsigned char * to = m_src.line(y);
      for (int x = 0; x < s_maxHiresSize; ++x)
        to[x] = qAlpha(from[x]);
    }

    Image sdf;
    sdf.allocate(sdfSize.x, sdfSize.y, Luminous::PixelFormat::redUByte());
    DistanceFieldGenerator::generate(m_src, srcSize, sdf, hiresSize / 12);

    FontCache::Glyph * glyph = makeGlyph(sdf);
    glyph->setSize(Nimble::Vector2f(2.0f * s_padding * glyphSize + br.width(),
                                    2.0f * s_padding * glyphSize + br.height()));
    glyph->setLocation(Nimble::Vector2f(br.left() - s_padding * glyphSize,
                                        br.top() - s_padding * glyphSize));


    const QString file = cacheFileName(m_fontKey, glyphIndex);

    if (sdf.write(file.toUtf8().data())) {
      FontCache::D::FileCacheItem item(file, QRectF(glyph->location().x, glyph->location().y,
                                     glyph->size().x, glyph->size().y));
      m_cache.m_fileCache[glyphIndex] = item;

      QSettings settings("MultiTouch", "GlyphCache");

      settings.beginGroup(m_fontKey);
      settings.beginGroup(QString::number(glyphIndex));
      settings.setValue("rect", item.rect);
      settings.setValue("src", file);
      settings.endGroup();
      settings.endGroup();
    }

    /*
    Radiant::info("%f %f, size %f %f", br.left(), br.top(), br.width(), br.height());
    Radiant::info("%f %f, size %f %f", path.boundingRect().left(), path.boundingRect().top(), path.boundingRect().width(), path.boundingRect().height());
    Radiant::info("hiresScale %f", hiresScale);
    Radiant::info("hiresPadding %f", hiresPadding);
    Radiant::info("hiresContentSize %f", hiresContentSize);
    Radiant::info("glyphSize %f", glyphSize);

    Radiant::info("dfScale %f", dfScale);
    Radiant::info("dfPadding %f", dfPadding);
    Radiant::info("dfContentSize %f", dfContentSize);

    Radiant::info("srcSize %d %d", srcSize.x, srcSize.y);
    Radiant::info("sdfSize %d %d", sdfSize.x, sdfSize.y);
    Radiant::info("glyph->m_size %f %f", glyph->m_size.x, glyph->m_size.y);
    Radiant::info("left top %f %f", left, top);
    */

    return glyph;
  }

  FontCache::Glyph * FontCache::FontGenerator::getGlyph(quint32 glyphIndex)
  {
    auto it = m_cache.m_fileCache.find(glyphIndex);
    if (it != m_cache.m_fileCache.end()) {
      const FontCache::D::FileCacheItem & item = it->second;
      if (item.rect.isEmpty())
        return &s_emptyGlyph;

      Luminous::Image img;
      if (img.read(item.src.toUtf8().data())) {
        FontCache::Glyph * glyph = makeGlyph(img);
        glyph->setLocation(Nimble::Vector2f(item.rect.left(), item.rect.top()));
        glyph->setSize(Nimble::Vector2f(item.rect.width(), item.rect.height()));
        return glyph;
      }
    }
    return generateGlyph(glyphIndex);
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
        for (int x = 0; x < img.width(); ++x) {
          target.setPixel(glyph->m_node->m_location.x+y, glyph->m_node->m_location.y+x,
                          Nimble::Vector4f(from[x] / 255.0f, 0, 0, 0));
        }
      } else {
        unsigned char * to = target.line(glyph->m_node->m_location.y+y) + glyph->m_node->m_location.x;
        std::copy(from, from + img.width(), to);
      }
    }

    Texture & texture = glyph->m_atlas->texture();
    {
      Radiant::Guard g(glyph->m_atlas->textureMutex());
      texture.addDirtyRect(QRect(glyph->m_node->m_location.x, glyph->m_node->m_location.y,
                                 glyph->m_node->m_size.x, glyph->m_node->m_size.y));
    }

    return glyph;
  }

  void FontCache::FontGenerator::loadFileCache()
  {
    QSettings settings("MultiTouch", "GlyphCache");

    settings.beginGroup(m_fontKey);

    foreach (const QString & index, settings.childGroups()) {
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

  QRawFont & FontCache::FontGenerator::createRawFont()
  {
    if (m_rawFont)
      return *m_rawFont;

    m_rawFont.reset(new QRawFont(m_cache.m_rawFont));
    return *m_rawFont;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  FontCache::D::D(const QRawFont & rawFont)
    : m_rawFont(rawFont)
    , m_taskCreated(false)
    , m_fileCacheLoaded(false)
  {
    m_rawFont.setPixelSize(s_distanceFieldPixelSize);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  FontCache & FontCache::acquire(const QRawFont & rawFont)
  {
    /// QRawFont doesn't work as a key, since it doesn't have operator== that works as expected.
    /// Also the pixelSize shouldn't matter
    const QString fontKey = makeKey(rawFont);

    Radiant::Guard g(s_fontCacheMutex);
    std::unique_ptr<Luminous::FontCache> & ptr = s_fontCache[fontKey];
    if (!ptr)
      ptr.reset(new Luminous::FontCache(rawFont));
    return *ptr;
  }

  FontCache::Glyph * FontCache::glyph(quint32 glyph)
  {
    Radiant::Guard g(m_d->m_cacheMutex);
    auto it = m_d->m_cache.find(glyph);
    if (it != m_d->m_cache.end())
      return it->second;

    m_d->m_request.insert(glyph);
    if (!m_d->m_taskCreated) {
      m_d->m_taskCreated = true;
      Luminous::BGThread::instance()->addTask(new FontGenerator(*m_d));
    }
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
    delete m_d;
  }
} // namespace Luminous