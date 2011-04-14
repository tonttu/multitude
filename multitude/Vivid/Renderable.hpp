#ifndef VIVID_RENDERABLE_HPP
#define VIVID_RENDERABLE_HPP

#include <Radiant/RefPtr.hpp>
#include <Nimble/Matrix4.hpp>

namespace Vivid
{

class Mesh;

class Renderable
{
public:
  void setTransform(const Nimble::Matrix4f& transform);
  Nimble::Matrix4f& getTransform();

  void setMesh(const std::shared_ptr<Mesh>& mesh);
  const std::shared_ptr<Mesh>& getMesh() const;
private:
  std::shared_ptr<Mesh> m_mesh;
  Nimble::Matrix4f m_transform;
};

}

#endif
