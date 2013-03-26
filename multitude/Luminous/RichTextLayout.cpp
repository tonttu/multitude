/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "RichTextLayout.hpp"

#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>
#include <QTextList>

#include <memory>

namespace Luminous
{
  class RichTextLayout::D : public QObject
  {
    Q_OBJECT

  public:
    D(RichTextLayout & host);

    void disableHinting();

  public:
    RichTextLayout & m_host;
    std::unique_ptr<QTextDocument> m_doc;

    QTextDocument & doc();

  private slots:
    void changed();
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  RichTextLayout::D::D(RichTextLayout & host)
    : m_host(host)
  {
  }

  void RichTextLayout::D::disableHinting()
  {
    QTextCursor cursor(&doc());
    for (QTextBlock block = doc().begin(); block.isValid(); block = block.next()) {
      for (auto it = block.begin(); it != block.end(); ++it) {
        QTextFragment fragment = it.fragment();
        if (!fragment.isValid())
          continue;

        QTextCharFormat fmt = fragment.charFormat();
        QFont font = fmt.font();
        font.setHintingPreference(QFont::PreferNoHinting);
        fmt.setFont(font);

        cursor.setPosition(fragment.position());
        cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
        cursor.setCharFormat(fmt);
      }
    }
  }

  QTextDocument & RichTextLayout::D::doc()
  {
    if (m_doc)
      return *m_doc;

    m_doc.reset(new QTextDocument());
    connect(m_doc.get(), SIGNAL(contentsChanged()), this, SLOT(changed()));
    connect(m_doc.get(), SIGNAL(documentLayoutChanged()), this, SLOT(changed()));
    return *m_doc;
  }

  void RichTextLayout::D::changed()
  {
    m_host.setLayoutReady(false);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  RichTextLayout::RichTextLayout(const Nimble::Vector2f & size)
    : TextLayout(size)
    , m_d(new D(*this))
  {
  }

  RichTextLayout::~RichTextLayout()
  {
    delete m_d;
  }

  RichTextLayout::RichTextLayout(RichTextLayout && t)
    : TextLayout(std::move(t))
    , m_d(t.m_d)
  {
    t.m_d = nullptr;
  }

  RichTextLayout & RichTextLayout::operator=(RichTextLayout && t)
  {
    TextLayout::operator=(std::move(t));
    std::swap(m_d, t.m_d);
    return *this;
  }

  void RichTextLayout::generateInternal() const
  {
    RichTextLayout *nonConst = const_cast<RichTextLayout*>(this);
    if (!isLayoutReady()) {
      // trigger relayout in Qt
      m_d->disableHinting();
      m_d->doc().setTextWidth(maximumSize().x);
      QSizeF size = m_d->doc().documentLayout()->documentSize();
      nonConst->setBoundingBox(Nimble::Rectf(0, 0, size.width(), size.height()));

      nonConst->setLayoutReady(true);
      nonConst->clearGlyphs();
    }

    if (isComplete())
      return;

    nonConst->clearGlyphs();

    bool missingGlyphs = false;
    QAbstractTextDocumentLayout * layout = m_d->doc().documentLayout();

    for (QTextBlock block = m_d->doc().begin(); block.isValid(); block = block.next()) {
      QTextLayout * textLayout = block.layout();
      QRectF rect = layout->blockBoundingRect(block);

      const Nimble::Vector2f layoutLocation(rect.left(), rect.top());
      foreach (const QGlyphRun & glyphRun, textLayout->glyphRuns())
        missingGlyphs |= nonConst->generateGlyphs(layoutLocation, glyphRun);
    }

    QList<int> indices;
    for (const QTextFormat & fmt: m_d->doc().allFormats()) {
      int idx = fmt.objectIndex();
      if (idx >= 0)
        indices << idx;
    }

    for (int i: indices) {
      QTextObject * obj = m_d->doc().object(i);
      if (!obj) continue;
      QTextList * lst = dynamic_cast<QTextList*>(obj);
      if (!lst) continue;

      QTextListFormat fmt = lst->format();
      for (int j = 0; j < lst->count(); ++j) {
        QTextBlock block = lst->item(j);
        QRectF rect = layout->blockBoundingRect(block);
        const bool rtl = block.layout()->textOption().textDirection() == Qt::RightToLeft;

        QTextLayout textLayout("∙", block.charFormat().font());
        int size = textLayout.font().pixelSize();

        textLayout.beginLayout();
        QTextLine line = textLayout.createLine();
        int indent = m_d->doc().indentWidth() * fmt.indent();
        line.setLineWidth(size);
        line.setPosition(QPointF(0, 0));
        textLayout.endLayout();
        QRectF bullet = textLayout.boundingRect();

        Nimble::Vector2f loc;
        loc.y = rect.top() - bullet.top();
        if (rtl) {
          loc.x = rect.right() + bullet.right() * 1.5;
        } else {
          loc.x = rect.left() + indent - bullet.right() * 1.5;
        }

        foreach (const QGlyphRun & glyphRun, textLayout.glyphRuns())
          missingGlyphs |= nonConst->generateGlyphs(loc, glyphRun);
      }
    }

    nonConst->setGlyphsReady(!missingGlyphs);
  }

  QTextDocument & RichTextLayout::document()
  {
    return m_d->doc();
  }

  const QTextDocument & RichTextLayout::document() const
  {
    return m_d->doc();
  }

} // namespace Luminous

#include "RichTextLayout.moc"
