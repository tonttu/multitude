#include "Luminous/RenderDriverGL.hpp"
#include "Luminous/VertexAttributeBinding.hpp"
#include "Luminous/VertexDescription.hpp"
#include "Luminous/HardwareBuffer.hpp"
#include "Luminous/ShaderConstantBlock.hpp"
#include "Luminous/ShaderProgram.hpp"
#include "Luminous/GLUtils.hpp"
#include "Luminous/Utils.hpp"   // glCheck

#include <Nimble/Matrix4.hpp>
#include <Radiant/RefPtr.hpp>

#include <cassert>
#include <map>
#include <vector>

#if RADIANT_DEBUG
# define GLERROR(txt) Utils::glCheck(txt)
#else
# define GLERROR(txt)
#endif

namespace Luminous
{
  class RenderDriverGL::D
  {
  public:
    // Some types
    struct ResourceHandle
    {
      ResourceHandle() : handle(0), generation(0), type(RT_Unknown), size(0) {}
      ResourceHandle(ResourceType type) : handle(0), generation(0), type(type), size(0) {}
      ResourceType type;
      GLuint handle;
      uint64_t generation;
      size_t size;
    };
    typedef std::map<RenderResource::Id, ResourceHandle> ResourceList;

    // Current state of a single thread
    struct ThreadState
    {
      ThreadState()
        : currentProgram(0)
      {
      }

      GLuint currentProgram;      // Currently bound shader program
      ResourceList resourceList;  // Active resources
    };

  public:
    D(unsigned int threadCount)
      : resourceId(0)
      , m_threadResources(threadCount)
    {}

    RenderResource::Id createId()
    {
      return resourceId++;
    }

    ResourceHandle getOrCreateResource(unsigned int threadIndex, const RenderResource & resource)
    {
      // See if we can find it in cache
      ResourceList & res = m_threadResources[threadIndex].resourceList;
      ResourceList::iterator it = res.find(resource.resourceId());
      if (it != res.end())
        return it->second;

      // No hit, create new resource
      ResourceHandle resourceHandle(resource.resourceType());
      resourceHandle.handle = createGLResource(resource.resourceType());
      // Update cache
      updateResource(threadIndex, resource.resourceId(), resourceHandle);
      return resourceHandle;
    }

    void updateResource(unsigned int threadIndex, RenderResource::Id id, ResourceHandle handle)
    {
      m_threadResources[threadIndex].resourceList[id] = handle;
    }

  private:
    GLuint createGLResource(ResourceType type)
    {
      GLuint resource;
      switch (type)
      {
      case RT_VertexArray: glGenVertexArrays(1, &resource); return resource;
      case RT_Buffer: glGenBuffers(1, &resource); return resource;
      case RT_ShaderProgram: return glCreateProgram();
      case RT_VertexShader: return glCreateShader(GL_VERTEX_SHADER);
      case RT_FragmentShader: return glCreateShader(GL_FRAGMENT_SHADER);
      case RT_GeometryShader: return glCreateShader(GL_GEOMETRY_SHADER_EXT);
      default:
        Radiant::error("RenderDriverGL: Can't create GL resource: unknown type %d", type);
        assert(false);
        return 0;
      }
    }

  public:
    RenderResource::Id resourceId;              // Next free available resource ID
    std::vector<ThreadState> m_threadResources;   // Thread resources
  };

  RenderDriverGL::RenderDriverGL(unsigned int threadCount)
    : m_d(new RenderDriverGL::D(threadCount))
  {

  }

  RenderDriverGL::~RenderDriverGL()
  {
    delete m_d;
  }

  std::shared_ptr<VertexDescription> RenderDriverGL::createVertexDescription()
  {
    return std::make_shared<VertexDescription>();
  }

  std::shared_ptr<VertexAttributeBinding> RenderDriverGL::createVertexAttributeBinding()
  {
    /// @todo use make_shared once fully available
    return std::shared_ptr<VertexAttributeBinding>(new VertexAttributeBinding(m_d->createId(), *this));
  }

  std::shared_ptr<HardwareBuffer> RenderDriverGL::createHardwareBuffer(BufferType type)
  {
    /// @todo use make_shared once fully available
    return std::shared_ptr<HardwareBuffer>(new HardwareBuffer(m_d->createId(), type, *this));
  }

  std::shared_ptr<ShaderConstantBlock> RenderDriverGL::createShaderConstantBlock()
  {
    /// @todo this should be done with createHardwareBuffer once ShaderConstantBlock has been redesigned
    /// @todo use make_shared once fully available
    return std::shared_ptr<ShaderConstantBlock>(new ShaderConstantBlock(m_d->createId(), *this));
  }

  std::shared_ptr<ShaderProgram> RenderDriverGL::createShaderProgram()
  {
    /// @todo use make_shared once fully available
    return std::shared_ptr<ShaderProgram>(new ShaderProgram(m_d->createId(),*this));
  }

  std::shared_ptr<ShaderGLSL> RenderDriverGL::createShader(ShaderType type)
  {
    /// @todo use make_shared once fully available
    return std::shared_ptr<ShaderGLSL>(new ShaderGLSL(m_d->createId(), type, *this));
  }

  void RenderDriverGL::preFrame(unsigned int threadIndex)
  {
  }

  void RenderDriverGL::postFrame(unsigned int threadIndex)
  {
  }

  void RenderDriverGL::bind(unsigned int threadIndex, const ShaderProgram & program)
  {
    m_d->m_threadResources[threadIndex].currentProgram = 0;

    D::ResourceHandle programHandle = m_d->getOrCreateResource(threadIndex, program);

    // Check if any of the shaders are dirty
    bool needsRelink = false;

    // Recreate if the program is dirty
    if (programHandle.generation < program.generation()) {
      needsRelink = true;

      glDeleteProgram(programHandle.handle);
      GLERROR("RenderDriverGL::Bind ShaderProgram delete");
      programHandle.handle = glCreateProgram();
      programHandle.generation = program.generation();
    }

    for (size_t i = 0; i < program.shaderCount(); ++i) {
      const std::shared_ptr<ShaderGLSL> & shader = program.shader(i);
      D::ResourceHandle shaderHandle = m_d->getOrCreateResource(threadIndex, *shader);
      if (shaderHandle.generation < shader->generation()) {
        needsRelink = true;
        /// @todo Detach old shader? (if it exists)
        //glDetachShader(programHandle.handle, shaderHandle.handle);
        //GLERROR("RenderDriverGL::Bind ShaderProgram detach");

        // Recompile and link
        const QByteArray shaderData = shader->text().toAscii();
        const GLchar * text = shaderData.data();
        const GLint length = shaderData.size();
        glShaderSource(shaderHandle.handle, 1, &text, &length);
        GLERROR("RenderDriverGL::Bind ShaderProgram source");
        glCompileShader(shaderHandle.handle);
        GLERROR("RenderDriverGL::Bind ShaderProgram compile");
        GLint compiled = GL_FALSE;
        glGetShaderiv(shaderHandle.handle, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_TRUE) {
          glAttachShader(programHandle.handle, shaderHandle.handle);
          GLERROR("RenderDriverGL::Bind ShaderProgram attach");
          // All seems okay, update resource handle
          shaderHandle.generation = shader->generation();
          m_d->updateResource(threadIndex, shader->resourceId(), shaderHandle);
        }
        else {
          Radiant::error("Failed to compile shader %d (type %d)", shaderHandle.handle, shaderHandle.type);

          // Dump info log
          GLsizei len;
          glGetShaderiv(shaderHandle.handle, GL_INFO_LOG_LENGTH, &len);
          std::vector<GLchar> log(len);
          glGetShaderInfoLog(shaderHandle.handle, len, &len, log.data());
          Radiant::error("%s", log.data());
          // Abort binding
          return;
        }
      }
    }
    if (needsRelink) {
      glLinkProgram(programHandle.handle);
      GLERROR("RenderDriverGL::Bind ShaderProgram link");
      // Check for linking errors
      GLint status;
      glGetProgramiv(programHandle.handle, GL_LINK_STATUS, &status);
      if (status == GL_FALSE) {
        Radiant::error("Failed to link shader program %d", programHandle.handle);
        GLsizei len;
        glGetProgramiv(programHandle.handle, GL_INFO_LOG_LENGTH, &len);
        std::vector<GLchar> log(len);
        glGetProgramInfoLog(programHandle.handle, len, &len, log.data());
        Radiant::error("%s", log.data());
        // Can't link: abort binding
        return;
      }
    }

    m_d->m_threadResources[threadIndex].currentProgram = programHandle.handle;
    glUseProgram(programHandle.handle);
    GLERROR("RenderDriverGL::Bind Shaderprogram use");
  }

  void RenderDriverGL::bind(unsigned int threadIndex, const HardwareBuffer & buffer)
  {
    D::ResourceHandle bufferHandle = m_d->getOrCreateResource(threadIndex, buffer);

    // Bind buffer
    GLenum bufferTarget = Luminous::GLUtils::getBufferType(buffer.type());
    glBindBuffer(bufferTarget, bufferHandle.handle);
    GLERROR("RenderDriverGL::Bind HardwareBuffer");

    // Update if dirty
    if (bufferHandle.generation < buffer.generation()) {
      // Update or reallocate the resource
      if (buffer.size() != bufferHandle.size) {
        GLenum bufferUsage = Luminous::GLUtils::getBufferUsage(buffer.usage());
        glBufferData(bufferTarget, buffer.size(), buffer.data(), bufferUsage);
        GLERROR("RenderDriverGL::Bind HardwareBuffer reallocate");
      }
      else {
        glBufferSubData(bufferTarget, 0, buffer.size(), buffer.data());
        GLERROR("RenderDriverGL::Bind HardwareBuffer update");
      }

      // Update cache handle
      bufferHandle.generation = buffer.generation();
      bufferHandle.size = buffer.size();
      m_d->updateResource(threadIndex, buffer.resourceId(), bufferHandle);
    }
  }

  void RenderDriverGL::bind(unsigned int threadIndex, const VertexAttributeBinding & binding)
  {
    D::ResourceHandle bindingHandle = m_d->getOrCreateResource(threadIndex, binding);

    if (bindingHandle.generation < binding.generation()) {
      // Recreate the binding
      /// @todo I don't imagine these can really be updated, right?
      glDeleteVertexArrays(1, &bindingHandle.handle);
      glGenVertexArrays(1, &bindingHandle.handle);
      glBindVertexArray(bindingHandle.handle);
      GLERROR("RenderDriverGL::Bind VertexAttributeBinding recreate");

      // Bind all vertex buffers
      for (size_t i = 0; i < binding.bindingCount(); ++i) {
        VertexAttributeBinding::Binding b = binding.binding(i);
        // Attach buffer
        bind(threadIndex, *b.buffer);
        // Set buffer attributes
        for (size_t attrIndex = 0; attrIndex < b.description->attributeCount(); ++attrIndex) {
          VertexAttribute attr = b.description->attribute(attrIndex);
          /// @todo Should cache these locations
          GLint location = glGetAttribLocation(m_d->m_threadResources[threadIndex].currentProgram, attr.name.toAscii().data());
          if (location == -1) {
            Radiant::warning("Unable to bind vertex attribute %s", attr.name.toAscii().data());
          }
          else {
            GLenum normalized = (attr.normalized ? GL_TRUE : GL_FALSE);
            glVertexAttribPointer(location, attr.count, GLUtils::getDataType(attr.type), normalized, b.description->vertexSize(), (const GLvoid*)attr.offset);
            glEnableVertexAttribArray(location);
            GLERROR("RenderDriverGL::Bind VertexAttributeBinding vertexAttribPointer");
          }
        }

        // Update cache
        bindingHandle.generation = binding.generation();
        m_d->updateResource(threadIndex, binding.resourceId(), bindingHandle);
      }
    }
    else {
      glBindVertexArray(bindingHandle.handle);
      GLERROR("RenderDriverGL::Bind VertexAttributeBinding bind");
    }
  }

  void RenderDriverGL::bind(unsigned int threadIndex, const ShaderConstantBlock & constants)
  {
    D::ResourceHandle bufferHandle = m_d->getOrCreateResource(threadIndex, constants);
    
    // Bind buffer
    glBindBuffer(GL_UNIFORM_BUFFER_EXT, bufferHandle.handle);

    /// @note: Basically same code as buffer binding with few different arguments
    // Update if dirty
    if (bufferHandle.generation < constants.generation()) {
      // Update or reallocate the resource
      if (constants.size() != bufferHandle.size) {
        /// @todo What should be the usage type for a uniform buffer? Should it be configurable like a normal buffer?
        glBufferData(GL_UNIFORM_BUFFER_EXT, constants.size(), constants.data(), GL_DYNAMIC_DRAW);
      }
      else {
        glBufferSubData(GL_UNIFORM_BUFFER_EXT, 0, constants.size(), constants.data());
      }

      // Update cache handle
      bufferHandle.generation = constants.generation();
      bufferHandle.size = constants.size();
      m_d->updateResource(threadIndex, constants.resourceId(), bufferHandle);
    }
  }

  void RenderDriverGL::unbind(unsigned int, const HardwareBuffer & buffer)
  {
    /// @todo Should we verify it's a valid resource?
    GLenum bufferTarget = Luminous::GLUtils::getBufferType(buffer.type());
    glBindBuffer(bufferTarget, 0);
  }

  void RenderDriverGL::unbind(unsigned int, const VertexAttributeBinding &)
  {
    /// @todo Should we verify it's a valid resource?
    glBindVertexArray(0);
  }

  void RenderDriverGL::unbind(unsigned int, const ShaderConstantBlock &)
  {
    /// @todo Should we verify it's a valid resource?
    glBindBuffer(GL_UNIFORM_BUFFER_EXT, 0);
  }

  void RenderDriverGL::unbind(unsigned int, const ShaderProgram &)
  {
    /// @todo Should we verify it's a valid resource?
    glUseProgram(0);
  }

  void RenderDriverGL::setViewport(unsigned int threadIndex, float x, float y, float width, float height)
  {
/*
    const std::shared_ptr<RenderTarget> & target = m_d->m_threadResources[threadIndex].currentTarget;
    GLint realX = target->width() * x;
    GLint realY = target->height() * y;
    GLsizei realWidth = target->width() * width;
    GLsizei realHeight = target->height() * height;

    glViewport(realX, realY, realWidth, realHeight);
    glScissor(realX, realY, realWidth, realHeight);
    glEnable(GL_SCISSOR_TEST);
*/
  }

  void RenderDriverGL::clear(ClearMask mask, const Radiant::Color & color, double depth, int stencil)
  {
    /// @todo check current target for availability of depth and stencil buffers?

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

#define SETUNIFORM(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderConstant(unsigned int threadIndex, const QString & name, const TYPE & value) { \
    GLint location = glGetUniformLocation(m_d->m_threadResources[threadIndex].currentProgram, name.toAscii().data()); \
    if (location != -1) FUNCTION(location, value); \
    return (location != -1); \
  }
#define SETUNIFORMVECTOR(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderConstant(unsigned int threadIndex, const QString & name, const TYPE & value) { \
    GLint location = glGetUniformLocation(m_d->m_threadResources[threadIndex].currentProgram, name.toAscii().data()); \
    if (location != -1) FUNCTION(location, 1, value.data()); \
    return (location != -1); \
  }
#define SETUNIFORMMATRIX(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderConstant(unsigned int threadIndex, const QString & name, const TYPE & value) { \
    GLint location = glGetUniformLocation(m_d->m_threadResources[threadIndex].currentProgram, name.toAscii().data()); \
    if (location != -1) FUNCTION(location, 1, GL_TRUE, value.data()); \
    return (location != -1); \
  }

  SETUNIFORM(int, glUniform1i);
  SETUNIFORM(float, glUniform1f);
  SETUNIFORMVECTOR(Nimble::Vector2i, glUniform2iv);
  SETUNIFORMVECTOR(Nimble::Vector3i, glUniform3iv);
  SETUNIFORMVECTOR(Nimble::Vector4i, glUniform4iv);
  SETUNIFORMVECTOR(Nimble::Vector2f, glUniform2fv);
  SETUNIFORMVECTOR(Nimble::Vector3f, glUniform3fv);
  SETUNIFORMVECTOR(Nimble::Vector4f, glUniform4fv);
  SETUNIFORMMATRIX(Nimble::Matrix2f, glUniformMatrix2fv);
  SETUNIFORMMATRIX(Nimble::Matrix3f, glUniformMatrix3fv);
  SETUNIFORMMATRIX(Nimble::Matrix4f, glUniformMatrix4fv);
#undef SETUNIFORM
#undef SETUNIFORMVECTOR
#undef SETUNIFORMMATRIX

  void RenderDriverGL::removeResource(RenderResource::Id id)
  {
    /// @todo Queue resource for removal
  }
}

#undef GLERROR