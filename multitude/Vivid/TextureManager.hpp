#ifndef VIVID_TEXTUREMANAGER_HPP
#define VIVID_TEXTUREMANAGER_HPP

#include <Radiant/RefPtr.hpp>
#include <Radiant/Singleton.hpp>

#include <map>

namespace Luminous
{
class ImageTex;
}

namespace Vivid
{

class TextureManager
{
  DECLARE_SINGLETON(TextureManager);
public:

  std::shared_ptr<Luminous::ImageTex> load(const std::string& name);

private:
  typedef std::map<std::string, std::weak_ptr<Luminous::ImageTex> > TextureMap;
  TextureMap m_textures;
};

}

#endif
