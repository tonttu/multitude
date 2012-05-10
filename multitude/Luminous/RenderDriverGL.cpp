#include "Luminous/RenderDriverGL.hpp"
#include "Luminous/VertexAttributeBindingGL.hpp"
#include "Luminous/VertexDescription.hpp"
#include "Luminous/HardwareBufferGL.hpp"
#include "Luminous/ShaderConstantsGL.hpp"
#include "Luminous/GLUtils.hpp"

#include <Nimble/Matrix4.hpp>
#include <Radiant/RefPtr.hpp>
#include <cassert>

namespace Luminous
{
  class RenderDriverGL::Impl
  {
  public:
    Impl(unsigned int threadId, unsigned int threadCount)
      : threadId(threadId)
      , threadCount(threadCount)
      , currentProgram(0)
      , resourceId(0)
    {}

    RenderResource::Id createId()
    {
      return resourceId++;
    }

    // ThreadID that is serviced by this driver
    unsigned int threadId;
    // Total number of render threads
    unsigned int threadCount;
    // Currently bound shader program
    GLuint currentProgram;

    // Next free available resource ID
    RenderResource::Id resourceId;
  };

  RenderDriverGL::RenderDriverGL(unsigned int threadId, unsigned int threadCount)
    : m_impl(new RenderDriverGL::Impl(threadId, threadCount))
  {

  }

  RenderDriverGL::~RenderDriverGL()
  {
    delete m_impl;
  }

  std::shared_ptr<VertexDescription> RenderDriverGL::createVertexDescription()
  {
    return std::make_shared<VertexDescription>();
  }

  std::shared_ptr<VertexAttributeBinding> RenderDriverGL::createVertexAttributeBinding()
  {
    return std::make_shared<VertexAttributeBindingGL>(m_impl->createId(), m_impl->threadCount);
  }

  std::shared_ptr<HardwareBuffer> RenderDriverGL::createVertexBuffer(size_t vertexSize, size_t vertexCount, BufferUsage usage)
  {
    return std::make_shared<HardwareBufferGL>(m_impl->createId(), BT_VertexBuffer, m_impl->threadCount);
  }

  std::shared_ptr<HardwareBuffer> RenderDriverGL::createConstantBuffer(BufferUsage usage)
  {
    return std::make_shared<ShaderConstantsGL>(usage, m_impl->createId(), m_impl->threadCount);
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
