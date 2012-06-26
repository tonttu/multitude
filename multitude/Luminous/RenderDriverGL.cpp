#include "Luminous/RenderDriverGL.hpp"
#include "Luminous/RenderManager.hpp"
#include "Luminous/VertexAttributeBinding.hpp"
#include "Luminous/VertexDescription.hpp"
#include "Luminous/HardwareBuffer.hpp"
#include "Luminous/ShaderProgram.hpp"
#include "Luminous/Texture2.hpp"
#include "Luminous/GLUtils.hpp"
#include "Luminous/PixelFormat.hpp"
#include "Luminous/Utils.hpp"   // glCheck

#include <Nimble/Matrix4.hpp>
#include <Radiant/RefPtr.hpp>
#include <Radiant/Timer.hpp>

#ifdef RADIANT_OSX
#include <OpenGL/gl3.h>
#endif

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
  //////////////////////////////////////////////////////////////////////////
  // Configuration

  /// PCIe bandwidth
  /// PCIe 1.0 x16: 4GB/sec (2001)
  /// PCIe 2.0 x16: 8GB/sec (2007)
  /// PCIe 3.0 x16: 15.8GB/sec (2011)
  static const int64_t upload_bytes_limit = ((uint64_t)4 << 30);

  //////////////////////////////////////////////////////////////////////////
  /// Resource handles
  template <typename T>
  struct ResourceHandle
  {
    ResourceHandle(const T * ptr = nullptr) : resource(ptr), handle(0), generation(0) { if (ptr) type = ptr->resourceType(); }
    ResourceType type;
    const T * resource;
    Radiant::Timer lastUsed;
    GLuint handle;
    uint64_t generation;
  };

  struct BufferHandle : public ResourceHandle<HardwareBuffer>
  {
    BufferHandle(const HardwareBuffer * ptr = nullptr) : ResourceHandle(ptr), usage(BufferUsage_Static_Draw), size(0), uploaded(0) {}
    
    BufferUsage usage;
    size_t size;        // Size (in bytes)
    size_t uploaded;    // Uploaded bytes (for incremental upload)
  };
  
  struct TextureHandle : public ResourceHandle<Texture>
  {
    TextureHandle(const Texture * ptr = nullptr) : ResourceHandle(ptr), size(0), uploaded(0) {}
    size_t size;      // Total size (in bytes)
    size_t uploaded;  // Bytes uploaded (for incremental uploading)
  };

  // Generic handle
  typedef ResourceHandle<ShaderProgram> ProgramHandle;
  typedef ResourceHandle<ShaderGLSL> ShaderHandle;
  typedef ResourceHandle<VertexAttributeBinding> BindingHandle;
  typedef ResourceHandle<VertexDescription> DescriptionHandle;

  //////////////////////////////////////////////////////////////////////////
  // RenderDriver implementation
  class RenderDriverGL::D
  {
  public:
    typedef std::map<RenderResource::Id, ProgramHandle> ProgramList;
    typedef std::map<RenderResource::Id, ShaderHandle> ShaderList;
    typedef std::map<RenderResource::Id, TextureHandle> TextureList;
    typedef std::map<RenderResource::Id, BufferHandle> BufferList;
    typedef std::map<RenderResource::Id, BindingHandle> BindingList;
    typedef std::map<RenderResource::Id, DescriptionHandle> DescriptionList;

    // Current state of a single thread
    struct ThreadState
    {
      ThreadState()
        : currentProgram(0)
        , currentBinding(0)
      {
        reset();
      }

      void reset()
      {
        uploadedBytes = 0;
        frame = 0;
        fps = 0.0;
      }

      typedef std::vector<GLuint> AttributeList;
      AttributeList activeAttributes;

      GLuint currentProgram;  // Currently bound shader program
      GLuint currentBinding;  // Currently bound vertex binding

      /// Resources
      ProgramList programs;
      ShaderList shaders;
      TextureList textures;
      BufferList buffers;
      BindingList bindings;
      DescriptionList descriptions;

      // Resources to be released
      typedef std::vector<RenderResource::Id> ReleaseQueue;
      ReleaseQueue releaseQueue;

      /// Render statistics
      int32_t uploadedBytes;
      Radiant::Timer frameTimer;
      uint64_t frame;
      double fps;
    };

  public:
    D(unsigned int threadCount)
      : m_threadResources(threadCount)
    {}

    void setVertexAttributes(unsigned int threadIndex, const VertexAttributeBinding & binding)
    {
      // Bind all vertex buffers
      for (size_t i = 0; i < binding.bindingCount(); ++i) {
        VertexAttributeBinding::Binding b = binding.binding(i);
        // Attach buffer
        auto * buffer = RenderManager::getBuffer(b.buffer);
        auto * descr = RenderManager::getVertexDescription(b.description);
        assert(buffer != nullptr && descr != nullptr);
        bindBuffer(threadIndex, GL_ARRAY_BUFFER, *buffer);
        setVertexDescription(threadIndex, *descr);
      }
    }

    void setVertexDescription(unsigned int threadIndex, const VertexDescription & description)
    {
      // Set buffer attributes from its bound VertexDescription
      for (size_t attrIndex = 0; attrIndex < description.attributeCount(); ++attrIndex) {
        VertexAttribute attr = description.attribute(attrIndex);

        GLint location = glGetAttribLocation(m_threadResources[threadIndex].currentProgram, attr.name.toAscii().data());
        if (location == -1) {
          Radiant::warning("Unable to bind vertex attribute %s", attr.name.toAscii().data());
        }
        else {
          GLenum normalized = (attr.normalized ? GL_TRUE : GL_FALSE);
          GLenum type = GLUtils::getDataType(attr.type);

          glVertexAttribPointer(location, attr.count, type, normalized, description.vertexSize(), reinterpret_cast<GLvoid *>(attr.offset));
          GLERROR("RenderDriverGL::Bind VertexAttributeBinding glVertexAttribPointer");
          glEnableVertexAttribArray(location);
          GLERROR("RenderDriverGL::Bind VertexAttributeBinding glEnableVertexAttribArray");
        }
      }
    }

    void bindBuffer(unsigned int threadIndex, GLenum bufferTarget, const HardwareBuffer & buffer)
    {
      auto & buffers = m_threadResources[threadIndex].buffers;
      auto it = buffers.find(buffer.resourceId());
      BufferHandle bufferHandle(&buffer);
      if (it == std::end(buffers))
        bufferHandle.handle = GLUtils::createResource(ResourceType_Buffer);
      else
        bufferHandle = it->second;

      // Reset usage timer
      bufferHandle.lastUsed.start();

      glBindBuffer(bufferTarget, bufferHandle.handle);
      GLERROR("RenderDriverGL::bindBuffer glBindBuffer");

      // Update if dirty
      if (bufferHandle.generation < buffer.generation()) {
        // Update or reallocate the resource
        /// @todo incremental upload
        if (buffer.size() != bufferHandle.size || buffer.usage() != bufferHandle.usage) {
          glBufferData(bufferTarget, buffer.size(), buffer.data(), GLUtils::getUsageFlags(buffer));
          GLERROR("RenderDriverGL::bindBuffer glBufferData");
        }
        else {
          glBufferSubData(bufferTarget, 0, buffer.size(), buffer.data());
          GLERROR("RenderDriverGL::bindBuffer glBufferSubData");
        }

        // Update cache handle
        bufferHandle.generation = buffer.generation();
        bufferHandle.size = buffer.size();
        bufferHandle.uploaded = buffer.size();
        bufferHandle.usage = buffer.usage();
      }
      // Update cache
      buffers[buffer.resourceId()] = bufferHandle;
    }

    // Utility function for resource cleanup
    template <typename ContainerType>
    void removeResource(ContainerType & container, const ThreadState::ReleaseQueue & releaseQueue)
    {
      auto it = std::begin(container);
      while (it != std::end(container)) {
        if (std::find( std::begin(releaseQueue), std::end(releaseQueue), it->first) != std::end(releaseQueue) ||  // First, check if resource has been deleted
          it->second.resource->expiration() > 0 && it->second.lastUsed.time() > it->second.resource->expiration())      // If not, we can check if it has expired
        {
          // Release the GL resource
          GLUtils::destroyResource(it->second.type, it->second.handle);
          // Remove from container
          it = container.erase(it);
        }
        else
          it++;
      }
    }

  public:
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
  
  void RenderDriverGL::clear(ClearMask mask, const Radiant::Color & color, double depth, int stencil)
  {
    /// @todo check current target for availability of depth and stencil buffers?

    GLbitfield glMask = 0;
    // Clear color buffer
    if (mask & ClearMask_Color) {
      glClearColor(color.red(), color.green(), color.blue(), color.alpha());
      glMask |= GL_COLOR_BUFFER_BIT;
    }
    // Clear depth buffer
    if (mask & ClearMask_Depth) {
      glClearDepth(depth);
      glMask |= GL_DEPTH_BUFFER_BIT;
    }
    // Clear stencil buffer
    if (mask & ClearMask_Stencil) {
      glClearStencil(stencil);
      glMask |= GL_DEPTH_BUFFER_BIT;
    }
    glClear(glMask);
  }

  void RenderDriverGL::draw(PrimitiveType type, unsigned int offset, unsigned int primitives)
  {
    GLenum mode = GLUtils::getPrimitiveType(type);
    glDrawArrays(mode, (GLint) offset, (GLsizei) primitives);
    GLERROR("RenderDriverGL::draw glDrawArrays");
  }

  void RenderDriverGL::drawIndexed(PrimitiveType type, unsigned int offset, unsigned int primitives)
  {
    /// @todo allow other index types (unsigned byte, unsigned short and unsigned int)
    GLenum mode = GLUtils::getPrimitiveType(type);
    glDrawElements(mode, (GLsizei) primitives, GL_UNSIGNED_INT, (const GLvoid *)((sizeof(uint) * offset)));
    GLERROR("RenderDriverGL::draw glDrawElements");
  }

  void RenderDriverGL::preFrame(unsigned int threadIndex)
  {
    /// Reset statistics
    m_d->m_threadResources[threadIndex].uploadedBytes = 0;
    m_d->m_threadResources[threadIndex].frameTimer.start();

    auto & r = m_d->m_threadResources[threadIndex];

    /// Clear out any released resources
    m_d->removeResource(r.bindings, r.releaseQueue);
    m_d->removeResource(r.buffers, r.releaseQueue);
    m_d->removeResource(r.descriptions, r.releaseQueue);
    m_d->removeResource(r.programs, r.releaseQueue);
    m_d->removeResource(r.shaders, r.releaseQueue);
    m_d->removeResource(r.textures, r.releaseQueue);
    r.releaseQueue.clear();
  }

  void RenderDriverGL::postFrame(unsigned int threadIndex)
  {
    auto & r = m_d->m_threadResources[threadIndex];
    double frameTime = r.frameTimer.time();

    // Update statistics
    r.frame++;
    r.fps = 1.0 / frameTime;
  }

#define SETUNIFORM(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderUniform(unsigned int threadIndex, const char * name, const TYPE & value) { \
    GLint location = glGetUniformLocation(m_d->m_threadResources[threadIndex].currentProgram, name); \
    if (location != -1) FUNCTION(location, value); \
    return (location != -1); \
  }
#define SETUNIFORMVECTOR(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderUniform(unsigned int threadIndex, const char * name, const TYPE & value) { \
    GLint location = glGetUniformLocation(m_d->m_threadResources[threadIndex].currentProgram, name); \
    if (location != -1) FUNCTION(location, 1, value.data()); \
    return (location != -1); \
  }
#define SETUNIFORMMATRIX(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderUniform(unsigned int threadIndex, const char * name, const TYPE & value) { \
    GLint location = glGetUniformLocation(m_d->m_threadResources[threadIndex].currentProgram, name); \
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

  void RenderDriverGL::setShaderProgram(unsigned int threadIndex, const ShaderProgram & program)
  {
    auto & programList = m_d->m_threadResources[threadIndex].programs;
    auto it = programList.find(program.resourceId());
    ProgramHandle programHandle(&program);
    if (it == std::end(programList)) {
      /// New program: create it
      programHandle.handle = GLUtils::createResource(program.resourceType());
      programHandle.generation = 0;
    }
    else
      programHandle = it->second;

    // Reset usage timer
    programHandle.lastUsed.start();

    /// Tag if program needs relinking
    bool needsRelinking = (programHandle.generation < program.generation());

    // Check if any of our shaders are dirty
    auto & shaderList = m_d->m_threadResources[threadIndex].shaders;
    for (size_t i = 0; i < program.shaderCount(); ++i) {
      auto & shader = program.shader(i);
      ShaderHandle shaderHandle(&shader);

      // Find the correct shader
      auto it = shaderList.find(shader.resourceId());
      if (it == std::end(shaderList)) {
        /// New shader: create it
        shaderHandle.handle = GLUtils::createResource(shader.resourceType());
        shaderHandle.generation = 0;
        shaderList[shader.resourceId()] = shaderHandle;
      }
      else
        shaderHandle = it->second;

      // Reset usage timer
      shaderHandle.lastUsed.start();

      /// Check if it needs updating
      if (shaderHandle.generation < shader.generation()) {
        // Set and compile source
        const QByteArray shaderData = shader.text().toAscii();
        const GLchar * text = shaderData.data();
        const GLint length = shaderData.size();
        glShaderSource(shaderHandle.handle, 1, &text, &length);
        glCompileShader(shaderHandle.handle);
        GLERROR("RenderDriverGL::setShaderProgram glCompileShader");
        GLint compiled = GL_FALSE;
        glGetShaderiv(shaderHandle.handle, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_TRUE) {
          // All seems okay, update resource handle
          shaderHandle.generation = shader.generation();
          shaderList[shader.resourceId()] = shaderHandle;

          // Attach to the program
          glAttachShader(programHandle.handle, shaderHandle.handle);
          GLERROR("RenderDriverGL::setShaderProgram glAttachShader");
          // Tag the program for relinking
          needsRelinking = true;
        }
        else {
          Radiant::error("Failed to compile shader %d", shaderHandle.handle);

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

    if (needsRelinking) {
      glLinkProgram(programHandle.handle);
      GLERROR("RenderDriverGL::setShaderProgram glLinkProgram");
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

      /// Update program handle
      programHandle.generation = program.generation();
      programList[program.resourceId()] = programHandle;
    }

    // Avoid re-applying the same shader
    if (m_d->m_threadResources[threadIndex].currentProgram != programHandle.handle) {
      m_d->m_threadResources[threadIndex].currentProgram = programHandle.handle;
      glUseProgram(programHandle.handle);
      GLERROR("RenderDriverGL::setShaderProgram glUseProgram");
    }
  }

  void RenderDriverGL::setVertexBuffer(unsigned int threadIndex, const HardwareBuffer & buffer)
  {
    m_d->bindBuffer(threadIndex, GL_ARRAY_BUFFER, buffer);
  }

  void RenderDriverGL::setIndexBuffer(unsigned int threadIndex, const HardwareBuffer & buffer)
  {
    m_d->bindBuffer(threadIndex, GL_ELEMENT_ARRAY_BUFFER, buffer);
  }

  void RenderDriverGL::setUniformBuffer(unsigned int threadIndex, const HardwareBuffer & buffer)
  {
    m_d->bindBuffer(threadIndex, GL_UNIFORM_BUFFER_EXT, buffer);
  }

  void RenderDriverGL::setVertexBinding(unsigned int threadIndex, const VertexAttributeBinding & binding)
  {
    auto & bindings = m_d->m_threadResources[threadIndex].bindings;
    auto it = bindings.find(binding.resourceId());
    BindingHandle bindingHandle(&binding);
    if (it == std::end(bindings))
      bindingHandle.handle = GLUtils::createResource(ResourceType_VertexArray);
    else
      bindingHandle = it->second;

    // Reset usage timer
    bindingHandle.lastUsed.start();

    /// @todo Recreate the VAO if the vertex description has changed
    if (bindingHandle.generation < binding.generation()) {
      // Recreate the binding
      GLUtils::destroyResource(ResourceType_VertexArray, bindingHandle.handle);
      bindingHandle.handle = GLUtils::createResource(ResourceType_VertexArray);
      // Bind
      glBindVertexArray(bindingHandle.handle);
      GLERROR("RenderDriverGL::setVertexBinding glBindVertexArray (recreate)");

      m_d->setVertexAttributes(threadIndex, binding);

      // Update cache
      bindingHandle.generation = binding.generation();
      bindings[binding.resourceId()] = bindingHandle;
    }
    else {
      if (m_d->m_threadResources[threadIndex].currentBinding != bindingHandle.handle) {
        glBindVertexArray(bindingHandle.handle);
        GLERROR("RenderDriverGL::setVertexBinding glBindVertexArray");
        m_d->m_threadResources[threadIndex].currentBinding = bindingHandle.handle;
      }

      // Make sure the buffers get updated if necessary
      for (size_t i = 0; i < binding.bindingCount(); ++i) {
        VertexAttributeBinding::Binding b = binding.binding(i);
        auto * buffer = RenderManager::getBuffer(b.buffer);
        assert(buffer != nullptr);
        setVertexBuffer(threadIndex, *buffer);
      }
    }
  }

  void RenderDriverGL::setTexture(unsigned int threadIndex, unsigned int textureUnit, const Texture & texture)
  {
    auto & textures = m_d->m_threadResources[threadIndex].textures;
    auto it = textures.find(texture.resourceId());
    TextureHandle textureHandle(&texture);
    if (it == std::end(textures) )
      textureHandle.handle = GLUtils::createResource(ResourceType_Texture);
    else
      textureHandle = it->second;

    // Reset usage timer
    textureHandle.lastUsed.start();

    GLenum target;
    if (texture.dimensions() == 1)      target = GL_TEXTURE_1D;
    else if (texture.dimensions() == 2) target = GL_TEXTURE_2D;
    else if (texture.dimensions() == 3) target = GL_TEXTURE_3D;

    /// Specify the external format (number of channels)
    GLenum extFormat;
    if (texture.format().numChannels() == 1)      extFormat = GL_RED;
    else if (texture.format().numChannels() == 2) extFormat = GL_RG;
    else if (texture.format().numChannels() == 3) extFormat = GL_RGB;
    else if (texture.format().numChannels() == 4) extFormat = GL_RGBA;

    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(target, textureHandle.handle);
    GLERROR("RenderDriverGL::setVertexBinding glBindTexture");

    if (textureHandle.generation < texture.generation()) {
      // Start the uploading
      textureHandle.uploaded = 0;

      if (texture.dimensions() == 1)      glTexImage1D(GL_TEXTURE_1D, 0, texture.format().layout(), texture.width(), 0, extFormat , texture.format().type(), nullptr );
      else if (texture.dimensions() == 2) glTexImage2D(GL_TEXTURE_2D, 0, texture.format().layout(), texture.width(), texture.height(), 0, extFormat, texture.format().type(), nullptr );
      else if (texture.dimensions() == 3) glTexImage3D(GL_TEXTURE_3D, 0, texture.format().layout(), texture.width(), texture.height(), texture.depth(), 0, extFormat, texture.format().type(), nullptr );

      /// @todo Get these from the texture settings
      glTexParameteri(target,  GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(target,  GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      textureHandle.size = texture.width() * texture.height() * texture.depth() * texture.format().bytesPerPixel();
      textureHandle.uploaded = 0;

      // Update handle cache
      textureHandle.generation = texture.generation();
      textures[texture.resourceId()] = textureHandle;
    }

    // Calculate bytes that still need uploading
    int32_t toUpload = textureHandle.size - textureHandle.uploaded;
    if (toUpload > 0) {
      size_t uploaded;
      
      // Set proper alignment
      int alignment = 8;
      while (texture.width() % alignment)
        alignment >>= 1;
      glPixelStorei(GL_PACK_ALIGNMENT, alignment);

      if (texture.dimensions() == 1) {
        /// @todo incremental upload
        glTexSubImage1D(GL_TEXTURE_1D, 0, 0, texture.width(), extFormat, texture.format().type(), texture.data() );
        uploaded = texture.width() * texture.format().bytesPerPixel();
      }
      else if (texture.dimensions() == 2) {
        // See how much of the bytes we can upload in this frame
        int64_t bytesFree = std::min<int64_t>(m_d->m_threadResources[threadIndex].uploadedBytes + toUpload, upload_bytes_limit) - m_d->m_threadResources[threadIndex].uploadedBytes;
        int64_t bytesPerScanline = texture.width() * texture.format().bytesPerPixel();
        // Number of scanlines to upload (minimum of 1 so we always have progress)
        const size_t scanLines = std::max<int32_t>(1, bytesFree / bytesPerScanline);
        // Start line (where we left of)
        const size_t startLine = textureHandle.uploaded / bytesPerScanline;

        // Upload data
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, startLine, texture.width(), scanLines, extFormat,texture.format().type(), texture.data() + textureHandle.uploaded);
        uploaded = scanLines * bytesPerScanline;
      }
      else if (texture.dimensions() == 3) {
        /// @todo incremental upload
        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, texture.width(), texture.height(), texture.depth(), extFormat, texture.format().type(), texture.data() );
        uploaded = texture.width() * texture.height() * texture.depth() * texture.format().bytesPerPixel();
      }

      // Update upload-limiter
      m_d->m_threadResources[threadIndex].uploadedBytes += uploaded;
      // Update handle
      textureHandle.uploaded += uploaded;
      textures[texture.resourceId()] = textureHandle;
    }
  }

  void RenderDriverGL::clearState(unsigned int threadIndex)
  {
    //m_d->m_threadResources[threadIndex].reset();
  }

/*
  void RenderDriverGL::setStencilFunc( StencilFunc func )
  {
  }

  void RenderDriverGL::setBlendFunction( BlendFunc func )
  {
  }
*/

  void RenderDriverGL::setRenderBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer)
  {
    // Color buffers
    GLboolean color = (colorBuffer ? GL_TRUE : GL_FALSE);
    glColorMask( color, color, color, color);

    // Depth buffer
    GLboolean depth = (depthBuffer ? GL_TRUE : GL_FALSE);
    glDepthMask(depth);

    // Stencil buffer
    /// @todo Should we only draw for front-facing?
    GLuint stencil = (stencilBuffer ? 0xff : 0x00);
    glStencilMaskSeparate(GL_FRONT_AND_BACK, stencil);
  }

  void RenderDriverGL::releaseResource(RenderResource::Id id)
  {
    /// @note This should only be called from the main thread
    for (unsigned int thread = 0; thread < m_d->m_threadResources.size(); ++thread)
      m_d->m_threadResources[thread].releaseQueue.push_back(id);
  }
}

#undef GLERROR
