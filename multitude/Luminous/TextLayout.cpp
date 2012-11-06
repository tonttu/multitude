#include "TextLayout.hpp"

#include <Luminous/FontCache.hpp>

#include <QGlyphRun>

namespace Luminous
{
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
    D(const Nimble::Vector2f & size);

    bool generate(const Nimble::Vector2f & location, const QGlyphRun & glyphRun);

  protected:
    Group & findGroup(Texture & texture);

  public:
    Nimble::Vector2f m_maximumSize;
    Nimble::Vector2f m_renderLocation;
    Nimble::Rectf m_boundingBox;
    /// Set to false if we need to do layout again
    bool m_layoutReady;
    /// Do we have all glyphs == no need to call regenerate()
    bool m_glyphsReady;

    std::map<RenderResource::Id, int> m_groupCache;
    std::vector<Group> m_groups;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  TextLayout::D::D(const Nimble::Vector2f & maximumSize)
    : m_maximumSize(maximumSize)
    , m_renderLocation(0, 0)
    , m_layoutReady(false)
    , m_glyphsReady(false)
  {
    /*QImage img(m_layout.boundingRect().width(), m_layout.boundingRect().height(), QImage::Format_ARGB32_Premultiplied);
    QPainter p(&img);
    m_layout.draw(&p, QPointF(0, 0));
    img.save("/tmp/qlayout.png");*/
  }

  bool TextLayout::D::generate(const Nimble::Vector2f & layoutLocation,
                               const QGlyphRun & glyphRun)
  {
    bool missingGlyphs = false;

    const QRawFont & font = glyphRun.rawFont();
    const QVector<quint32> & glyphs = glyphRun.glyphIndexes();
    const QVector<QPointF> & positions = glyphRun.positions();

    FontCache & cache = FontCache::acquire(font);
    const float scale = float(font.pixelSize()) / cache.pixelSize();
    const float invsize = 1.0f / float(font.pixelSize());

    for (int i = 0; i < glyphs.size(); ++i) {
      const quint32 glyph = glyphs[i];

      FontCache::Glyph * glyphCache = cache.glyph(glyph);
      if (glyphCache) {
        if (glyphCache->isEmpty())
          continue;

        const Nimble::Vector2f location = Nimble::Vector2f(positions[i].x(), positions[i].y()) +
            layoutLocation + glyphCache->location() * scale;
        const Nimble::Vector2f & size = glyphCache->size() * scale;

        Group & g = findGroup(glyphCache->texture());

        TextLayout::Item item;
        item.vertices[0].location.make(location.x, location.y);
        item.vertices[1].location.make(location.x+size.x, location.y);
        item.vertices[2].location.make(location.x, location.y+size.y);
        item.vertices[3].location.make(location.x+size.x, location.y+size.y);
        for (int j = 0; j < 4; ++j) {
          item.vertices[j].texCoord = glyphCache->uv()[j];
          item.vertices[j].invsize = invsize;
        }

        g.items.push_back(item);
      } else {
        missingGlyphs = true;
      }
    }

    return missingGlyphs;
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

  TextLayout::TextLayout(const Nimble::Vector2f & maximumSize)
    : m_d(new D(maximumSize))
  {}

  TextLayout::~TextLayout()
  {
    delete m_d;
  }

  TextLayout::TextLayout(const TextLayout & t)
    : m_d(new D(*t.m_d))
  {
  }

  TextLayout::TextLayout(TextLayout && t)
    : m_d(t.m_d)
  {
    t.m_d = nullptr;
  }

  TextLayout & TextLayout::operator=(const TextLayout & t)
  {
    *m_d = *t.m_d;
    return *this;
  }

  TextLayout & TextLayout::operator=(TextLayout && t)
  {
    std::swap(m_d, t.m_d);
    return *this;
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
    return m_d->m_glyphsReady && m_d->m_layoutReady;
  }

  void TextLayout::invalidate()
  {
    m_d->m_layoutReady = false;
  }

  void TextLayout::generate()
  {
    generateInternal();
  }

  void TextLayout::setMaximumSize(const Nimble::Vector2f & size)
  {
    m_d->m_maximumSize = size;
    m_d->m_layoutReady = false;
    m_d->m_glyphsReady = false;
  }

  Nimble::Vector2f TextLayout::maximumSize() const
  {
    return m_d->m_maximumSize;
  }

  const Nimble::Rectf & TextLayout::boundingBox() const
  {
    generateInternal();
    return m_d->m_boundingBox;
  }

  const Nimble::Vector2f & TextLayout::renderLocation() const
  {
    return m_d->m_renderLocation;
  }

  void TextLayout::setRenderLocation(const Nimble::Vector2f & location)
  {
    m_d->m_renderLocation = location;
  }

  void TextLayout::setBoundingBox(const Nimble::Rectf & bb)
  {
    m_d->m_boundingBox = bb;
  }

  void TextLayout::setLayoutReady(bool v)
  {
    m_d->m_layoutReady = v;
  }

  bool TextLayout::isLayoutReady() const
  {
    return m_d->m_layoutReady;
  }

  void TextLayout::setGlyphsReady(bool v)
  {
    m_d->m_glyphsReady = v;
  }

  void TextLayout::clearGlyphs()
  {
    m_d->m_groupCache.clear();
    m_d->m_groups.clear();
    m_d->m_glyphsReady = false;
  }

  bool TextLayout::generateGlyphs(const Nimble::Vector2f & location,
                                  const QGlyphRun & glyphRun)
  {
    return m_d->generate(location, glyphRun);
  }
} // namespace Luminous
