/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_RENDER_CONTEXT_IMPL_HPP
#define LUMINOUS_RENDER_CONTEXT_IMPL_HPP

namespace Luminous
{
  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::render(
                            bool translucent,
                            Luminous::PrimitiveType type,
                            int offset, int vertexCount,
                            float primitiveSize,
                            const Luminous::VertexArray & vertexArray,
                            const Luminous::Program & program,
                            const std::map<QByteArray, const Texture *> * textures,
                            const std::map<QByteArray, ShaderUniform> * uniforms)
  {
    /// @todo Should return the command, since we're only using the builder for returning depth here
    RenderBuilder<Vertex, UniformBlock> builder;

    std::size_t uniformSize = alignUniform(sizeof(UniformBlock));
    unsigned int uniformOffset;

    SharedBuffer * ubuffer;
    void * uniformData;
    std::tie(uniformData, ubuffer) = sharedBuffer(uniformSize, 1, Buffer::UNIFORM, uniformOffset);
    builder.uniform = static_cast<UniformBlock*>(uniformData);

    RenderCommand & cmd = createRenderCommand(translucent, vertexArray, ubuffer->buffer, builder.depth, program, textures, uniforms);

    cmd.primitiveType = type;                                             // Lines, points, vertices, etc
    cmd.primitiveSize = primitiveSize;                                    // For lines/points
    cmd.primitiveCount = vertexCount;                                     // Number of vertices
    cmd.indexed = (vertexArray.indexBuffer() != 0);                       // Whether we should use indexed or non-indexed drawing
    cmd.vertexOffset = (cmd.indexed ? 0 : offset);                        // Vertex offset (for non-indexed)
    cmd.indexOffset = offset;                                             // Index offset (for indexed drawing only)
    cmd.uniformOffsetBytes = uniformOffset * (unsigned int) uniformSize;  // Start of active uniform in buffer
    cmd.uniformSizeBytes = (unsigned int)uniformSize;                     // Size of uniform

    builder.command = &cmd;

    return builder;
  }

  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::render(
              bool translucent, Luminous::PrimitiveType type, int indexCount, int vertexCount, float primitiveSize,
              const Luminous::Program & program,
              const std::map<QByteArray, const Texture *> * textures,
              const std::map<QByteArray, ShaderUniform> * uniforms)
  {
    RenderBuilder<Vertex, UniformBlock> builder;
    RenderCommand & cmd = createRenderCommand(translucent,
                                              indexCount, vertexCount,
                                              builder.idx, builder.vertex, builder.uniform,
                                              builder.depth,
                                              program, textures, uniforms);
    cmd.primitiveType = type;
    cmd.primitiveSize = primitiveSize;

    /// Using "layout(row_major)" on AMD only specifies the row-major -layout
    /// to the first matrix in the uniform block. That is why we don't use that
    /// feature at all. For GLES this also makes things easier.
    /// This happens at least on Ubuntu 12.04, AMD drivers 12-6, OpenGL 4.2.11733
    viewTransform().transpose(builder.uniform->projMatrix);
    transform().transpose(builder.uniform->modelMatrix);

    builder.command = &cmd;

    return builder;
  }
    
  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::drawPrimitiveT(
    Luminous::PrimitiveType primType, unsigned int indexCount, unsigned int vertexCount,
    const Program & shader, const Radiant::ColorPMA & color, float width, const Luminous::Style & style)
  {
    /// @todo Should we be able to overrule this with Style::Translucent
    bool translucent = shader.translucent() || (color.a * opacity() < 0.99999999f) ||
        style.fill().hasTranslucentTextures();

    RenderBuilder<Vertex,UniformBlock> b = render<Vertex, UniformBlock>(translucent, primType, indexCount, vertexCount, width, shader,
                                          style.fill().textures(),
                                          style.fill().uniforms());

    // Set the color and apply opacity
    b.uniform->color = color * opacity();
    // Set default depth
    b.uniform->depth = b.depth;

    return b;
  }

  template <typename InputIterator>
  void RenderContext::drawPoints(InputIterator begin, size_t numPoints, const Luminous::Style & style)
  {
    /// @todo Can't rely on supported point sizes. Should this just call drawCircle instead for values > 1
    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_POINT, 0, numPoints, program, style.strokeColor(), style.strokeWidth(), style);
    for (size_t i = 0; i < numPoints; ++i, ++begin)
      b.vertex[i].location = *begin;
  }

  template <typename InputIterator>
  void RenderContext::drawPolyLine(InputIterator begin, size_t numVertices, const Luminous::Style & style)
  {
    if (numVertices < 2)
      return;
    assert(style.strokeWidth() > 0.f);
    const Program & program = (style.strokeProgram() ? *style.strokeProgram() : basicShader());
    auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(
          Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, numVertices*2, program, style.strokeColor(), 0, style);
    const float r = style.strokeWidth() / 2.f;
    BasicVertex * out = b.vertex;

    Nimble::Vector2f prev = *begin++;
    Nimble::Vector2f curr = *begin++;
    // Tangent of first point is points[1] - points[0]
    // "begin" doesn't need to be random access iterator, so we will only use
    // operator++ with it.
    Nimble::Vector2f n = (curr - prev).perpendicular().normalized(r);
    out++->location = prev + n;
    out++->location = prev - n;

    for (size_t i = 1; i < numVertices-1; ++i) {
      Nimble::Vector2f next = *begin++;
      // Tangent of point n is points[n+1] - points[n-1]
      n = (next - prev).perpendicular().normalized(r);
      out++->location = curr + n;
      out++->location = curr - n;
      prev = curr;
      curr = next;
    }

    // Tangent of last point is points[last] - points[last-1]
    n = (curr - prev).perpendicular().normalized(r);
    out++->location = curr + n;
    out++->location = curr - n;
  }

  template<typename T>
  void RenderContext::drawRect(const Nimble::RectT<T> &rect, const Style & style)
  {
    if(style.fill().hasTextures()) {

      // Automatically generate texture coordinates for the rectangle since the
      // style uses textures
      drawRect(rect, Nimble::Rect(0, 0, 1, 1), style);

    } else {

      // Is this a non-textured fill?
      if(!style.fillColor().isZero()) {

        // Draw rectangle without texture coordinates
        const Program & program = (style.fillProgram() ? *style.fillProgram() : basicShader());
        auto b = drawPrimitiveT<BasicVertex, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, 4, program, style.fillColor(), 1.f, style);
        b.vertex[0].location = rect.low().template cast<float>();
        b.vertex[1].location = rect.highLow().template cast<float>();
        b.vertex[2].location = rect.lowHigh().template cast<float>();
        b.vertex[3].location = rect.high().template cast<float>();
      }

      // Draw stroke
      drawRectStroke(rect, style);
    }
  }

  template<typename T>
  void RenderContext::drawRect(const Nimble::RectT<T>& rect, const Nimble::Rectf & uvs, const Style & style)
  {
    // Draw rectangle with texture coordinates
    if (!style.fillColor().isZero()) {
      const Program & program = (style.fillProgram() ? *style.fillProgram() : texShader());
      auto b = drawPrimitiveT<BasicVertexUV, BasicUniformBlock>(Luminous::PRIMITIVE_TRIANGLE_STRIP, 0, 4, program, style.fillColor(), 1.f, style);

      b.vertex[0].location = rect.low().template cast<float>();
      b.vertex[0].texCoord = uvs.low();

      b.vertex[1].location = rect.highLow().template cast<float>();
      b.vertex[1].texCoord = uvs.highLow();

      b.vertex[2].location = rect.lowHigh().template cast<float>();
      b.vertex[2].texCoord = uvs.lowHigh();

      b.vertex[3].location = rect.high().template cast<float>();
      b.vertex[3].texCoord = uvs.high();
    }

    // Draw stroke
    drawRectStroke(rect, style);
  }

  template<typename T>
  void RenderContext::drawRectStroke(const Nimble::RectT<T>& rect, const Style &style)
  {
    // Draw the outline
    if (style.strokeWidth() > 0.f && !style.strokeColor().isZero()) {

      Luminous::Style s = style;
      s.stroke().clear();
      s.setFillColor(style.strokeColor());
      if(style.strokeProgram())
        s.setFillProgram(*style.strokeProgram());
      else
        s.setDefaultFillProgram();

      Nimble::Rectf outer = rect.template cast<float>();
      Nimble::Rectf inner = outer;
      outer.grow(0.5f*style.strokeWidth());
      inner.shrink(0.5f*style.strokeWidth());

      drawRectWithHole(outer, inner, s);
    }
  }

}

#endif // LUMINOUS_RENDER_CONTEXT_IMPL_HPP
