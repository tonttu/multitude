#include "TextureManager.hpp"

#include <Luminous/Image.hpp>

namespace Vivid
{

std::shared_ptr<Luminous::ImageTex> TextureManager::load(const std::string& name)
{
  TextureMap::iterator it = m_textures.find(name);
  if (it != m_textures.end()) {
    if (!it->second.expired()) {
      return it->second.lock();
    }
  }

  std::shared_ptr<Luminous::ImageTex> image(new Luminous::ImageTex);
  if (!image->read(name.c_str())) {
    image.reset();
  } else {
    m_textures[name] = image;
  }
  return image;
}

}
