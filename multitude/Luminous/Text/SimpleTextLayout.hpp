#ifndef LUMINOUS_SIMPLE_TEXT_LAYOUT_HPP
#define LUMINOUS_SIMPLE_TEXT_LAYOUT_HPP

#include "TextLayout.hpp"

class QTextOption;
class QFont;
class QTextLayout;

namespace Luminous
{
  /// Plain text, usually one font, inside rectangle (0,0) -> size
  class SimpleTextLayout : public TextLayout
  {
  public:
    LUMINOUS_API SimpleTextLayout();
    LUMINOUS_API SimpleTextLayout(const QString & text, const Nimble::Vector2f & maximumSize,
                                  const QFont & font, const QTextOption & textOption);
    LUMINOUS_API virtual ~SimpleTextLayout();

    /// If the QTextLayout is modified, it's required to call invalidate() manually
    LUMINOUS_API QTextLayout & layout();
    LUMINOUS_API const QTextLayout & layout() const;

    LUMINOUS_API static const SimpleTextLayout & cachedLayout(const QString & text,
                                                              const Nimble::Vector2f & size,
                                                              const QFont & font,
                                                              const QTextOption & option);

    LUMINOUS_API virtual void generate() OVERRIDE;

  private:
    class D;
    D * m_d;
  };
} // namespace Luminous

#endif // LUMINOUS_SIMPLE_TEXT_LAYOUT_HPP
