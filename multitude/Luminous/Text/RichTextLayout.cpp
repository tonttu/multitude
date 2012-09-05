#include "RichTextLayout.hpp"

#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>
#include <QTextList>

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
    QTextDocument m_doc;

  private slots:
    void changed();
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  RichTextLayout::D::D(RichTextLayout & host)
    : m_host(host)
  {
    connect(&m_doc, SIGNAL(documentLayoutChanged()), this, SLOT(changed()));
  }

  void RichTextLayout::D::disableHinting()
  {
    QTextCursor cursor(&m_doc);
    for (QTextBlock block = m_doc.begin(); block.isValid(); block = block.next()) {
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

  void RichTextLayout::generate()
  {
    if (!isLayoutReady()) {
      // trigger relayout in Qt
      m_d->disableHinting();
      m_d->m_doc.setTextWidth(maximumSize().x);
      QSizeF size = m_d->m_doc.documentLayout()->documentSize();
      setBoundingBox(Nimble::Rectf(0, 0, size.width(), size.height()));

      setLayoutReady(true);
      clearGlyphs();
    }

    if (isComplete())
      return;

    clearGlyphs();

    bool missingGlyphs = false;

    for (QTextBlock block = m_d->m_doc.begin(); block.isValid(); block = block.next()) {
      QTextLayout * layout = block.layout();
      const Nimble::Vector2f layoutLocation(layout->position().x(), layout->position().y());
      foreach (const QGlyphRun & glyphRun, layout->glyphRuns())
        missingGlyphs |= generateGlyphs(layoutLocation, glyphRun);
    }

    QAbstractTextDocumentLayout * layout = m_d->m_doc.documentLayout();
    for (int i = 0; ; ++i) {
      QTextObject * obj = m_d->m_doc.object(i);
      if (!obj) break;
      QTextList * lst = dynamic_cast<QTextList*>(obj);
      if (!lst) continue;

      QTextListFormat fmt = lst->format();
      for (int j = 0; j < lst->count(); ++j) {
        QTextBlock block = lst->item(j);
        QRectF rect = layout->blockBoundingRect(block);
        QTextLayout textLayout("∙", block.charFormat().font());
        int size = textLayout.font().pixelSize();

        textLayout.beginLayout();
        QTextLine line = textLayout.createLine();
        int indent = m_d->m_doc.indentWidth() * fmt.indent();
        line.setLineWidth(size);
        line.setPosition(QPointF(0, 0));
        textLayout.endLayout();
        QRectF bullet = textLayout.boundingRect();

        foreach (const QGlyphRun & glyphRun, textLayout.glyphRuns())
          missingGlyphs |= generateGlyphs(Nimble::Vector2f(rect.left() + indent - bullet.right() * 1.5,
                                                           rect.top() - bullet.top()), glyphRun);
      }
    }

    setGlyphsReady(!missingGlyphs);
  }

  QTextDocument & RichTextLayout::document()
  {
    return m_d->m_doc;
  }

  const QTextDocument & RichTextLayout::document() const
  {
    return m_d->m_doc;
  }

} // namespace Luminous

#include "RichTextLayout.moc"
