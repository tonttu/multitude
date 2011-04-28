#ifndef VIVID_MESHMANAGER_HPP
#define VIVID_MESHMANAGER_HPP

#include <Radiant/RefPtr.hpp>
#include <Patterns/Singleton.hpp>

#include <map>

namespace Vivid
{

class Mesh;

class MeshManager : public Patterns::Singleton<MeshManager>
{
public:

  std::shared_ptr<Mesh> load(const std::string& file, const std::string& name);

private:
  typedef std::map<std::string, std::weak_ptr<Mesh> > MeshMap;
  MeshMap m_meshes;
};

}

#endif
