#ifndef LUMINOUS_TEXTLAYOUT_HPP
#define LUMINOUS_TEXTLAYOUT_HPP

#include <Luminous/RenderCommand.hpp>

#include <Nimble/Rect.hpp>

#include <array>
#include <vector>

class QGlyphRun;

namespace Luminous {
  
  class TextLayout
  {
  public:
    struct LUMINOUS_API Item
    {
      std::array<FontVertex, 4> vertices;
    };

  public:
    LUMINOUS_API virtual ~TextLayout();

    LUMINOUS_API int groupCount() const;
    LUMINOUS_API Texture * texture(int groupIndex) const;
    LUMINOUS_API const std::vector<Item> & items(int groupIndex) const;

    LUMINOUS_API bool isLayoutReady() const;
    LUMINOUS_API bool isComplete() const;
    /// Not thread safe
    LUMINOUS_API virtual void generate() = 0;

    LUMINOUS_API void invalidate();

    LUMINOUS_API virtual void setMaximumSize(const Nimble::Vector2f & size);
    LUMINOUS_API Nimble::Vector2f maximumSize() const;

    LUMINOUS_API const Nimble::Rectf & boundingBox() const;

    LUMINOUS_API const Nimble::Vector2f & renderLocation() const;

  protected:
    LUMINOUS_API TextLayout(const Nimble::Vector2f & maximumSize);

    LUMINOUS_API void setRenderLocation(const Nimble::Vector2f & location);
    LUMINOUS_API void setBoundingBox(const Nimble::Rectf & bb);
    LUMINOUS_API void setLayoutReady(bool v);
    LUMINOUS_API void setGlyphsReady(bool v);
    LUMINOUS_API void clearGlyphs();
    LUMINOUS_API bool generateGlyphs(const Nimble::Vector2f & location,
                                     const QGlyphRun & glyphRun);

  private:
    class D;
    D * m_d;
  };
} // namespace Luminous

#endif // LUMINOUS_TEXTLAYOUT_HPP
