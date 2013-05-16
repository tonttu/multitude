/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "SimpleTextLayout.hpp"

#include <Radiant/Mutex.hpp>

#include <Valuable/StyleValue.hpp>

#include <Luminous/RenderManager.hpp>

#include <QFontMetricsF>
#include <QTextLayout>
#include <QRegExp>
#include <QThread>

#include <unordered_map>
#include <memory>
#include <tuple>

typedef std::unique_ptr<Luminous::SimpleTextLayout> LayoutPtr;

/// MSVC2010 has something weird going on with tuples and pairs so we use the old-fashioned alternative here
#if defined (RADIANT_MSVC10)
struct LayoutCacheKey {
  LayoutCacheKey(const QString & text, const Nimble::Vector2i & v, const QFont & font, const QTextOption & option, unsigned thread)
    : text(text), v(v), font(font), option(option), thread(thread)
  {
  }

  QString text;
  Nimble::Vector2i v;
  QFont font;
  QTextOption option;
  unsigned thread;
};

namespace std
{
  template<> struct hash<LayoutCacheKey>
  {
    inline size_t operator()(const LayoutCacheKey & tuple) const
    {
      std::hash<uint> hasher;
      return qHash(tuple.text) ^ hasher(tuple.v.x) ^
        hasher(tuple.v.y) ^ qHash(tuple.font.key()) ^
        hasher(tuple.option.alignment()) ^ hasher(tuple.thread);
    }
  };
}

bool operator==(const LayoutCacheKey & lhs, const LayoutCacheKey & rhs)
{
  return std::hash<LayoutCacheKey>()(lhs) == std::hash<LayoutCacheKey>()(rhs);
}

#else
typedef std::tuple<QString, Nimble::Size, QFont, QTextOption, unsigned> LayoutCacheKey;


namespace std
{
  template<> struct hash<LayoutCacheKey>
  {
    inline size_t operator()(const LayoutCacheKey & tuple) const
    {
      std::hash<uint> hasher;
      return qHash(std::get<0>(tuple)) ^ hasher(std::get<1>(tuple).width()) ^
          hasher(std::get<1>(tuple).height()) ^ qHash(std::get<2>(tuple).key()) ^
          hasher(std::get<3>(tuple).alignment()) ^ hasher(std::get<4>(tuple));
    }
  };
}
#endif


bool operator==(const QTextOption & o1, const QTextOption & o2)
{
  return int(o1.alignment()) == int(o2.alignment()) &&
      o1.flags() == o2.flags() &&
      o1.tabStop() == o2.tabStop() &&
      o1.tabs() == o2.tabs() &&
      o1.textDirection() == o2.textDirection() &&
      o1.useDesignMetrics() == o2.useDesignMetrics() &&
      o1.wrapMode() == o2.wrapMode();
}

namespace
{
  Radiant::Mutex s_layoutCacheMutex;
  std::unordered_map<LayoutCacheKey, LayoutPtr> s_layoutCache;
}

namespace Luminous
{
  class SimpleTextLayout::D
  {
  public:
    D();
    D(const QTextLayout & copy);

    void layout(const Nimble::SizeF & size);

  public:
    Radiant::Mutex m_generateMutex;
    Valuable::StyleValue m_lineHeight;
    Valuable::StyleValue m_letterSpacing;
    QTextLayout m_layout;
    QRectF m_boundingBox;
    QThread * m_layoutThread;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  SimpleTextLayout::D::D()
    : m_layoutThread(nullptr)
  {
    QFont font;
    font.setHintingPreference(QFont::PreferNoHinting);
    m_layout.setFont(font);
  }

  SimpleTextLayout::D::D(const QTextLayout & copy)
    : m_layout(copy.text(), copy.font())
    , m_layoutThread(nullptr)
  {
    m_layout.setTextOption(copy.textOption());
  }

  void SimpleTextLayout::D::layout(const Nimble::SizeF & size)
  {
    const float lineWidth = size.width();

    bool forceHeight = false;
    float height = 0.0f;
    float heightFactor = 1.0f;
    float y = 0;

    if (m_lineHeight.size() == 1) {
      if (m_lineHeight.unit() == Valuable::Attribute::VU_PXS) {
        forceHeight = true;
        height = m_lineHeight.asFloat();
      } else if (m_lineHeight.unit() == Valuable::Attribute::VU_UNKNOWN ||
                 m_lineHeight.unit() == Valuable::Attribute::VU_PERCENTAGE) {
        heightFactor = m_lineHeight.asFloat();
      }
    }

    assert(m_layout.font().hintingPreference() == QFont::PreferNoHinting);

    QFont font = m_layout.font();
    if (m_letterSpacing.size() == 1) {
      if (m_letterSpacing.unit() == Valuable::Attribute::VU_PERCENTAGE) {
        font.setLetterSpacing(QFont::PercentageSpacing, m_letterSpacing.asFloat() * 100.0f);
      } else {
        font.setLetterSpacing(QFont::AbsoluteSpacing, m_letterSpacing.asFloat());
      }
    } else {
      font.setLetterSpacing(QFont::PercentageSpacing, 100.0f);
    }
    m_layout.setFont(font);

    QFontMetricsF fontMetrics(font);
    const float leading = fontMetrics.leading();

    m_boundingBox = QRectF();
    m_layout.beginLayout();
    while (true) {
      QTextLine line = m_layout.createLine();
      if (!line.isValid())
        break;

      line.setLineWidth(lineWidth);
      y += leading;
      line.setPosition(QPointF(0, y));
      if (forceHeight)
        y += height;
      else
        y += line.height() * heightFactor;

      m_boundingBox |= line.naturalTextRect();
    }

    m_layout.endLayout();
    if (m_layout.text().isEmpty())
      m_boundingBox = QRectF();

    m_layoutThread = QThread::currentThread();

#if 0
    static QAtomicInt id;
    QImage dbgimg(m_layout.boundingRect().width(), m_layout.boundingRect().height(),
                  QImage::Format_ARGB32_Premultiplied);
    dbgimg.fill(Qt::white);
    QPainter painter(&dbgimg);
    m_layout.draw(&painter, QPoint(0, 0));
    dbgimg.save(QString("img-%1.png").arg(id.fetchAndAddOrdered(1)));

    foreach (const QGlyphRun & glyphRun, m_layout.glyphRuns()) {
      const QRawFont & rawFont = glyphRun.rawFont();
      const QVector<quint32> & glyphs = glyphRun.glyphIndexes();
      // const QVector<QPointF> & positions = glyphRun.positions();
      for (int i = 0; i < glyphs.size(); ++i) {
        QImage dbgimg2(500, 500, QImage::Format_ARGB32_Premultiplied);
        dbgimg2.fill(Qt::white);
        QPainter painter2(&dbgimg2);

        painter2.setRenderHint(QPainter::Antialiasing, true);
        painter2.setRenderHint(QPainter::TextAntialiasing, true);
        painter2.setRenderHint(QPainter::HighQualityAntialiasing, true);
        painter2.setPen(Qt::NoPen);
        painter2.setBrush(QBrush(Qt::black));
        QPainterPath path = rawFont.pathForGlyph(glyphs[i]);
        path.translate(250, 250);
        painter2.drawPath(path);

        dbgimg2.save(QString("glyph-%1.png").arg(id.fetchAndAddOrdered(1)));
        if (id > 10)
          return;
      }
    }
#endif
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  SimpleTextLayout::SimpleTextLayout()
    : TextLayout(Nimble::SizeF(100, 100))
    , m_d(new D())
  {
  }

  SimpleTextLayout::SimpleTextLayout(const SimpleTextLayout & that)
    : TextLayout(that.maximumSize())
    , m_d(new D(that.m_d->m_layout))
  {
    m_d->m_letterSpacing = that.m_d->m_letterSpacing;
    m_d->m_lineHeight = that.m_d->m_lineHeight;
  }

  SimpleTextLayout::SimpleTextLayout(const QString & text, const Nimble::SizeF & maximumSize,
                                     const QFont & font, const QTextOption & textOption)
    : TextLayout(maximumSize)
    , m_d(new D())
  {
    setFont(font);
    setTextOption(textOption);
    setText(text);
  }

  SimpleTextLayout::~SimpleTextLayout()
  {
    delete m_d;
  }

  void SimpleTextLayout::setText(const QString & text)
  {
    QString tmp = text;
    tmp.replace(QRegExp("\\r\\n|\\n|\\r"), QString(QChar::LineSeparator));
    m_d->m_layout.setText(tmp);
    invalidate();
  }

  QTextOption SimpleTextLayout::textOption() const
  {
    return m_d->m_layout.textOption();
  }

  void SimpleTextLayout::setTextOption(const QTextOption & textOption)
  {
    m_d->m_layout.setTextOption(textOption);
    invalidate();
  }

  QFont SimpleTextLayout::font() const
  {
    return m_d->m_layout.font();
  }

  void SimpleTextLayout::setFont(const QFont & font)
  {
    QFont tmp = font;
    tmp.setHintingPreference(QFont::PreferNoHinting);
    m_d->m_layout.setFont(tmp);
    invalidate();
  }

  void SimpleTextLayout::setLineHeight(const Valuable::StyleValue & height)
  {
    if (m_d->m_lineHeight == height)
      return;
    m_d->m_lineHeight = height;
    invalidate();
  }

  const Valuable::StyleValue & SimpleTextLayout::lineHeight() const
  {
    return m_d->m_lineHeight;
  }

  void SimpleTextLayout::setLetterSpacing(const Valuable::StyleValue & letterSpacing)
  {
    if (m_d->m_letterSpacing == letterSpacing)
      return;
    m_d->m_letterSpacing = letterSpacing;
    invalidate();
  }

  const Valuable::StyleValue & SimpleTextLayout::letterSpacing() const
  {
    return m_d->m_letterSpacing;
  }

  QTextLayout & SimpleTextLayout::layout()
  {
    return m_d->m_layout;
  }

  const QTextLayout & SimpleTextLayout::layout() const
  {
    return m_d->m_layout;
  }

  const SimpleTextLayout & SimpleTextLayout::cachedLayout(const QString & text,
                                                          const Nimble::SizeF & size,
                                                          const QFont & font,
                                                          const QTextOption & option)
  {
    SimpleTextLayout * layout;

    {
      /// @todo someone should also delete old layouts..
#if defined (RADIANT_MSVC10)
      const auto & key = LayoutCacheKey(text, size.cast<int>(), font, option, RenderManager::threadIndex());
#else
      const auto & key = std::make_tuple(text, size.cast<int>(), font, option, RenderManager::threadIndex());
#endif

      Radiant::Guard g(s_layoutCacheMutex);
      std::unique_ptr<SimpleTextLayout> & tptr = s_layoutCache[key];

      bool reset = tptr && !tptr->correctAtlas();
      if(reset) {
        s_layoutCache.clear();
      }
      std::unique_ptr<SimpleTextLayout> & ptr = reset?s_layoutCache[key]:tptr;

      if (!ptr) {
        ptr.reset(new SimpleTextLayout(text, size, font, option));
      }
      layout = ptr.get();
    }

    layout->generate();

    return *layout;
  }

  void SimpleTextLayout::generateInternal() const
  {
    Radiant::Guard g(m_d->m_generateMutex);

    SimpleTextLayout *nonConst = const_cast<SimpleTextLayout*>(this);
    if (!isLayoutReady() || m_d->m_layoutThread != QThread::currentThread()) {
      m_d->layout(maximumSize());
      // We need to avoid calling bounding box here, becourse it calls generateInternal
      nonConst->setBoundingBox(m_d->m_boundingBox);

      float contentHeigth = m_d->m_boundingBox.height();
      if(m_d->m_layout.text().isEmpty()) {
        QTextLine line = m_d->m_layout.lineAt(0);
        if(line.isValid())
          contentHeigth = line.height();
      }

      nonConst->setLayoutReady(true);
      nonConst->clearGlyphs();
    }

    if (isComplete())
      return;

    nonConst->clearGlyphs();

    bool missingGlyphs = false;
    const Nimble::Vector2f layoutLocation(m_d->m_layout.position().x(), m_d->m_layout.position().y());

    foreach (const QGlyphRun & glyphRun, m_d->m_layout.glyphRuns())
      missingGlyphs |= nonConst->generateGlyphs(layoutLocation, glyphRun);

    nonConst->setGlyphsReady(!missingGlyphs);
  }
} // namespace Luminous
