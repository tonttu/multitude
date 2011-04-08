#ifndef VIVID_MATERIAL_HPP
#define VIVID_MATERIAL_HPP

#include <string>
#include <map>

namespace Vivid {


class Material {
public:
  std::string m_shadingModel;

  typedef std::map<std::string, std::string> TextureMap;
  TextureMap m_textures;
};

}
#endif
