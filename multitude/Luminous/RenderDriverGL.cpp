#include "Luminous/RenderDriverGL.hpp"
#include "Luminous/VertexAttributeBinding.hpp"
#include "Luminous/VertexDescription.hpp"
#include "Luminous/HardwareBuffer.hpp"
#include "Luminous/ShaderConstantBlock.hpp"
#include "Luminous/GLUtils.hpp"

#include <Nimble/Matrix4.hpp>
#include <Radiant/RefPtr.hpp>

#include <cassert>
#include <map>
#include <vector>

namespace Luminous
{
  class RenderDriverGL::Impl
  {
  public:
    Impl(unsigned int threadCount)
      : resourceId(0)
      , currentProgram(threadCount, 0)
      , threadResources(threadCount)
    {}

    RenderResource::Id createId()
    {
      return resourceId++;
    }

    // Next free available resource ID
    RenderResource::Id resourceId;

    // Hardware buffer management
    struct ResourceHandle {
      GLuint handle;
      uint64_t version;
      size_t size;
    };

    typedef std::map<RenderResource::Id, ResourceHandle> ResourceList;
    typedef std::vector<ResourceList> ThreadResources;

    // Thread state
    std::vector<GLuint> currentProgram;

    // Thread resources
    ThreadResources threadResources;
  };

  RenderDriverGL::RenderDriverGL(unsigned int threadId)
    : m_impl(new RenderDriverGL::Impl(threadId))
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
    return std::make_shared<VertexAttributeBinding>(m_impl->createId());
  }

  std::shared_ptr<HardwareBuffer> RenderDriverGL::createVertexBuffer()
  {
    return std::make_shared<HardwareBuffer>(m_impl->createId(), BT_VertexBuffer);
  }

  std::shared_ptr<ShaderConstantBlock> RenderDriverGL::createConstantBuffer()
  {
    return std::make_shared<ShaderConstantBlock>(m_impl->createId());
  }

  void RenderDriverGL::preFrame(unsigned int threadIndex)
  {

  }

  void RenderDriverGL::postFrame(unsigned int threadIndex)
  {

  }

  void RenderDriverGL::bind(unsigned int threadIndex, const HardwareBuffer & buffer)
  {
    // See if the buffer exists in cache
    Impl::ResourceHandle h;
    Impl::ResourceList::iterator it = m_impl->threadResources[threadIndex].find(buffer.resourceId());
    if (it == m_impl->threadResources[threadIndex].end()) {
      // No hit, create new buffer
      glGenBuffers(1, &h.handle);
      h.version = 0;
      h.size = 0;
    }
    else {
      // Existing buffer
      h = it->second;
    }

    // Bind it
    GLenum bufferTarget = Luminous::GLUtils::getBufferType(buffer.type());
    glBindBuffer(bufferTarget, h.handle);

    // Update if dirty
    if (h.version < buffer.version()) {
      // Update or reallocate
      if (buffer.size() != h.size)
        glBufferData(bufferTarget, buffer.size(), buffer.data(), buffer.usage() );
      else 
        glBufferSubData(bufferTarget, 0, buffer.size(), buffer.data());

      // Update version/size
      h.version = buffer.version();
      h.size = buffer.size();
    }

    // Update cache
    m_impl->threadResources[threadIndex][buffer.resourceId()] = h;
  }

  void RenderDriverGL::bind(unsigned int threadIndex, const VertexAttributeBinding & binding)
  {
    Impl::ResourceHandle h;
    Impl::ResourceList::iterator it = m_impl->threadResources[threadIndex].find(binding.resourceId());
    if (it == m_impl->threadResources[threadIndex].end()) {
      // No hit, create new vertex array
      glGenVertexArrays(1, &h.handle);
      h.version = 0;
    }
    else {
      // Existing array
      h = it->second;
    }

    glBindVertexArray(h.handle);

    // Update if dirty
    if (h.version < binding.version()) {
      /// @todo Should we just kill the old one to get rid of old bindings?
      for (size_t i = 0; i < binding.bindingCount(); ++i) {
        VertexAttributeBinding::Binding b = binding.binding(i);
        // Attach buffer
        bind(threadIndex, *b.buffer);
        // Set buffer attributes
        for (size_t attrIndex = 0; attrIndex < b.description->attributeCount(); ++attrIndex) {
          VertexAttribute attr = b.description->attribute(attrIndex);
          /// @todo Should cache these locations
          GLint location = glGetAttribLocation(m_impl->currentProgram[threadIndex], attr.name.toAscii().data());
          GLenum normalized = (attr.normalized ? GL_TRUE : GL_FALSE);
          glVertexAttribPointer(location, attr.count, GLUtils::getDataType(attr.type), normalized, b.description->vertexSize(), (const GLvoid*)attr.offset);
        }
      }

      // Update cache
      h.version = binding.version();
      m_impl->threadResources[threadIndex][binding.resourceId()] = h;
    }
  }

  /// @todo: duplicate code from buffer binding: make functions
  void RenderDriverGL::bind(unsigned int threadIndex, const ShaderConstantBlock & constants)
  {
    Impl::ResourceHandle h;
    Impl::ResourceList::iterator it = m_impl->threadResources[threadIndex].find(constants.resourceId());

    // See if the buffer exists in cache
    if (it == m_impl->threadResources[threadIndex].end()) {
      // No hit, create new buffer
      glGenBuffers(1, &h.handle);
      h.version = 0;
      h.size = 0;
    }
    else {
      // Existing buffer
      h = it->second;
    }

    // Bind buffer
    glBindBuffer(GL_UNIFORM_BUFFER_EXT, h.handle);

    // Update if dirty
    if (h.version < constants.version()) {
      if (h.size != constants.size())
        glBufferData(GL_UNIFORM_BUFFER_EXT, constants.size(), constants.data(), GL_DYNAMIC_DRAW );
      else 
        glBufferSubData(GL_UNIFORM_BUFFER_EXT, 0, constants.size(), constants.data());
    }
  }

  void RenderDriverGL::unbind(unsigned int threadIndex, const HardwareBuffer & buffer)
  {
    /// @todo Should we verify it's a valid resource?
    GLenum bufferTarget = Luminous::GLUtils::getBufferType(buffer.type());
    glBindBuffer(bufferTarget, 0);
  }

  void RenderDriverGL::unbind(unsigned int threadIndex, const VertexAttributeBinding & binding)
  {
    /// @todo Should we verify it's a valid resource?
    glBindVertexArray(0);
  }

  void RenderDriverGL::unbind(unsigned int threadIndex, const ShaderConstantBlock & constants)
  {
    glBindBuffer(GL_UNIFORM_BUFFER_EXT, 0);
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

  void RenderDriverGL::draw(PrimitiveType type, size_t offset, size_t primitives)
  {
    GLenum mode = GLUtils::getPrimitiveType(type);
    glDrawArrays(mode, (GLint) offset, (GLsizei) primitives);
  }

  void RenderDriverGL::drawIndexed(PrimitiveType type, size_t offset, size_t primitives)
  {
    /// @todo allow other index types (unsigned byte, unsigned short and unsigned int)
    GLenum mode = GLUtils::getPrimitiveType(type);
    glDrawElements(mode, (GLsizei) primitives, GL_UNSIGNED_INT, (const GLvoid *)((sizeof(uint) * offset)));
  }
}
