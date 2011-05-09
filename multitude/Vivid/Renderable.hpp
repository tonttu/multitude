#ifndef VIVID_RENDERABLE_HPP
#define VIVID_RENDERABLE_HPP

#include <Radiant/RefPtr.hpp>

#include <Nimble/Matrix4.hpp>

namespace Luminous
{
  class RenderContext;
}

namespace Vivid
{

class Mesh;

class Renderable
{
public:
  Renderable();
  virtual ~Renderable();

  void setTransform(const Nimble::Matrix4f& transform);
  Nimble::Matrix4f& getTransform();


  virtual void customRender(Luminous::RenderContext&) {}

  void setMesh(const std::shared_ptr<Mesh>& mesh);
  const std::shared_ptr<Mesh>& getMesh() const;

private:
  class D;
  D * m_data;
};

}

#endif
