/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "TextureAtlas.hpp"

#include "Texture2.hpp"

#include <memory>
#include <cassert>

namespace
{
  class AtlasNode : public Luminous::TextureAtlas::Node, public std::enable_shared_from_this<AtlasNode>
  {
  public:
    /// @todo memory pool for AtlasNodes, and custom new operator
    std::shared_ptr<AtlasNode> m_children[2];
    std::weak_ptr<AtlasNode> m_parent;

    bool m_reserved;

  public:
    AtlasNode(std::weak_ptr<AtlasNode> parent);
    std::shared_ptr<AtlasNode> insert(Nimble::Vector2i size, int padding);

    inline bool isLeaf() const;
  };
  typedef std::shared_ptr<AtlasNode> AtlasNodePtr;

  ///////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////

  AtlasNode::AtlasNode(std::weak_ptr<AtlasNode> parent)
    : Node()
    , m_parent(std::move(parent))
    , m_reserved(false)
  {
  }

  bool AtlasNode::isLeaf() const
  {
    return m_children[0] == m_children[1];
  }

  AtlasNodePtr AtlasNode::insert(Nimble::Vector2i size, int padding)
  {
    // This seems to be a leaf and already reserved
    if (m_reserved) return nullptr;

    bool rotate;
    if (size.x <= m_size.x && size.y <= m_size.y) {
      // Fits ok without rotation
      rotate = false;
    } else if (size.x <= m_size.y && size.y <= m_size.x) {
      // Fits if we rotate the object 90 degrees
      /// @todo not supported yet
      return nullptr;
      rotate = true;
      std::swap(size.x, size.y);
    } else {
      // Not going to fit, abort
      return nullptr;
    }

    if (isLeaf()) {
      // This is a leaf, we can either reserve this whole leaf for this new
      // object, or split this to two. We will split iff there is enough space
      // for something else.

      if (m_size.x - size.x <= padding && m_size.y - size.y <= padding) {
        // There is no space for anything else, reserve this whole leaf
        m_size = size;
        m_reserved = true;
        return shared_from_this();
      }

      m_children[0].reset(new AtlasNode(shared_from_this()));
      m_children[1].reset(new AtlasNode(shared_from_this()));

      const Nimble::Vector2i diff = m_size - size;

      if (diff.x > diff.y) {
        m_children[0]->m_location = m_location;
        m_children[0]->m_size.make(size.x, m_size.y);
        m_children[1]->m_location.make(m_location.x + size.x + padding, m_location.y);
        m_children[1]->m_size.make(m_size.x - size.x - padding, m_size.y);
      } else {
        m_children[0]->m_location = m_location;
        m_children[0]->m_size.make(m_size.x, size.y);
        m_children[1]->m_location.make(m_location.x, m_location.y + size.y + padding);
        m_children[1]->m_size.make(m_size.x, m_size.y - size.y - padding);
      }

      AtlasNodePtr n = m_children[0]->insert(size, padding);
      if (rotate && n) {
        n->m_rotated = !n->m_rotated;
      }
      return n;
    } else {
      AtlasNodePtr node = m_children[0]->insert(size, padding);
      if (node)
        return node;
      return m_children[1]->insert(size, padding);
    }
  }
} // Anonymous namespace

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

namespace Luminous
{
  class TextureAtlas::D
  {
  public:
    int m_padding;
    Luminous::Image m_image;
    Luminous::Texture m_texture;
    std::shared_ptr<AtlasNode> m_root;
  };

  ///////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////

  TextureAtlas::Node::Node()
    : m_rotated(false)
  {}

  ///////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////

  TextureAtlas::TextureAtlas(Nimble::Vector2i size, const Luminous::PixelFormat & pixelFormat, int padding)
    : m_d(new D())
  {
    m_d->m_padding = padding;
    m_d->m_root.reset(new AtlasNode(std::weak_ptr<AtlasNode>()));
    m_d->m_root->m_location.make(padding, padding);
    m_d->m_root->m_size = size - Nimble::Vector2i(padding, padding) * 2;

    m_d->m_image.allocate(size.x, size.y, pixelFormat);
    m_d->m_image.zero();
    m_d->m_texture.setData(size.x, size.y, pixelFormat, m_d->m_image.data());
  }

  TextureAtlas::~TextureAtlas()
  {
    delete m_d;
  }

  TextureAtlas::TextureAtlas(TextureAtlas && src)
    : m_d(src.m_d)
  {
    src.m_d = nullptr;
  }

  TextureAtlas & TextureAtlas::operator=(TextureAtlas && src)
  {
    std::swap(m_d, src.m_d);
    return *this;
  }

  int TextureAtlas::padding() const
  {
    return m_d->m_padding;
  }

  Nimble::Vector2i TextureAtlas::size() const
  {
    return m_d->m_image.size();
  }

  TextureAtlas::NodePtr TextureAtlas::insert(Nimble::Vector2i size)
  {
    return m_d->m_root->insert(size, padding());
  }

  void TextureAtlas::remove(TextureAtlas::NodePtr)
  {
  }

  Luminous::Image & TextureAtlas::image()
  {
    return m_d->m_image;
  }

  Luminous::Texture & TextureAtlas::texture()
  {
    return m_d->m_texture;
  }

} // namespace Luminous
