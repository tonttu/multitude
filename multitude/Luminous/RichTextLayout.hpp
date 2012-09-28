#ifndef LUMINOUS_RICH_TEXT_LAYOUT_HPP
#define LUMINOUS_RICH_TEXT_LAYOUT_HPP

#include "TextLayout.hpp"

class QTextDocument;

namespace Luminous
{
  /// Rich text document layout
  class RichTextLayout : public TextLayout
  {
  public:
    LUMINOUS_API RichTextLayout(const Nimble::Vector2f & size = Nimble::Vector2f(100, 100));
    LUMINOUS_API virtual ~RichTextLayout();

    LUMINOUS_API virtual void generate() OVERRIDE;

    LUMINOUS_API QTextDocument & document();
    LUMINOUS_API const QTextDocument & document() const;

  private:
    class D;
    D * m_d;
  };
} // namespace Luminous

#endif
