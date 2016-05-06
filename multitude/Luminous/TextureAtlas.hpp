/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_TEXTUREATLAS_HPP
#define LUMINOUS_TEXTUREATLAS_HPP

/// @cond

#include "Image.hpp"

#include <Patterns/NotCopyable.hpp>

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector4.hpp>

#include <map>
#include <vector>
#include <array>

namespace Luminous
{
  /// Implements simple rectangle packing algorithm using binary trees
  /// inspired by http://www.blackpawn.com/texts/lightmaps/default.html
  class LUMINOUS_API TextureAtlas : public Patterns::NotCopyable
  {
  public:
    /// A node in the TextureAtlas binary tree
    class Node : public Patterns::NotCopyable
    {
    public:
      LUMINOUS_API Node();

    public:
      Nimble::Vector2i m_location;
      Nimble::Size m_size;
      bool m_rotated;
    };
    typedef std::shared_ptr<Node> NodePtr;

  public:
    TextureAtlas(Nimble::Size size, const Luminous::PixelFormat & pixelFormat, int padding = 1);
    ~TextureAtlas();

    TextureAtlas(TextureAtlas && src);
    TextureAtlas & operator=(TextureAtlas && src);

    int padding() const;

    Nimble::Size size() const;

    NodePtr insert(Nimble::Size size);
    void remove(TextureAtlas::NodePtr node);

    Luminous::Image & image();
    Luminous::Texture & texture();

  private:
    class D;
    D * m_d;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////

  /// The actual texture data associated with binary tree nodes in the texture atlas.
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

  /// This class provides the high-level API for the texture atlas. It
  /// allocates fixed-size large textures to use for storing smaller textures. If
  /// more space is required to fit all contents, new textures are allocated.
  template <typename Item>
  class TextureAtlasGroup
  {
  public:
    /// Construct a new texture atlas group with the given pixel format
    /// @param pixelFormat pixel format for the textures
    TextureAtlasGroup(const Luminous::PixelFormat & pixelFormat);
    virtual ~TextureAtlasGroup() { }

    /// Clears all atlases and items from the group
    void clear();

    /// Reserve space for an item from the atlas. This function will return an
    /// item that can be used to store a texture of the requested size. If all
    /// current atlas textures are full, a new one is allocated automatically.
    /// @param size requested space
    Item & insert(Nimble::Size size);

    /// Store the texture atlases on disk. This function can be useful for debugging.
    /// @param basename base filename to use for the atlases
    void save(const QString & basename);

    /// Get the atlases in the group
    /// @return vector of atlases in this texture atlas group
    const std::vector<std::unique_ptr<TextureAtlas>> & atlases() const { return m_atlases; }

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
  Item & TextureAtlasGroup<Item>::insert(Nimble::Size size)
  {
    /// @todo configure, check for hw limits
    /// @todo does the distance field shader anti-aliasing break if the texture size changes?
    const int baseSize = 4096;
    const int maxSize = 8*1024;

    m_items.emplace_back(new Item());
    Item & item = *m_items.back();

    for (size_t i = 0, s = m_atlases.size(); i <= s; ++i) {
      if (i == s) {
        int size = std::min(maxSize, baseSize << i);
        m_atlases.emplace_back(new TextureAtlas(Nimble::Size(size, size), m_pixelFormat));
      }
      TextureAtlas & atlas = *m_atlases[i];
      TextureAtlas::NodePtr node = atlas.insert(size);
      if (!node)
        continue;
      item.m_atlas = &atlas;
      item.m_node = std::move(node);
      float scalex = 1.0f / item.m_atlas->size().width(),
          scaley = 1.0f / item.m_atlas->size().height();
      float uvs[4] = { item.m_node->m_location.x * scalex, (item.m_node->m_location.x + item.m_node->m_size.width()) * scaley,
                       item.m_node->m_location.y * scalex, (item.m_node->m_location.y + item.m_node->m_size.height()) * scaley };
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

/// @endcond

#endif // LUMINOUS_TEXTUREATLAS_HPP
