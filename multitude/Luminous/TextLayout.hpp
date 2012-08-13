#ifndef LUMINOUS_TEXTLAYOUT_HPP
#define LUMINOUS_TEXTLAYOUT_HPP

#include "VertexHolder.hpp"
#include "TextureAtlas.hpp"

#include <Nimble/Vector2.hpp>

#include <QFont>

#include <array>

class QRawFont;

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
    LUMINOUS_API ~TextLayout();

    LUMINOUS_API int groupCount() const;
    LUMINOUS_API Texture * texture(int groupIndex) const;
    LUMINOUS_API const std::vector<Item> & items(int groupIndex) const;

    LUMINOUS_API bool isComplete() const;

    LUMINOUS_API static const TextLayout & layout(const QString & text, const Nimble::Vector2f & size, QFont font, bool useCache);

  private:
    TextLayout(const QString & text, const Nimble::Vector2f & size, const QFont & font);

  private:
    class D;
    D * m_d;
  };
  
} // namespace Luminous

#endif // LUMINOUS_TEXTLAYOUT_HPP
