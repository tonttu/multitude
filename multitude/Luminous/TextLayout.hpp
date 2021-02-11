/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_TEXTLAYOUT_HPP
#define LUMINOUS_TEXTLAYOUT_HPP

#include <Valuable/Node.hpp>

#include <Luminous/RenderCommand.hpp>

#include <Nimble/Rect.hpp>

#include <array>
#include <vector>

#include <QUrl>

class QGlyphRun;
class QTextCharFormat;

namespace Luminous {
  /// TextLayout is the base class for different implementations of text layouting.
  /// It is a node since it sends layout-event
  /// @event[out] layout New layout has been made - boundingBox and other
  ///                    things might have changed.
  class TextLayout : public Valuable::Node
  {
  public:
    /// The bounds for a single glyph in the layout
    struct LUMINOUS_API Item
    {
      std::array<FontVertex, 4> vertices;
    };

    /// @todo clean up and document
    struct Group
    {
      Group(Texture & tex, QColor c) : texture(&tex), color(c) {}
      Texture * texture;
      QColor color;
      std::vector<TextLayout::Item> items;
    };

  public:
    LUMINOUS_API virtual ~TextLayout();

    LUMINOUS_API TextLayout(TextLayout && t);
    LUMINOUS_API TextLayout & operator=(TextLayout && t);

    LUMINOUS_API int groupCount() const;
    LUMINOUS_API Texture * texture(int groupIndex) const;
    LUMINOUS_API const std::vector<Item> & items(int groupIndex) const;
    LUMINOUS_API const Group & group(int groupIndex) const;

    LUMINOUS_API bool isLayoutReady() const;
    LUMINOUS_API bool isComplete() const;
    LUMINOUS_API void generate();

    LUMINOUS_API bool autoGenerate() const;
    LUMINOUS_API void setAutoGenerate(bool autogenerate);

    LUMINOUS_API bool correctAtlas() const;

    LUMINOUS_API void invalidate();

    LUMINOUS_API virtual void setMaximumSize(const Nimble::SizeF & size);
    LUMINOUS_API Nimble::SizeF maximumSize() const;
    /// Returns the bounding box of the text.
    LUMINOUS_API const Nimble::Rectf & boundingBox() const;

    /// This tells how much there is need to offset the vertical position
    /// of the layout. It is necessary for enabling vertical alignment.
    /// @return Offset for rendering this layout
    LUMINOUS_API float verticalOffset() const;

    /// @todo document, maybe rename
    LUMINOUS_API QUrl findUrl(Nimble::Vector2f location) const;

    /// Converts between pixel and point sizes with QFont like Qt does it,
    /// this doesn't necessarily use the actual DPI of the displays or system.
    LUMINOUS_API static float pixelToPointSize(float size);
    LUMINOUS_API static float pointToPixelSize(float size);

  protected:
    LUMINOUS_API void setVerticalOffset(float offset);

    LUMINOUS_API void doGenerateInternal() const;
    LUMINOUS_API bool isGenerating() const;

    LUMINOUS_API TextLayout(const Nimble::SizeF & maximumSize);

    LUMINOUS_API void setBoundingBox(const Nimble::Rectf & bb);
    LUMINOUS_API void setLayoutReady(bool v);
    LUMINOUS_API void setGlyphsReady(bool v);
    LUMINOUS_API void clearGlyphs();
    LUMINOUS_API bool generateGlyphs(const Nimble::Vector2f & location,
                                     const QGlyphRun & glyphRun, int stretch,
                                     const QTextCharFormat * format = nullptr);

  private:
    LUMINOUS_API virtual void generateInternal() const = 0;

    class D;
    std::unique_ptr<D> m_d;
  };
} // namespace Luminous

#endif // LUMINOUS_TEXTLAYOUT_HPP
