#include "Renderable.hpp"
namespace Vivid
{

class Renderable::D
{
public:
  D()
    : m_transform(Nimble::Matrix4::IDENTITY)
  {
  }

  std::shared_ptr<Mesh> m_mesh;
  Nimble::Matrix4f m_transform;
};

Renderable::Renderable()
  : m_data(new D())
{}

Renderable::~Renderable()
{
  delete m_data;
}

void Renderable::setTransform(const Nimble::Matrix4f& transform)
{
  m_data->m_transform = transform;
}

Nimble::Matrix4f& Renderable::getTransform()
{
  return m_data->m_transform;
}


void Renderable::setMesh(const std::shared_ptr<Mesh>& mesh)
{
  m_data->m_mesh = mesh;
}

const std::shared_ptr<Mesh>& Renderable::getMesh() const
{
  return m_data->m_mesh;
}

}
