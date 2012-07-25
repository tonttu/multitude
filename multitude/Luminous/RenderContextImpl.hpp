#ifndef LUMINOUS_RENDER_CONTEXT_IMPL_HPP
#define LUMINOUS_RENDER_CONTEXT_IMPL_HPP

namespace Luminous
{
  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::render(Luminous::PrimitiveType type, int indexCount, int vertexCount, const Style & style)
  {
    RenderBuilder<Vertex, UniformBlock> builder;
    RenderCommand & cmd = createRenderCommand(indexCount, vertexCount, builder.idx,
                                              builder.vertex, builder.uniform, builder.depth, style);
    cmd.primitiveType = type;
    return builder;
  }

  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::drawRectT(
      const QRectF & area, Style & style)
  {
    if(!style.fill.shader)
      style.fill.shader = &basicShader();
    RenderBuilder<Vertex, UniformBlock> b = render<Vertex, UniformBlock>(Luminous::PrimitiveType_TriangleStrip, 4, 4, style);
    auto v = b.vertex;
    auto idx = b.idx;

    v++->location.make(area.left(), area.top(), b.depth);
    v++->location.make(area.right(), area.top(), b.depth);
    v++->location.make(area.left(), area.bottom(), b.depth);
    v->location.make(area.right(), area.bottom(), b.depth);

    *idx++ = 0;
    *idx++ = 1;
    *idx++ = 2;
    *idx = 3;

    b.uniform->projMatrix = viewTransform();
    b.uniform->modelMatrix = transform4();
    b.uniform->color = style.fill.color;

    return b;
  }

  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::drawTexRectT(
      const QRectF & area, Style & style)
  {
    if(!style.fill.shader)
      style.fill.shader = &texShader();
    RenderBuilder<Vertex, UniformBlock> b = render<Vertex, UniformBlock>(Luminous::PrimitiveType_TriangleStrip, 4, 4, style);
    auto v = b.vertex;
    auto idx = b.idx;

    v->location.make(area.left(), area.top(), b.depth);
    v++->texCoord.make(0, 0);

    v->location.make(area.right(), area.top(), b.depth);
    v++->texCoord.make(1, 0);

    v->location.make(area.left(), area.bottom(), b.depth);
    v++->texCoord.make(0, 1);

    v->location.make(area.right(), area.bottom(), b.depth);
    v->texCoord.make(1, 1);

    *idx++ = 0;
    *idx++ = 1;
    *idx++ = 2;
    *idx = 3;

    b.uniform->projMatrix = viewTransform();
    b.uniform->modelMatrix = transform4();
    b.uniform->color = style.fill.color;

    return b;
  }

  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::drawRectWithHoleT(
      const QRectF & area, const QRectF & hole, Luminous::Style & style)
  {
    if(!style.fill.shader)
      style.fill.shader = &basicShader();

    RenderBuilder<Vertex, UniformBlock> b = render<Vertex, UniformBlock>(
          Luminous::PrimitiveType_TriangleStrip, 10, 8, style);
    auto v = b.vertex;
    auto idx = b.idx;

    v++->location.make(area.left(), area.top(), b.depth);
    v++->location.make(hole.left(), hole.top(), b.depth);

    v++->location.make(area.right(), area.top(), b.depth);
    v++->location.make(hole.right(), hole.top(), b.depth);

    v++->location.make(area.right(), area.bottom(), b.depth);
    v++->location.make(hole.right(), hole.bottom(), b.depth);

    v++->location.make(area.left(), area.bottom(), b.depth);
    v->location.make(hole.left(), hole.bottom(), b.depth);

    for(int i = 0; i < 8; ++i)
      *idx++ = i;
    *idx++ = 0;
    *idx++ = 1;

    b.uniform->projMatrix = viewTransform();
    b.uniform->modelMatrix = transform4();
    b.uniform->color = style.fill.color;

    return b;
  }
}

#endif // LUMINOUS_RENDER_CONTEXT_IMPL_HPP
