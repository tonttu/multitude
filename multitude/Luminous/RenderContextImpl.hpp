#ifndef LUMINOUS_RENDER_CONTEXT_IMPL_HPP
#define LUMINOUS_RENDER_CONTEXT_IMPL_HPP

namespace Luminous
{
  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::render(Luminous::PrimitiveType type, int indexCount, int vertexCount, float primitiveSize, const Style & style)
  {
    RenderBuilder<Vertex, UniformBlock> builder;
    RenderCommand & cmd = createRenderCommand(indexCount, vertexCount, builder.idx,
                                              builder.vertex, builder.uniform, builder.depth, style);
    cmd.primitiveType = type;
    cmd.primitiveSize = primitiveSize;

    /// Using "layout(row_major)" on AMD only specifies the row-major -layout
    /// to the first matrix in the uniform block. That is why we don't use that
    /// feature at all. For GLES this also makes things easier.
    /// This happens at least on Ubuntu 12.04, AMD drivers 12-6, OpenGL 4.2.11733
    builder.uniform->projMatrix = viewTransform();
    builder.uniform->projMatrix.transpose();
    builder.uniform->modelMatrix = transform4();
    builder.uniform->modelMatrix.transpose();
    builder.uniform->color = style.fillColor();

    return builder;
  }

  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::drawPointsT(
    const Nimble::Vector2f * vertices, unsigned int vertexCount, float size, Style & style)
  {
    if(!style.fillProgramGL() && !style.fillProgram())
      style.setFillProgram(basicShader());
    RenderBuilder<Vertex, UniformBlock> b = render<Vertex, UniformBlock>(Luminous::PrimitiveType_Point, vertexCount, vertexCount, size, style);
    auto v = b.vertex;
    auto idx = b.idx;

    for (unsigned int i = 0; i < vertexCount; ++i) {
      v++->location.make(vertices[i].x, vertices[i].y, b.depth);
      *idx++ = i;
    }

    return b;
  }

  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::drawLineStripT(
    const Nimble::Vector2f * vertices, unsigned int vertexCount, float width, Style & style)
  {
    if(!style.fillProgramGL() && !style.fillProgram())
      style.setFillProgram(basicShader());
    RenderBuilder<Vertex, UniformBlock> b = render<Vertex, UniformBlock>(Luminous::PrimitiveType_LineStrip, vertexCount, vertexCount, width, style);
    auto v = b.vertex;
    auto idx = b.idx;

    for (unsigned int i = 0; i < vertexCount; ++i) {
      v++->location.make(vertices[i].x, vertices[i].y, b.depth);
      *idx++ = i;
    }

    return b;
  }
  
  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::drawTriStripT(
    const Nimble::Vector2f * vertices, unsigned int vertexCount, Style & style)
  {
    if(!style.fillProgramGL() && !style.fillProgram())
      style.setFillProgram(basicShader());
    RenderBuilder<Vertex, UniformBlock> b = render<Vertex, UniformBlock>(Luminous::PrimitiveType_TriangleStrip, vertexCount, vertexCount, 1.f, style);
    auto v = b.vertex;
    auto idx = b.idx;

    for (unsigned int i = 0; i < vertexCount; ++i) {
      v++->location.make(vertices[i].x, vertices[i].y, b.depth);
      *idx++ = i;
    }

    return b;
  }

  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::drawTexTriStripT(
    const Nimble::Vector2f * vertices, const Nimble::Vector2f * uvs, unsigned int vertexCount, Style & style)
  {
    if(!style.fillProgramGL() && !style.fillProgram())
      style.setFillProgram(texShader());
    RenderBuilder<Vertex, UniformBlock> b = render<Vertex, UniformBlock>(Luminous::PrimitiveType_TriangleStrip, vertexCount, vertexCount, 1.f, style);
    auto v = b.vertex;
    auto idx = b.idx;

    for (unsigned int i = 0; i < vertexCount; ++i) {
      v->location.make(vertices[i].x, vertices[i].y, b.depth);
      v++->texCoord = uvs[i];
      *idx++ = i;
    }

    return b;
  }
}

#endif // LUMINOUS_RENDER_CONTEXT_IMPL_HPP
