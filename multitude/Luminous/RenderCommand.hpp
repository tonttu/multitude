/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

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

namespace Luminous
{
/// @cond
  struct RenderCommandBase
  {
    PrimitiveType primitiveType;

    unsigned int uniformSizeBytes;
    unsigned int uniformOffsetBytes;

    // Index to RenderDriverGL::D::m_samplers
    unsigned int samplersBegin;
    unsigned int samplersEnd;

    // Index to RenderDriverGL::D::m_uniforms
    unsigned int uniformsBegin;
    unsigned int uniformsEnd;
  };

  struct RenderCommand : public RenderCommandBase
  {
    std::size_t primitiveCount;

    float primitiveSize;    // Used for points and lines

    bool indexed;
    unsigned int indexOffset;
    unsigned int vertexOffset;
  };

  struct MultiDrawCommand : public RenderCommandBase
  {
    int drawCount;

    // Allocated by RenderDriverGL::D
    int * offsets;
    int * counts;
  };

/// @endcond

  /// At the moment only normal RenderCommand is supported, but in the future
  /// we could add alternative render commands for things like
  /// glMultiDrawElements and they could all be indexed through this class.
  struct RenderCommandIndex
  {
    /// Index to RenderDriverGL::D::m_renderCommands, max means null command
    unsigned int renderCommandIndex = std::numeric_limits<unsigned int>::max();
    unsigned int multiDrawCommandIndex = std::numeric_limits<unsigned int>::max();
  };

  /// The most basic type of vertex to use with shader programs.
  /// Contains only 2D location of the vertex.
  struct BasicVertex
  {
    /// The location of the vertex.
    Nimble::Vector2f location;
  };

  /// Vertex to use with shader programs that use texturing.
  struct BasicVertexUV
  {
    /// The location of the vertex
    Nimble::Vector2f location;
    /// The texture coordinate of the vertex.
    Nimble::Vector2f texCoord;
  };

/// @cond
  struct FontVertex : public BasicVertexUV
  {
    float invsize;
  };
/// @endcond

  /// Uniform block to use with most of the shaders included in Luminous.
  struct BasicUniformBlock
  {
    /// Projection matrix for the geometry. Transforms vertices from world to clip coordinates.
    Nimble::Matrix4f projMatrix;
    /// Model matrix for the geometry. Transforms vertices from model to world coordinates.
    Nimble::Matrix4f modelMatrix;
    /// Color of the vertices.
    Radiant::ColorPMA color;
    /// Depth of the vertices.
    float depth;
  };

  /// Uniform block to be used with trilinear texture filtering
  struct TrilinearFilteringUniformBlock : public Luminous::BasicUniformBlock
  {
    /// Blend parameter, will be used like mix(tex[0], tex[1], blending)
    float blending;
  };


/// @cond
  struct FontUniformBlock
  {
    Nimble::Matrix4f projMatrix;
    Nimble::Matrix4f modelMatrix;
    Radiant::ColorPMA colorIn;
    Radiant::ColorPMA colorOut;
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
    float depth;
  };
/// @endcond
}

#endif // LUMINOUS_RENDERCOMMAND_HPP
