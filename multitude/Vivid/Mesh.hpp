#ifndef VIVID_MESH_HPP
#define VIVID_MESH_HPP

#include "Triangle.hpp"
#include "Material.hpp"
#include "Export.hpp"

#include <Luminous/ContextVariable.hpp>
#include <Luminous/VertexBuffer.hpp>
#include <Luminous/Shader.hpp>

#include <Nimble/Vector3.hpp>

#include <string>
#include <vector>

namespace Vivid
{

class VIVID_API Mesh
{
public:
  std::string m_name;

  std::vector<Nimble::Vector3> m_vertices;
  std::vector<Nimble::Vector3> m_normals;
  std::vector<Nimble::Vector3> m_tangents;
  std::vector<Nimble::Vector3> m_bitangents;

  std::vector<int> m_indices;
  std::vector<Nimble::Vector2> m_textureCoordinates;
  Material m_material;

  Luminous::ContextVariableT<Luminous::VertexBuffer> m_vb;
  Luminous::Shader m_shader;

  void fillVB(Luminous::VertexBuffer & vb);

  size_t vertexSizeInBytes() const;
};

}
#endif
