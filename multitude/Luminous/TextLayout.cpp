/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "TextLayout.hpp"

#include <Luminous/FontCache.hpp>
#include <Luminous/RenderResource.hpp>
#include <Luminous/Texture.hpp>

#include <QGlyphRun>
#include <QTextCharFormat>

Q_GUI_EXPORT int qt_defaultDpiY();

bool operator<(QColor a, QColor b)
{
  if (a.isValid() && b.isValid()) {
    const uint64_t av = (uint64_t(a.red()) << 48) | (uint64_t(a.green()) << 32) | (uint64_t(a.blue()) << 16) | (uint64_t(a.alpha()));
    const uint64_t bv = (uint64_t(b.red()) << 48) | (uint64_t(b.green()) << 32) | (uint64_t(b.blue()) << 16) | (uint64_t(b.alpha()));
    return av < bv;
  }
  return b.isValid();
}

namespace Luminous
{
  class TextLayout::D
  {
  public:
    D(const Nimble::SizeF & size);

    bool generate(const Nimble::Vector2f & location, const QGlyphRun & glyphRun, const QTextCharFormat * format);

  protected:
    Group & findGroup(Texture & texture, QColor color);

  public:
    Nimble::SizeF m_maximumSize;
    Nimble::Rectf m_boundingBox;
    float m_verticalOffset;
    /// Set to false if we need to do layout again
    bool m_layoutReady;
    /// Do we have all glyphs == no need to call regenerate()
    bool m_glyphsReady;

    bool m_autoGenerate;
    int m_generating;

    std::vector<std::pair<Nimble::Rectf, QUrl> > m_urls;

    int m_atlasGeneration;
    std::map<std::pair<RenderResource::Id, QColor>, int> m_groupCache;
    std::vector<Group> m_groups;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  TextLayout::D::D(const Nimble::SizeF & maximumSize)
    : m_maximumSize(maximumSize)
    , m_verticalOffset(0.f)
    , m_layoutReady(false)
    , m_glyphsReady(false)
    , m_autoGenerate(false)
    , m_generating(0)
    , m_atlasGeneration(-1)
  {
  }

  bool TextLayout::D::generate(const Nimble::Vector2f & layoutLocation,
                               const QGlyphRun & glyphRun, const QTextCharFormat * format)
  {
    bool missingGlyphs = false;

    const QRawFont & font = glyphRun.rawFont();
    const QVector<quint32> & glyphs = glyphRun.glyphIndexes();
    const QVector<QPointF> & positions = glyphRun.positions();

    FontCache & cache = FontCache::acquire(font);

    const float scale = float(font.pixelSize()) / cache.pixelSize();
    const float invsize = 1.0f / float(font.pixelSize());

    QColor color;
    if (format)
      color = format->foreground().color();

    Nimble::Rectf bb;
    for (int i = 0; i < glyphs.size(); ++i) {
      const quint32 glyph = glyphs[i];

      FontCache::Glyph * glyphCache = cache.glyph(font, glyph);
      if (glyphCache) {
        if (glyphCache->isEmpty())
          continue;

        const Nimble::Vector2f location = Nimble::Vector2f(positions[i].x(), positions[i].y()) +
            layoutLocation + glyphCache->location() * scale;
        const Nimble::Vector2f & size = glyphCache->size() * scale;

        Group & g = findGroup(glyphCache->texture(), color);

        TextLayout::Item item;
        bb.expand(location);
        bb.expand(location+size);
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
    if (format && !format->anchorHref().isEmpty())
      m_urls.push_back(std::make_pair(bb, format->anchorHref()));

    return missingGlyphs;
  }

  TextLayout::Group & TextLayout::D::findGroup(Texture & texture, QColor color)
  {
    std::pair<RenderResource::Id, QColor> key = std::make_pair(texture.resourceId(), color);
    auto it = m_groupCache.find(key);
    if (it != m_groupCache.end()) {
      return m_groups[it->second];
    }
    m_groupCache.insert(std::make_pair(key, static_cast<int>(m_groups.size())));
    m_groups.emplace_back(texture, color);
    return m_groups.back();
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  TextLayout::TextLayout(const Nimble::SizeF & maximumSize)
    : m_d(new D(maximumSize))
  {
    eventAddOut("layout");
  }

  TextLayout::~TextLayout()
  {
  }

  TextLayout::TextLayout(TextLayout && t)
    : Node(std::move(t))
    , m_d(std::move(t.m_d))
  {
  }

  TextLayout & TextLayout::operator=(TextLayout && t)
  {
    Node::operator=(std::move(t));
    std::swap(m_d, t.m_d);
    return *this;
  }

  size_t TextLayout::groupCount() const
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

  const TextLayout::Group & TextLayout::group(int groupIndex) const
  {
    return m_d->m_groups[groupIndex];
  }

  bool TextLayout::isComplete() const
  {
    return m_d->m_glyphsReady && m_d->m_layoutReady && correctAtlas();
  }

  void TextLayout::invalidate()
  {
    m_d->m_layoutReady = false;
    if (autoGenerate())
      doGenerateInternal();
  }

  void TextLayout::generate()
  {
    if(!isComplete())
      doGenerateInternal();
  }

  bool TextLayout::autoGenerate() const
  {
    return m_d->m_autoGenerate;
  }

  void TextLayout::setAutoGenerate(bool autogenerate)
  {
    m_d->m_autoGenerate = autogenerate;
  }

  void TextLayout::setMaximumSize(const Nimble::SizeF & size)
  {
    m_d->m_maximumSize = size;
    m_d->m_layoutReady = false;
    m_d->m_glyphsReady = false;
    if (autoGenerate() && m_d->m_generating == 0)
      doGenerateInternal();
  }

  Nimble::SizeF TextLayout::maximumSize() const
  {
    return m_d->m_maximumSize;
  }

  const Nimble::Rectf & TextLayout::boundingBox() const
  {
    // Updates the internal state if needed so the user gets
    // always the current state
    if (!isLayoutReady())
      doGenerateInternal();
    return m_d->m_boundingBox;
  }

  QUrl TextLayout::findUrl(Nimble::Vector2f location) const
  {
    for (auto p: m_d->m_urls)
      if (p.first.contains(location))
        return p.second;
    return QUrl();
  }

  float TextLayout::verticalOffset() const
  {
    return m_d->m_verticalOffset;
  }

  void TextLayout::setVerticalOffset(float offset)
  {
    m_d->m_verticalOffset = offset;
  }

  void TextLayout::setBoundingBox(const Nimble::Rectf & bb)
  {
    m_d->m_boundingBox = bb;
  }

  void TextLayout::setLayoutReady(bool v)
  {
    if (v == m_d->m_layoutReady)
      return;
    m_d->m_layoutReady = v;
    if (v)
      eventSend("layout");
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
    m_d->m_atlasGeneration = FontCache::generation();
  }

  bool TextLayout::correctAtlas() const
  {
    return m_d->m_atlasGeneration == FontCache::generation();
  }

  bool TextLayout::generateGlyphs(const Nimble::Vector2f & location,
                                  const QGlyphRun & glyphRun, const QTextCharFormat * format)
  {
    if (glyphRun.glyphIndexes().isEmpty())
      return false;
    return m_d->generate(location, glyphRun, format);
  }

  float TextLayout::pixelToPointSize(float pixelSize)
  {
    // 72 would be the correct value here, but using it would break all existing applications
    return pixelSize * 96.f / qt_defaultDpiY();
  }

  float TextLayout::pointToPixelSize(float pointSize)
  {
    return pointSize / 96.f * qt_defaultDpiY();
  }

  void TextLayout::doGenerateInternal() const
  {
    ++m_d->m_generating;
    generateInternal();
    --m_d->m_generating;
  }

  bool TextLayout::isGenerating() const
  {
    return m_d->m_generating > 0;
  }
} // namespace Luminous
