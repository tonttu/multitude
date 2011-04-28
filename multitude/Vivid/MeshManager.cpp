#include "MeshManager.hpp"

#include "Mesh.hpp"
#include "Scene.hpp"

namespace Vivid
{

std::shared_ptr<Mesh> MeshManager::load(const std::string& file, const std::string& name)
{
  MeshMap::iterator it = m_meshes.find(name);
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
    m_meshes[name] = mesh;
  }

  return mesh;
}

}
