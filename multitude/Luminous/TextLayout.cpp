#include "TextLayout.hpp"
#include "RenderResource.hpp"
#include "Texture2.hpp"
#include "BGThread.hpp"
#include "Image.hpp"
#include "DistanceFieldGenerator.hpp"

#include <Radiant/Mutex.hpp>

#include <tuple>
#include <unordered_map>

#include <QHash>
#include <QFontMetricsF>
#include <QTextLayout>
#include <QPainter>

namespace std
{
  template<> struct hash<std::tuple<QString, Nimble::Vector2i, QFont>>
  {
    inline size_t operator()(const std::tuple<QString, Nimble::Vector2i, QFont> & tuple) const
    {
      std::hash<uint> hasher;
      return qHash(std::get<0>(tuple)) ^ hasher(std::get<1>(tuple).x) ^
          hasher(std::get<1>(tuple).y) ^ qHash(std::get<2>(tuple).key());
    }
  };
}

namespace
{
  std::unordered_map<std::tuple<QString, Nimble::Vector2i, QFont>, std::unique_ptr<Luminous::TextLayout>> s_layoutCache;
  Radiant::Mutex s_layoutCacheMutex;

  std::map<QString, std::unique_ptr<Luminous::FontCache>> s_fontCache;
  Radiant::Mutex s_fontCacheMutex;

  Luminous::TextureAtlasGroup<Luminous::FontCache::Glyph> m_atlas(Luminous::PixelFormat::redUByte());
  Radiant::Mutex s_atlasMutex;

  const int s_distanceFieldPixelSize = 160;
  const int s_maxHiresSize = 2048;
  const float s_padding = 1/16.0f;
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
    Glyph * generateGlyph(QPainter & painter, const QRawFont & rawFont, quint32 glyphIndex);

  private:
    FontCache::D & m_cache;
    Luminous::Image m_src;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class FontCache::D
  {
  public:
    D(const QRawFont & rawFont);

  public:
    QRawFont m_rawFont;

    /// This locks m_cache, m_request, and m_taskRunning
    Radiant::Mutex m_cacheMutex;

    std::map<quint32, Glyph*> m_cache;
    std::set<quint32> m_request;
    bool m_taskCreated;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  FontCache::FontGenerator::FontGenerator(FontCache::D & cache)
    : m_cache(cache)
  {
  }

  void FontCache::FontGenerator::doTask()
  {
    QImage img(s_maxHiresSize, s_maxHiresSize, QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&img);

    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setRenderHint(QPainter::HighQualityAntialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(Qt::black));

    m_src.allocate(s_maxHiresSize, s_maxHiresSize, Luminous::PixelFormat::alphaUByte());

    /// @todo QRawFont isn't thread-safe, so we make our own copy.
    ///       However, it's unclear if even the copy constructor is thread-safe / reentrant
    const QRawFont rawFont = m_cache.m_rawFont;

    quint32 request = 0;
    bool first = true;
    while (true) {
      Glyph * ready = nullptr;
      if (!first)
        ready = generateGlyph(painter, rawFont, request);

      Radiant::Guard g(m_cache.m_cacheMutex);
      if (ready) {
        m_cache.m_request.erase(request);
        m_cache.m_cache.insert(std::make_pair(request, ready));
      }

      if (m_cache.m_request.empty()) {
        m_cache.m_taskCreated = false;
        setFinished();
        return;
      } else {
        request = *m_cache.m_request.begin();
        first = false;
      }
    }
  }

  FontCache::Glyph * FontCache::FontGenerator::generateGlyph(QPainter & painter, const QRawFont & rawFont,
                                               quint32 glyphIndex)
  {
    QPainterPath path = rawFont.pathForGlyph(glyphIndex);
    if (path.isEmpty()) {
      // space character etc
      static FontCache::Glyph s_emptyGlyph;
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

    Glyph * glyph;
    {
      Radiant::Guard g(s_atlasMutex);
      glyph = &m_atlas.insert(sdfSize);
    }

    Image sdf;
    sdf.allocate(sdfSize.x, sdfSize.y, Luminous::PixelFormat::redUByte());
    DistanceFieldGenerator::generate(m_src, srcSize, sdf, hiresSize / 12);

    Image & target = glyph->m_atlas->image();
    for (int y = 0; y < sdfSize.y; ++y) {
      unsigned char * from = sdf.line(y);
      if (glyph->m_node->m_rotated) {
        for (int x = 0; x < sdfSize.x; ++x) {
          target.setPixel(glyph->m_node->m_location.x+y, glyph->m_node->m_location.y+x,
                          Nimble::Vector4f(from[x] / 255.0f, 0, 0, 0));
        }
      } else {
        unsigned char * to = target.line(glyph->m_node->m_location.y+y) + glyph->m_node->m_location.x;
        std::copy(from, from + sdfSize.x, to);
      }
    }

    glyph->setSize(Nimble::Vector2f(2.0f * s_padding * glyphSize + br.width(),
                                    2.0f * s_padding * glyphSize + br.height()));

    glyph->setLocation(Nimble::Vector2f(br.left() - s_padding * glyphSize,
                                        br.top() - s_padding * glyphSize));

    Texture & texture = glyph->m_atlas->texture();
    {
      Radiant::Guard g(glyph->m_atlas->textureMutex());
      texture.addDirtyRect(QRect(glyph->m_node->m_location.x, glyph->m_node->m_location.y,
                                 glyph->m_node->m_size.x, glyph->m_node->m_size.y));
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

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  FontCache::D::D(const QRawFont & rawFont)
    : m_rawFont(rawFont)
    , m_taskCreated(false)
  {
    m_rawFont.setPixelSize(s_distanceFieldPixelSize);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  FontCache & FontCache::acquire(const QRawFont & rawFont)
  {
    /// QRawFont doesn't work as a key, since it doesn't have operator== that works as expected.
    /// Also the pixelSize shouldn't matter
    const QString fontKey = QString("%3\n%4\n%1\n%2").arg(rawFont.weight()).
        arg(rawFont.style()).arg(rawFont.familyName(), rawFont.styleName());

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

  FontCache::FontCache(const QRawFont & rawFont)
    : m_d(new D(rawFont))
  {}

  FontCache::~FontCache()
  {
    delete m_d;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  class TextLayout::D
  {
  public:
    struct Group
    {
      Group(Texture & tex) : texture(&tex) {}
      Texture * texture;
      std::vector<TextLayout::Item> items;
    };

  public:
    D(const QString & text, const Nimble::Vector2f & size, const QFont & font);

    void layout();
    void regenerate();

  private:
    Group & findGroup(Texture & texture);

  public:
    const Nimble::Vector2f m_size;
    QTextLayout m_layout;
    bool m_complete;

    std::map<RenderResource::Id, int> m_groupCache;
    std::vector<Group> m_groups;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  TextLayout::D::D(const QString & text, const Nimble::Vector2f & size, const QFont & font)
    : m_size(size)
    , m_layout(text, font)
    , m_complete(false)
  {
    layout();
    /*QImage img(m_layout.boundingRect().width(), m_layout.boundingRect().height(), QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_layout.draw(&p, QPointF(0, 0));
    img.save("/tmp/qlayout.png");*/
  }

  void TextLayout::D::layout()
  {
    assert(!m_layout.font().kerning());
    QFontMetricsF fontMetrics(m_layout.font());
    const float lineWidth = m_size.x;
    const float leading = fontMetrics.leading();

    float y = 0;
    m_layout.beginLayout();
    while (true) {
      QTextLine line = m_layout.createLine();
      if (!line.isValid())
        break;

      line.setLineWidth(lineWidth);
      y += leading;
      line.setPosition(QPointF(0, y));
      y += line.height();
    }
    m_layout.endLayout();
  }

  /// @todo what should be thread-safe?
  void TextLayout::D::regenerate()
  {
    const Nimble::Vector2f layoutLocation(m_layout.position().x(), m_layout.position().y());
    bool missingGlyphs = false;

    m_groupCache.clear();
    m_groups.clear();

    /// @todo in Qt 5.0, we should use individual lines or give range to glyphRuns
    foreach (const QGlyphRun & glyphRun, m_layout.glyphRuns()) {
      const QRawFont & font = glyphRun.rawFont();
      const QVector<quint32> & glyphs = glyphRun.glyphIndexes();
      const QVector<QPointF> & positions = glyphRun.positions();
      const QVector<QPointF> & advances = font.advancesForGlyphIndexes(glyphs);

      FontCache & cache = FontCache::acquire(font);

      for (int i = 0; i < glyphs.size(); ++i) {
        const quint32 glyph = glyphs[i];

        FontCache::Glyph * glyphCache = cache.glyph(glyph);
        if (glyphCache) {
          if (glyphCache->isEmpty())
            continue;

          const float scale = float(font.pixelSize()) / s_distanceFieldPixelSize;
          const Nimble::Vector2f location = Nimble::Vector2f(positions[i].x(), positions[i].y()) +
              layoutLocation + glyphCache->location() * scale;
          const Nimble::Vector2f & size = glyphCache->size() * scale;

          Group & g = findGroup(glyphCache->texture());

          TextLayout::Item item;
          item.vertices[0].location.make(location.x, location.y, 0);
          item.vertices[1].location.make(location.x+size.x, location.y, 0);
          item.vertices[2].location.make(location.x, location.y+size.y, 0);
          item.vertices[3].location.make(location.x+size.x, location.y+size.y, 0);
          for (int j = 0; j < 4; ++j)
            item.vertices[j].texCoord = glyphCache->uv()[j];

          g.items.push_back(item);
        } else {
          missingGlyphs = true;
        }
      }
    }

    m_complete = !missingGlyphs;
  }

  TextLayout::D::Group & TextLayout::D::findGroup(Texture & texture)
  {
    auto it = m_groupCache.find(texture.resourceId());
    if (it != m_groupCache.end()) {
      return m_groups[it->second];
    }
    m_groupCache.insert(std::make_pair(texture.resourceId(), m_groups.size()));
    m_groups.emplace_back(texture);
    return m_groups.back();
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  TextLayout::TextLayout(const QString & text, const Nimble::Vector2f & size, const QFont & font)
    : m_d(new D(text, size, font))
  {
    assert(!font.kerning());
  }

  /// @todo should actually use "useCache"-variable
  const TextLayout & TextLayout::layout(const QString & text, const Nimble::Vector2f & size, QFont font, bool useCache)
  {
    font.setKerning(false);
    TextLayout * layout;

    {
      /// @todo someone should also delete old layouts..
      Radiant::Guard g(s_layoutCacheMutex);
      std::unique_ptr<TextLayout> & ptr = s_layoutCache[std::make_tuple(text, size.cast<int>(), font)];
      if (!ptr) {
        ptr.reset(new TextLayout(text, size, font));
      }
      layout = ptr.get();
    }

    if (!layout->isComplete()) {
      layout->m_d->regenerate();
    }

    return *layout;
  }

  TextLayout::~TextLayout()
  {
    delete m_d;
  }

  int TextLayout::groupCount() const
  {
    return m_d->m_groups.size();
  }

  Texture * TextLayout::texture(int groupIndex) const
  {
    return m_d->m_groups[groupIndex].texture;
  }

  const std::vector<TextLayout::Item> & TextLayout::items(int groupIndex) const
  {
    return m_d->m_groups[groupIndex].items;
  }

  bool TextLayout::isComplete() const
  {
    return m_d->m_complete;
  }

} // namespace Luminous
