#ifndef LUMINOUS_RENDER_CONTEXT_IMPL_HPP
#define LUMINOUS_RENDER_CONTEXT_IMPL_HPP

namespace Luminous
{
  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::render(
    bool translucent,
    Luminous::PrimitiveType type, int indexCount, int vertexCount, float primitiveSize,
    const Luminous::Program & program, const std::map<QByteArray, const Texture *> & textures)
  {
    RenderBuilder<Vertex, UniformBlock> builder;
    RenderCommand & cmd = createRenderCommand(translucent,
                                              indexCount, vertexCount,
                                              builder.idx, builder.vertex, builder.uniform,
                                              builder.depth,
                                              program, textures);
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

    return builder;
  }
    
  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::drawPrimitiveT(
    Luminous::PrimitiveType primType, const Nimble::Vector2f * vertices, unsigned int vertexCount,
    const Program & shader, const Radiant::Color & color, float width)
  {
    /// @todo Should we be able to overrule this with Style::Translucent
    bool translucent =
      shader.translucent() |
      (color.w < 0.99999999f);

    /// @todo temporary dummy to avoid separate render() function
    const std::map<QByteArray, const Texture *> textures;

    RenderBuilder<Vertex, UniformBlock> b = render<Vertex, UniformBlock>(translucent, primType, vertexCount, vertexCount, width, shader, textures);
    auto v = b.vertex;
    auto idx = b.idx;

    for (unsigned int i = 0; i < vertexCount; ++i) {
      v++->location.make(vertices[i].x, vertices[i].y, b.depth);
      *idx++ = i;
    }

    // Set the color
    b.uniform->color = color;
    // Apply opacity
    b.uniform->color.w *= opacity();

    return b;
  }

  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::drawTexPrimitiveT(
    Luminous::PrimitiveType primType, const Nimble::Vector2f * vertices, const Nimble::Vector2f * uvs, unsigned int vertexCount,
    const Program & shader, const std::map<QByteArray, const Texture *> & textures, const Radiant::Color & color, float width)
  {
    /// @todo Should we be able to overrule this with Style::Translucent
    bool translucent =
      shader.translucent() |
      (color.w < 0.99999999f);

    RenderBuilder<Vertex, UniformBlock> b = render<Vertex, UniformBlock>(translucent, primType, vertexCount, vertexCount, width, shader, textures);
    auto v = b.vertex;
    auto idx = b.idx;

    for (unsigned int i = 0; i < vertexCount; ++i) {
      v->location.make(vertices[i].x, vertices[i].y, b.depth);
      v++->texCoord = uvs[i];
      *idx++ = i;
    }

    // Set the color
    b.uniform->color = color;
    // Apply opacity
    b.uniform->color.w *= opacity();

    return b;
  }
}

#endif // LUMINOUS_RENDER_CONTEXT_IMPL_HPP
