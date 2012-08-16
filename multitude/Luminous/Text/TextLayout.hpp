#ifndef LUMINOUS_TEXTLAYOUT_HPP
#define LUMINOUS_TEXTLAYOUT_HPP

#include <Luminous/VertexHolder.hpp>
#include <Luminous/TextureAtlas.hpp>

#include <Nimble/Vector2.hpp>

#include <array>

class QRawFont;
class QGlyphRun;
class QTextDocument;
class QTextOption;
class QFont;
class QTextLayout;

namespace Luminous {
  
  class FontCache
  {
  public:
    class Glyph : public TextureAtlasItem
    {
    public:
      Glyph()
        : m_location(0, 0)
        , m_size(0, 0)
      {}

      Texture & texture() { assert(m_atlas); return m_atlas->texture(); }
      Nimble::Vector2f location() const { return m_location; }
      Nimble::Vector2f size() const { return m_size; }
      std::array<Nimble::Vector2f, 4> uv() const { return m_uv; }

      bool isEmpty() const { return m_size.x <= 0.0f; }

      void setLocation(const Nimble::Vector2f & location) { m_location = location; }
      void setSize(const Nimble::Vector2f & size) { m_size = size; }

    private:
      Nimble::Vector2f m_location;
      Nimble::Vector2f m_size;
    };

  public:
    LUMINOUS_API ~FontCache();

    LUMINOUS_API Glyph * glyph(quint32 glyph);

    LUMINOUS_API static FontCache & acquire(const QRawFont & rawFont);

  private:
    FontCache(const QRawFont & rawFont);

  private:
    class FontGenerator;
    class D;
    D * m_d;
  };

  class TextLayout
  {
  public:
    struct LUMINOUS_API Item
    {
      std::array<BasicVertexUV, 4> vertices;
    };

  public:
    LUMINOUS_API virtual ~TextLayout();

    LUMINOUS_API int groupCount() const;
    LUMINOUS_API Texture * texture(int groupIndex) const;
    LUMINOUS_API const std::vector<Item> & items(int groupIndex) const;

    LUMINOUS_API bool isComplete() const;
    LUMINOUS_API virtual void generate() = 0;

    LUMINOUS_API void invalidate();

    LUMINOUS_API virtual void setMaximumSize(const Nimble::Vector2f & size);
    LUMINOUS_API Nimble::Vector2f maximumSize() const;

    LUMINOUS_API Nimble::Rectf boundingBox() const;

    LUMINOUS_API const Nimble::Vector2f & renderLocation() const;

  protected:
    LUMINOUS_API TextLayout(const Nimble::Vector2f & maximumSize);

    LUMINOUS_API void setRenderLocation(const Nimble::Vector2f & location);
    LUMINOUS_API void setBoundingBox(const Nimble::Rectf & bb);
    LUMINOUS_API void setLayoutReady(bool v);
    LUMINOUS_API bool layoutReady() const;
    LUMINOUS_API void setGlyphsReady(bool v);
    LUMINOUS_API void clearGlyphs();
    LUMINOUS_API bool generateGlyphs(const Nimble::Vector2f & location,
                                     const QGlyphRun & glyphRun);

  private:
    class D;
    D * m_d;
  };

  /// Plain text, usually one font, inside rectangle (0,0) -> size
  class SimpleTextLayout : public TextLayout
  {
  public:
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

  /// Rich text document layout
  class RichTextLayout : public TextLayout
  {
  public:
    LUMINOUS_API RichTextLayout(const Nimble::Vector2f & size);
    LUMINOUS_API virtual ~RichTextLayout();

    LUMINOUS_API virtual void generate() OVERRIDE;

    LUMINOUS_API QTextDocument & document();
    LUMINOUS_API const QTextDocument & document() const;

  private:
    class D;
    D * m_d;
  };
} // namespace Luminous

#endif // LUMINOUS_TEXTLAYOUT_HPP
