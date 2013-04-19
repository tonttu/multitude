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

#include "Texture.hpp"

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
    std::shared_ptr<AtlasNode> insert(Nimble::Size size, int padding);

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

  AtlasNodePtr AtlasNode::insert(Nimble::Size size, int padding)
  {
    // This seems to be a leaf and already reserved
    if (m_reserved) return nullptr;

    bool rotate;
    if (size.width() <= m_size.width() && size.height() <= m_size.height()) {
      // Fits ok without rotation
      rotate = false;
    } else if (size.width() <= m_size.height() && size.height() <= m_size.width()) {
      // Fits if we rotate the object 90 degrees
      /// @todo not supported yet
      return nullptr;
      rotate = true;
      size.transpose();
    } else {
      // Not going to fit, abort
      return nullptr;
    }

    if (isLeaf()) {
      // This is a leaf, we can either reserve this whole leaf for this new
      // object, or split this to two. We will split iff there is enough space
      // for something else.

      if (m_size.width() - size.width() <= padding && m_size.height() - size.height() <= padding) {
        // There is no space for anything else, reserve this whole leaf
        m_size = size;
        m_reserved = true;
        return shared_from_this();
      }

      m_children[0].reset(new AtlasNode(shared_from_this()));
      m_children[1].reset(new AtlasNode(shared_from_this()));

      const Nimble::Size diff = m_size - size;

      if (diff.width() > diff.height()) {
        m_children[0]->m_location = m_location;
        m_children[0]->m_size.make(size.width(), m_size.height());
        m_children[1]->m_location.make(m_location.x + size.width() + padding, m_location.y);
        m_children[1]->m_size.make(m_size.width() - size.width() - padding, m_size.height());
      } else {
        m_children[0]->m_location = m_location;
        m_children[0]->m_size.make(m_size.width(), size.height());
        m_children[1]->m_location.make(m_location.x, m_location.y + size.height() + padding);
        m_children[1]->m_size.make(m_size.width(), m_size.height() - size.height() - padding);
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

  TextureAtlas::TextureAtlas(Nimble::Size size, const Luminous::PixelFormat & pixelFormat, int padding)
    : m_d(new D())
  {
    m_d->m_padding = padding;
    m_d->m_root.reset(new AtlasNode(std::weak_ptr<AtlasNode>()));
    m_d->m_root->m_location.make(padding, padding);
    m_d->m_root->m_size = size - Nimble::Size(padding, padding) * 2;

    m_d->m_image.allocate(size.width(), size.height(), pixelFormat);
    m_d->m_image.zero();
    m_d->m_texture.setData(size.width(), size.height(), pixelFormat, m_d->m_image.data());
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

  Nimble::Size TextureAtlas::size() const
  {
    return m_d->m_image.size();
  }

  TextureAtlas::NodePtr TextureAtlas::insert(Nimble::Size size)
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
