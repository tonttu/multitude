#ifndef LUMINOUS_TEXTUREATLAS_HPP
#define LUMINOUS_TEXTUREATLAS_HPP

#include "Image.hpp"

#include <Patterns/NotCopyable.hpp>

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector4.hpp>

#include <map>
#include <vector>

namespace Luminous
{
  /// Implements simple rectangle packing algorithm using binary trees
  /// inspired by http://www.blackpawn.com/texts/lightmaps/default.html
  class LUMINOUS_API TextureAtlas : public Patterns::NotCopyable
  {
  public:
    class Node : public Patterns::NotCopyable
    {
    public:
      Node();

    public:
      Nimble::Vector2i m_location;
      Nimble::Vector2i m_size;
      bool m_rotated;
    };
    typedef std::shared_ptr<Node> NodePtr;

  public:
    TextureAtlas(Nimble::Vector2i size, const Luminous::PixelFormat & pixelFormat, int padding = 1);
    ~TextureAtlas();

    TextureAtlas(TextureAtlas && src);
    TextureAtlas & operator=(TextureAtlas && src);

    int padding() const;

    Nimble::Vector2i size() const;

    NodePtr insert(Nimble::Vector2i size);
    void remove(TextureAtlas::NodePtr node);

    Luminous::Image & image();
    Luminous::Texture & texture();

    Radiant::Mutex & textureMutex();

  private:
    class D;
    D * m_d;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////

  struct TextureAtlasItem
  {
    TextureAtlasItem() : m_atlas(nullptr) {}
    ~TextureAtlasItem() { }

    TextureAtlas * m_atlas;
    TextureAtlas::NodePtr m_node;

    std::array<Nimble::Vector2f, 4> m_uv;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////

  template <typename Item>
  class TextureAtlasGroup
  {
  public:
    TextureAtlasGroup(const Luminous::PixelFormat & pixelFormat);
    ~TextureAtlasGroup() { }

    void clear();

    Item & insert(Nimble::Vector2i size);

    void save(const QString & basename);

  private:
    const Luminous::PixelFormat m_pixelFormat;
    std::vector<std::unique_ptr<Item>> m_items;
    std::vector<std::unique_ptr<TextureAtlas>> m_atlases;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////

  template <typename Item>
  TextureAtlasGroup<Item>::TextureAtlasGroup(const Luminous::PixelFormat & pixelFormat)
    : m_pixelFormat(pixelFormat)
  {
  }

  template <typename Item>
  void TextureAtlasGroup<Item>::clear()
  {
    m_items.clear();
    m_atlases.clear();
  }

  template <typename Item>
  Item & TextureAtlasGroup<Item>::insert(Nimble::Vector2i size)
  {
    /// @todo configure, check for hw limits
    /// @todo does the distance field shader anti-aliasing break if the texture size changes?
    const int baseSize = 2048;
    const int maxSize = 8*1024;

    m_items.emplace_back(new Item());
    Item & item = *m_items.back();

    for (int i = 0, s = m_atlases.size(); i <= s; ++i) {
      if (i == s) {
        int size = std::min(maxSize, baseSize << i);
        m_atlases.emplace_back(new TextureAtlas(Nimble::Vector2i(size, size), m_pixelFormat));
      }
      TextureAtlas & atlas = *m_atlases[i];
      TextureAtlas::NodePtr node = atlas.insert(size);
      if (!node)
        continue;
      item.m_atlas = &atlas;
      item.m_node = std::move(node);
      float scalex = 1.0f / item.m_atlas->size().x,
          scaley = 1.0f / item.m_atlas->size().y;
      float uvs[4] = { item.m_node->m_location.x * scalex, (item.m_node->m_location.x + item.m_node->m_size.x) * scaley,
                       item.m_node->m_location.y * scalex, (item.m_node->m_location.y + item.m_node->m_size.y) * scaley };
      if (item.m_node->m_rotated) {
        item.m_uv[0].make(uvs[0], uvs[3]);
        item.m_uv[1].make(uvs[0], uvs[2]);
        item.m_uv[2].make(uvs[1], uvs[3]);
        item.m_uv[3].make(uvs[1], uvs[2]);
      } else {
        item.m_uv[0].make(uvs[0], uvs[2]);
        item.m_uv[1].make(uvs[1], uvs[2]);
        item.m_uv[2].make(uvs[0], uvs[3]);
        item.m_uv[3].make(uvs[1], uvs[3]);
      }
      return item;
    }

    return item;
  }

  template <typename Item>
  void TextureAtlasGroup<Item>::save(const QString & basename)
  {
    for (int i = 0, s = m_atlases.size(); i < s; ++i) {
      TextureAtlas & atlas = *m_atlases[i];
      atlas.image().write(basename.arg(i).toUtf8().data());
    }
  }

} // namespace Luminous

#endif // LUMINOUS_TEXTUREATLAS_HPP
