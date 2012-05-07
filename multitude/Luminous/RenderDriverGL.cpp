#include "Luminous/RenderDriverGL.hpp"
#include "Luminous/VertexAttributeBindingGL.hpp"
#include "Luminous/VertexDescription.hpp"
#include "Luminous/HardwareBufferGL.hpp"
#include "Luminous/GLUtils.hpp"

#include <Nimble/Matrix4.hpp>
#include <Radiant/RefPtr.hpp>
#include <cassert>

namespace Luminous
{
  class RenderDriverGL::Impl
  {
    enum { max_textures = 8 };
  public:
    Impl() {}

    // Render state
    std::shared_ptr<ShaderGLSL> m_currentShader;
    std::shared_ptr<HardwareBuffer> m_currentVertexBuffer;
    std::shared_ptr<HardwareBuffer> m_currentIndexBuffer;

    //std::shared_ptr<Texture2> m_currentTexture[max_textures];
  };

  RenderDriverGL::RenderDriverGL()
    : m_impl(std::make_shared<RenderDriverGL::Impl>())
  {

  }

  std::shared_ptr<VertexDescription> RenderDriverGL::createVertexDescription()
  {
    return std::make_shared<VertexDescription>();
  }

  std::shared_ptr<VertexAttributeBinding> RenderDriverGL::createVertexAttributeBinding()
  {
    /// @todo implement (fetch threadCount from system)
    return std::shared_ptr<VertexAttributeBinding>();
  }

  std::shared_ptr<HardwareBuffer> RenderDriverGL::createVertexBuffer(size_t vertexSize, size_t vertexCount, BufferUsage usage)
  {
    /// @todo implement (fetch threadCount from system)
    return std::shared_ptr<HardwareBuffer>();
  }

  void RenderDriverGL::clear(ClearMask mask, const Radiant::Color & color, double depth, int stencil)
  {
    /// @todo check current target for depth and stencil buffers?

    GLbitfield glMask = 0;
    // Clear color buffer
    if (mask & CM_Color) {
      glClearColor(color.red(), color.green(), color.blue(), color.alpha());
      glMask |= GL_COLOR_BUFFER_BIT;
    }
    // Clear depth buffer
    if (mask & CM_Depth) {
      glClearDepth(depth);
      glMask |= GL_DEPTH_BUFFER_BIT;
    }
    // Clear stencil buffer
    if (mask & CM_Stencil) {
      glClearStencil(stencil);
      glMask |= GL_DEPTH_BUFFER_BIT;
    }
    glClear(glMask);
  }

  void RenderDriverGL::drawArrays(PrimitiveType type, size_t offset, size_t primitives)
  {
    GLenum mode = GLUtils::getPrimitiveType(type);
    glDrawArrays(mode, (GLint) offset, (GLsizei) primitives);
  }

  void RenderDriverGL::drawIndexedArrays(PrimitiveType type, size_t offset, size_t primitives)
  {
    /// @todo allow other index types (unsigned byte, unsigned short and unsigned int)
    GLenum mode = GLUtils::getPrimitiveType(type);
    glDrawElements(mode, (GLsizei) primitives, GL_UNSIGNED_INT, (const GLvoid *)((sizeof(uint) * offset)));
  }
}
