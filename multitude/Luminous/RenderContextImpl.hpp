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

    builder.command = &cmd;

    return builder;
  }
    
  template <typename Vertex, typename UniformBlock>
  RenderContext::RenderBuilder<Vertex, UniformBlock> RenderContext::drawPrimitiveT(
    Luminous::PrimitiveType primType, unsigned int indexCount, unsigned int vertexCount,
    const Program & shader, const Radiant::Color & color, float width, const Luminous::Style & style)
  {
    /// @todo Should we be able to overrule this with Style::Translucent
    bool translucent =
      shader.translucent() |
      (color.w < 0.99999999f);

    RenderBuilder<Vertex, UniformBlock> b = render<Vertex, UniformBlock>(translucent, primType, indexCount, vertexCount, width, shader, style.fill().textures());

    // Set the color
    b.uniform->color = color;
    // Apply opacity
    b.uniform->color.w *= opacity();

    // Set draw modes
    b.command->blendMode = style.blendMode();
    b.command->depthMode = style.depthMode();
    b.command->stencilMode = style.stencilMode();

    return b;
  }
}

#endif // LUMINOUS_RENDER_CONTEXT_IMPL_HPP
