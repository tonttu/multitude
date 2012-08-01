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

    b.uniform->projMatrix = viewTransform();
    b.uniform->modelMatrix = transform4();
    b.uniform->color = style.fillColor();

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

    b.uniform->projMatrix = viewTransform();
    b.uniform->modelMatrix = transform4();
    b.uniform->color = style.fillColor();

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

    b.uniform->projMatrix = viewTransform();
    b.uniform->modelMatrix = transform4();
    b.uniform->color = style.fillColor();

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

    b.uniform->projMatrix = viewTransform();
    b.uniform->modelMatrix = transform4();
    b.uniform->color = style.fillColor();

    return b;
  }
}

#endif // LUMINOUS_RENDER_CONTEXT_IMPL_HPP
