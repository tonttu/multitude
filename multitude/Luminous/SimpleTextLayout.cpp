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

#include <Radiant/BGThread.hpp>
#include <Radiant/Mutex.hpp>
#include <Radiant/Task.hpp>

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
#ifndef RADIANT_WINDOWS
      // On Windows we need to control this manually because it conflicts with
      // font-stretch attribute, so do not use it as a part of cache key.
      o1.useDesignMetrics() == o2.useDesignMetrics() &&
#endif
      o1.wrapMode() == o2.wrapMode();
}

namespace
{
  struct CachedLayout
  {
    LayoutPtr layout;
    /// In deciseconds, see Luminous::RenderManager::frameTime
    int lastUsed = 0;
  };

  /// Cache expiration time in deciseconds (40 seconds)
  static const int s_cacheExpireTime = 400;
  /// How often to check for expired cached layouts in seconds
  static const int s_cacheExpirePollingInterval = 41;

  Radiant::Mutex s_layoutCacheMutex;
  std::unordered_map<LayoutCacheKey, CachedLayout> s_layoutCache;
  Radiant::TaskPtr s_cacheReleaseTask;

  const float s_defaultLineHeight = 1.0f;
  const float s_defaultLetterSpacing = 1.0f;

  static void clearUnusedLayoutsFromCache()
  {
    int threshold = Luminous::RenderManager::lastFrameTime() - s_cacheExpireTime;
    if (threshold <= 0) return;

    Radiant::Guard g(s_layoutCacheMutex);
    for (auto it = s_layoutCache.cbegin(); it != s_layoutCache.cend();) {
      const CachedLayout & cache = it->second;
      if (cache.lastUsed < threshold) {
        it = s_layoutCache.erase(it);
      } else {
        ++it;
      }
    }
  }
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
    float m_indent;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  SimpleTextLayout::D::D()
    : m_generateMutex(true),
      m_layoutThread(nullptr),
      m_indent(0)
  {
    QFont font;
    font.setHintingPreference(QFont::PreferNoHinting);
    QTextOption textOption;
    textOption.setUseDesignMetrics(true);
    m_layout.setFont(font);
    m_layout.setTextOption(textOption);
  }

  SimpleTextLayout::D::D(const QTextLayout & copy)
    : m_layout(copy.text(), copy.font())
    , m_layoutThread(nullptr)
    , m_indent(0)
  {
    m_layout.setTextOption(copy.textOption());
  }

  void SimpleTextLayout::D::layout(const Nimble::SizeF & size)
  {
    const float lineWidth = size.width();

    bool forceHeight = false;
    float height = 0.0f;
    float heightFactor = s_defaultLineHeight;
    float y = 0;

    if (m_lineHeight.size() == 1) {
      if (m_lineHeight.unit() == Valuable::Attribute::VU_PXS) {
        forceHeight = true;
        height = m_lineHeight.asFloat();
      } else if (m_lineHeight.unit() == Valuable::Attribute::VU_PERCENTAGE) {
        heightFactor = m_lineHeight.asFloat();
      } else if (m_lineHeight.unit() == Valuable::Attribute::VU_UNKNOWN) {
        if(m_lineHeight.isNumber())
          heightFactor = m_lineHeight.asFloat();
        else if (m_lineHeight.asKeyword() == "normal")
          heightFactor = s_defaultLineHeight;
        else
          Radiant::error("Not a valid value for line-height: %s",
                         m_lineHeight.stringify().toUtf8().data());
      } else {
        Radiant::error("Unsupported unit for line-height: %s",
                       m_lineHeight.stringify().toUtf8().data());
      }
    }

    assert(m_layout.font().hintingPreference() == QFont::PreferNoHinting);

    QFont font = m_layout.font();
    if (m_letterSpacing.size() == 1) {
      if (m_letterSpacing.unit() == Valuable::Attribute::VU_PXS) {
        font.setLetterSpacing(QFont::AbsoluteSpacing, m_letterSpacing.asFloat());
      } else if (m_letterSpacing.unit() == Valuable::Attribute::VU_PERCENTAGE) {
        font.setLetterSpacing(QFont::PercentageSpacing, m_letterSpacing.asFloat() * 100.0f);
      } else if (m_letterSpacing.unit() == Valuable::Attribute::VU_UNKNOWN) {
        if(m_letterSpacing.asKeyword() == "normal")
          font.setLetterSpacing(QFont::PercentageSpacing, s_defaultLetterSpacing * 100.0f);
        else
          Radiant::error("Not a valid value for letter-spacing: %s",
                         m_letterSpacing.stringify().toUtf8().data());
      } else {
        Radiant::error("Unsupported unit for letter-spacing: %s",
                       m_letterSpacing.stringify().toUtf8().data());
      }
    } else {
      font.setLetterSpacing(QFont::PercentageSpacing, s_defaultLetterSpacing * 100.0f);
    }

#ifdef RADIANT_WINDOWS
    // If enabling design metrics and font stretching the whole layout will collapse.
    // Also the layouting will be bad if not using design metrics with small fonts
    // so this is resolved using somewhat questionable heuristic
    if(font.stretch() != QFont::Unstretched) {
      if(font.pointSizeF() >= 10.f) {
        auto option = m_layout.textOption();
        option.setUseDesignMetrics(false);
        m_layout.setTextOption(option);
      } else {
        font.setStretch(QFont::Unstretched);
      }
    }
#endif
    m_layout.setFont(font);

    QFontMetricsF fontMetrics(font);
    const float leading = fontMetrics.leading();

    m_boundingBox = QRectF();
    m_layout.beginLayout();
    float indent = m_indent;
    while (true) {
      QTextLine line = m_layout.createLine();
      if (!line.isValid())
        break;

      // first line has more or less available width due to the indent
      line.setLineWidth(lineWidth - indent);
      line.setPosition(QPointF(indent, y));
      if (forceHeight)
        y += height;
      else
        y += line.height() * heightFactor;
      y += leading;

      m_boundingBox |= line.naturalTextRect();
      indent = 0;
    }

    m_layout.endLayout();

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
    m_d->m_indent = that.m_d->m_indent;
  }

  SimpleTextLayout::SimpleTextLayout(SimpleTextLayout && that)
    : TextLayout(std::move(that)),
      m_d(std::move(that.m_d))
  {
  }

  SimpleTextLayout & SimpleTextLayout::operator=(SimpleTextLayout && that)
  {
    TextLayout::operator=(std::move(that));
    m_d = std::move(that.m_d);
    return *this;
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
  }

  QString SimpleTextLayout::text() const
  {
    return m_d->m_layout.text();
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
    QTextOption copy = textOption;
    copy.setUseDesignMetrics(true);
    m_d->m_layout.setTextOption(copy);
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

  void SimpleTextLayout::setIndent(float indent)
  {
    if (m_d->m_indent == indent)
      return;
    m_d->m_indent = indent;
    invalidate();
  }

  float SimpleTextLayout::indent() const
  {
    return m_d->m_indent;
  }

  QTextLayout & SimpleTextLayout::layout()
  {
    return m_d->m_layout;
  }

  const QTextLayout & SimpleTextLayout::layout() const
  {
    return m_d->m_layout;
  }

  void SimpleTextLayout::clearCache()
  {
    if (s_cacheReleaseTask) {
      Radiant::BGThread::instance()->removeTask(s_cacheReleaseTask);
      s_cacheReleaseTask.reset();
    }
    Radiant::Guard g(s_layoutCacheMutex);
    s_layoutCache.clear();
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

      const int now = Luminous::RenderManager::frameTime();
      Radiant::Guard g(s_layoutCacheMutex);

      if (!s_cacheReleaseTask) {
        s_cacheReleaseTask = std::make_shared<Radiant::FunctionTask>([] (Radiant::Task & task) {
          clearUnusedLayoutsFromCache();
          task.scheduleFromNowSecs(s_cacheExpirePollingInterval);
        });
        s_cacheReleaseTask->scheduleFromNowSecs(s_cacheExpirePollingInterval);
        Radiant::BGThread::instance()->addTask(s_cacheReleaseTask);
      }

      CachedLayout & cache = s_layoutCache[key];

      if (!cache.layout) {
        cache.layout.reset(new SimpleTextLayout(text, size, font, option));
      }
      cache.lastUsed = now;
      layout = cache.layout.get();
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

      auto align = m_d->m_layout.textOption().alignment();
      if (align & Qt::AlignBottom) {
        nonConst->setVerticalOffset(maximumSize().height() - contentHeigth);
      } else if (align & Qt::AlignVCenter) {
        // Align empty text so that the first line is in the vertical middle.
        nonConst->setVerticalOffset(.5f * (maximumSize().height() - contentHeigth));
      } else {
        nonConst->setVerticalOffset(0.f);
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
