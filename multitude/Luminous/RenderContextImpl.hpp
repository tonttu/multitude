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
    cmd.vertexOffset = offset;                                            // Vertex offset (for indexed and non-indexed)
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
    const Program & shader, const Radiant::Color & color, float width, const Luminous::Style & style)
  {
    /// @todo Should we be able to overrule this with Style::Translucent
    bool translucent = shader.translucent() || (color.w * opacity() < 0.99999999f) ||
        style.fill().hasTranslucentTextures();

    RenderBuilder<Vertex,UniformBlock> b = render<Vertex, UniformBlock>(translucent, primType, indexCount, vertexCount, width, shader,
                                          &style.fill().textures(),
                                          &style.fill().uniforms());

    // Set the color
    b.uniform->color = color;
    // Apply opacity
    b.uniform->color.w *= opacity();
    // Set default depth
    b.uniform->depth = b.depth;

    return b;
  }
}

#endif // LUMINOUS_RENDER_CONTEXT_IMPL_HPP
