#ifndef LUMINOUS_RENDERCOMMAND_HPP
#define LUMINOUS_RENDERCOMMAND_HPP

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector3.hpp>
#include <Nimble/Vector4.hpp>
#include <Nimble/Matrix4.hpp>
#include <Nimble/Rect.hpp>

#include "Luminous/Luminous.hpp"
#include "Luminous/BlendMode.hpp"
#include "Luminous/ShaderUniform.hpp"
#include "Luminous/DepthMode.hpp"
#include "Luminous/StencilMode.hpp"

#include <array>
#include <utility>

/// @cond
namespace Luminous
{
  struct RenderCommand
  {
    PrimitiveType primitiveType;
    std::size_t primitiveCount;

    float primitiveSize;    // Used for points and lines

    bool indexed;
    unsigned int indexOffset;
    unsigned int vertexOffset;

    unsigned int uniformSizeBytes;
    unsigned int uniformOffsetBytes;

    std::array<std::pair<int, int>, 8> samplers;
    std::array<std::pair<int, ShaderUniform>, 8> uniforms;
  };
  
  struct BasicVertex
  {
    Nimble::Vector2f location;
  };

  struct BasicVertexUV
  {
    Nimble::Vector2f location;
    Nimble::Vector2f texCoord;
  };

  struct FontVertex : public BasicVertexUV
  {
    float invsize;
  };

  struct BasicUniformBlock
  {
    Nimble::Matrix4f projMatrix;
    Nimble::Matrix4f modelMatrix;
    Nimble::Vector4f color;
    float depth;
  };

  struct FontUniformBlock
  {
    Nimble::Matrix4f projMatrix;
    Nimble::Matrix4f modelMatrix;
    Nimble::Vector4f colorIn;
    Nimble::Vector4f colorOut;
    Nimble::Rectf clip;
    /// Start and stop locations of edge fading, default is (0.5, 0.5), that means
    /// sharp edge at the correct glyph border. (0.0, 0.5) would generate a halo
    /// or glow effect, (0.35, 0.35) would make really "bold" text.
    Nimble::Vector2f outline;
    float invscale;
    /// Location where border color ends and text color starts, default = 0.0,
    /// meaning border color isn't used at all. Setting this to 0.5 and outline
    /// to (0.4, 0.4) would create a sharp border from 0.4..0.5.
    /// split = 0.5, outline = (0.3, 0.5) would create a halo from 0.3..0.5 with
    /// border color, and sharp edge at 0.5 between border and text color
    float split;
  };
}

/// @endcond
#endif // LUMINOUS_RENDERCOMMAND_HPP
