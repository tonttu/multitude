#include "RichTextLayout.hpp"

#include <QTextDocument>
#include <QTextCursor>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>

namespace Luminous
{
  class RichTextLayout::D
  {
  public:
    D();

    void disableKerning();

  public:
    QTextDocument m_doc;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  RichTextLayout::D::D()
  {}

  void RichTextLayout::D::disableKerning()
  {
    QTextCursor cursor(&m_doc);
    for (QTextBlock block = m_doc.begin(); block.isValid(); block = block.next()) {
      for (auto it = block.begin(); it != block.end(); ++it) {
        QTextFragment fragment = it.fragment();
        if (!fragment.isValid())
          continue;

        QTextCharFormat fmt = fragment.charFormat();
        QFont font = fmt.font();
        font.setKerning(false);
        fmt.setFont(font);

        cursor.setPosition(fragment.position());
        cursor.setPosition(fragment.position() + fragment.length(), QTextCursor::KeepAnchor);
        cursor.setCharFormat(fmt);
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  RichTextLayout::RichTextLayout(const Nimble::Vector2f & size)
    : TextLayout(size)
    , m_d(new D())
  {
  }

  RichTextLayout::~RichTextLayout()
  {
    delete m_d;
  }

  void RichTextLayout::generate()
  {
    if (!layoutReady()) {
      // trigger relayout in Qt
      m_d->disableKerning();
      m_d->m_doc.setTextWidth(maximumSize().x);
      QSizeF size = m_d->m_doc.documentLayout()->documentSize();
      setBoundingBox(Nimble::Rectf(0, 0, size.width(), size.height()));

      setLayoutReady(true);
      clearGlyphs();
    }

    if (isComplete())
      return;

    bool missingGlyphs = false;

    for (QTextBlock block = m_d->m_doc.begin(); block.isValid(); block = block.next()) {
      QTextLayout * layout = block.layout();
      const Nimble::Vector2f layoutLocation(layout->position().x(), layout->position().y());
      foreach (const QGlyphRun & glyphRun, layout->glyphRuns())
        missingGlyphs |= generateGlyphs(layoutLocation, glyphRun);
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