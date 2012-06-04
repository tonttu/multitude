#include "Luminous/TextureArray.hpp"

namespace Luminous {
  class TextureArray::D
  {
  public:
    std::vector<Texture2> textures;
    unsigned int level;
  };

  TextureArray(RenderResource::Id id, RenderDriver & driver)
    : RenderResource(id, RT_TEXTUREARRAY, driver)
    , m_d(new TextureArray::D())
  {
  }

  ~TextureArray()
  {
    delete m_d;
  }

  void TextureArray::setLevelCount(unsigned int levels)
  {
    assert(levels > 0);
    m_d->textures.resize(levels);

    // Reset the selected level if the level doesn't exist anymore
    if (m_d->level >= m_d->textures.size())
      m_d->level = 0;

    invalidate();
  }

  unsigned int TextureArray::levelCount() const
  {
    return m_d->textures.size();
  }

  void TextureArray::setLevel(unsigned int level)
  {
    m_d->level = level;
  }

  unsigned int TextureArray::level() const
  {
    return m_d->level;
  }
}

