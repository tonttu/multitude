#ifndef VIVID_MESHMANAGER_HPP
#define VIVID_MESHMANAGER_HPP

#include <Radiant/RefPtr.hpp>
#include <Radiant/Singleton.hpp>

#include <map>

namespace Vivid
{

class Mesh;

class MeshManager
{
  DECLARE_SINGLETON(MeshManager);
public:

  std::shared_ptr<Mesh> load(const std::string& file, const std::string& name);

private:
  typedef std::map<std::pair<std::string, std::string>, std::weak_ptr<Mesh> > MeshMap;
  MeshMap m_meshes;
};

}

#endif
