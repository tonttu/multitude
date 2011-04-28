#include "Renderable.hpp"

namespace Vivid
{


void Renderable::setTransform(const Nimble::Matrix4f& transform)
{
  m_transform = transform;
}

Nimble::Matrix4f& Renderable::getTransform()
{
  return m_transform;
}


void Renderable::setMesh(const std::shared_ptr<Mesh>& mesh)
{
  m_mesh = mesh;
}

const std::shared_ptr<Mesh>& Renderable::getMesh() const
{
  return m_mesh;
}

}
