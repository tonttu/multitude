#include "MeshManager.hpp"

#include "Mesh.hpp"
#include "Scene.hpp"

namespace Vivid
{

std::shared_ptr<Mesh> MeshManager::load(const std::string& file, const std::string& name)
{
  std::pair<std::string,std::string> key = std::make_pair(file, name);

  MeshMap::iterator it = m_meshes.find(key);
  if (it != m_meshes.end()) {
    if (!it->second.expired()) {
      return it->second.lock();
    }
  }


  KFbxSdkManager*  manager = KFbxSdkManager::Create();
  Scene scene(manager);
  std::shared_ptr<Mesh> mesh;

  if (!scene.import(file)) {
    return mesh;
  }

  mesh.reset(scene.findMesh(name));

  if (mesh) {
    m_meshes[key] = mesh;
  }

  return mesh;
}

}

DEFINE_SINGLETON(Vivid::MeshManager);
