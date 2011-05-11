#include "Mesh.hpp"

namespace Vivid
{

size_t Mesh::vertexSizeInBytes() const
{
  // Assume there's always position data
  size_t vertexSize = 3 * sizeof(float);

  /// @todo these shouldn't be hardcoded...
  if(!m_textureCoordinates.empty())
    vertexSize += 2 * sizeof(float);

  if(!m_tangents.empty())
    vertexSize += 3 * sizeof(float);

  if(!m_bitangents.empty())
    vertexSize += 3 * sizeof(float);

  if(!m_normals.empty())
    vertexSize += 3 * sizeof(float);

  return vertexSize;
}

  void Mesh::fillVB(Luminous::VertexBuffer &vb)
  {
    const size_t vertexSize = vertexSizeInBytes();

    // Allocate memory for the data
    vb.allocate(vertexSize * m_indices.size(), Luminous::VertexBuffer::STATIC_DRAW);

    // Interleave the data in the vb
    size_t offset = 0;
    for(size_t i = 0; i < m_indices.size(); i++) {

      const size_t index = m_indices[i];

      // Position
      vb.partialFill(offset, m_vertices[index].data(), 3 * sizeof(float));
      offset += 3 * sizeof(float);

      // Normal
      if(!m_normals.empty()) {
        vb.partialFill(offset, m_normals[index].data(), 3 * sizeof(float));
        offset += 3 * sizeof(float);
      }

      // Texture coordinate
      if(!m_textureCoordinates.empty()) {
        vb.partialFill(offset, m_textureCoordinates[index].data(), 2 * sizeof(float));
        offset += 2 * sizeof(float);
      }

      // Tangent
      if(!m_tangents.empty()) {
        vb.partialFill(offset, m_tangents[index].data(), 3 * sizeof(float));
        offset += 3 * sizeof(float);
      }

      // Bitangent
      if(!m_bitangents.empty()) {
        vb.partialFill(offset, m_bitangents[index].data(), 3 * sizeof(float));
        offset += 3 * sizeof(float);
      }
    }
  }
}
