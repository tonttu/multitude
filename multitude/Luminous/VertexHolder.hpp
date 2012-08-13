#ifndef LUMINOUS_VERTEXHOLDER_HPP
#define LUMINOUS_VERTEXHOLDER_HPP

#include "Nimble/Vector2.hpp"
#include "Nimble/Vector3.hpp"
#include "Nimble/Vector4.hpp"
#include "Nimble/Matrix4.hpp"

#include "Luminous/Luminous.hpp"
#include "Luminous/BlendMode.hpp"
#include "Luminous/DepthMode.hpp"
#include "Luminous/StencilMode.hpp"

#include <array>
#include <utility>

//@cond
namespace Luminous
{
  struct RenderCommand
  {
    PrimitiveType primitiveType;
    std::size_t primitiveCount;

    float primitiveSize;    // Used for points and lines
    BlendMode blendMode;
    DepthMode depthMode;
    StencilMode stencilMode;

    unsigned int indexOffset;
    unsigned int vertexOffset;

    unsigned int uniformSizeBytes;
    unsigned int uniformOffsetBytes;

    std::array<std::pair<int, int>, 8> samplers;
  };
  
  struct BasicVertex
  {
    Nimble::Vector3f location;
  };

  struct BasicVertexUV
  {
    Nimble::Vector3f location;
    Nimble::Vector2f texCoord;
  };

  struct BasicUniformBlock
  {
    Nimble::Matrix4f projMatrix;
    Nimble::Matrix4f modelMatrix;
    Nimble::Vector4f color;
  };

  struct FontUniformBlock : public BasicUniformBlock
  {
  };
}

//@endcod
#endif // VERTEXHOLDER_HPP
