#include "Luminous/RenderDriverGL.hpp"
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
  /// Resource handles
  struct BufferHandle
  {
    BufferHandle() : handle(0), generation(0), usage(BufferUsage_Static_Draw), size(0), uploaded(0) {}

    GLuint handle;
    uint64_t generation;
    BufferUsage usage;

    size_t size;      // Size (in bytes)
    size_t uploaded;  // Uploaded bytes (for incremental upload)
  };

  struct ProgramHandle
  {
    ProgramHandle() : handle(0), generation(0) {}

    GLuint handle;
    uint64_t generation;

    struct Variable {
      GLint location;
      GLenum type;
      GLint count;
    };
    typedef std::map<QString, Variable> NameIndexList;
    /// List of attributes and uniforms. These get queried once the shader is linked
    NameIndexList attributes;
    NameIndexList uniforms;

    // List of shaders linked to this program
    typedef std::vector<RenderResource::Id> ShaderList;
    ShaderList shaders;
  };

  struct TextureHandle
  {
    TextureHandle() : handle(0), generation(0), size(0), uploaded(0) {}
    GLuint handle;
    uint64_t generation;

    size_t size;      // Total size (in bytes)
    size_t uploaded;  // Bytes uploaded (for incremental uploading)
  };

  // Generic handle
  struct ResourceHandle
  {
    ResourceHandle() : handle(0), generation(0) {}
    GLuint handle;
    uint64_t generation;
  };
  typedef ResourceHandle ShaderHandle;
  typedef ResourceHandle BindingHandle;
  typedef ResourceHandle DescriptionHandle;


  /// 8 GB/sec upload limit is the bandwidth of PCIe 16x 2.0 (since 2007)
  static const int64_t upload_bytes_limit = 1024*1024*1024;

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
        : currentProgram(nullptr)
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

      const ProgramHandle * currentProgram;           // Currently bound shader program

      /// Resources
      ProgramList programs;
      ShaderList shaders;
      TextureList textures;
      BufferList buffers;
      BindingList bindings;
      DescriptionList descriptions;

      // Resources to be released
      std::vector<RenderResource::Id> releaseQueue;

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

    void bindBuffer(unsigned int threadIndex, GLenum bufferTarget, const HardwareBuffer & buffer)
    {
      auto & buffers = m_threadResources[threadIndex].buffers;
      auto it = buffers.find(buffer.resourceId());
      BufferHandle bufferHandle;
      if (it == std::end(buffers))
        bufferHandle.handle = GLUtils::createResource(ResourceType_Buffer);
      else
        bufferHandle = it->second;

      glBindBuffer(bufferTarget, bufferHandle.handle);
      GLERROR("RenderDriverGL::Bind HardwareBuffer");
      
      // Update if dirty
      if (bufferHandle.generation < buffer.generation()) {
        // Update or reallocate the resource
        /// @todo incremental upload
        if (buffer.size() != bufferHandle.size || buffer.usage() != bufferHandle.usage) {
          glBufferData(bufferTarget, buffer.size(), buffer.data(), GLUtils::getUsageFlags(buffer));
          GLERROR("RenderDriverGL::Bind HardwareBuffer reallocate");
        }
        else {
          glBufferSubData(bufferTarget, 0, buffer.size(), buffer.data());
          GLERROR("RenderDriverGL::Bind HardwareBuffer update");
        }

        // Update cache handle
        bufferHandle.generation = buffer.generation();
        bufferHandle.size = buffer.size();
        bufferHandle.uploaded = buffer.size();
        bufferHandle.usage = buffer.usage();
        buffers[buffer.resourceId()] = bufferHandle;
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

  void RenderDriverGL::preFrame(unsigned int threadIndex)
  {
    /// Reset statistics
    m_d->m_threadResources[threadIndex].uploadedBytes = 0;
    m_d->m_threadResources[threadIndex].frameTimer.start();

#define RELEASE_ME(container, id) { \
    auto iter = container.find(id); \
    if (iter != std::end(container)) \
      container.erase(iter); \
    }

    /// Clear out any released resources
    auto & r = m_d->m_threadResources[threadIndex];
    if (!r.releaseQueue.empty()) {
      for (auto it = std::begin(r.releaseQueue); it != std::end(r.releaseQueue); ++it) {
        RELEASE_ME(r.bindings, *it);
        RELEASE_ME(r.buffers, *it);
        RELEASE_ME(r.descriptions, *it);
        RELEASE_ME(r.programs, *it);
        RELEASE_ME(r.shaders, *it);
        RELEASE_ME(r.textures, *it);
      }
      r.releaseQueue.clear();
    }
#undef RELEASE_ME
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
  bool RenderDriverGL::setShaderConstant(unsigned int threadIndex, const QString & name, const TYPE & value) { \
    const auto & uniforms = m_d->m_threadResources[threadIndex].currentProgram->uniforms; \
    auto it = uniforms.find(name); \
    if (it != std::end(uniforms)) FUNCTION(it->second.location, value); \
    return (it != std::end(uniforms)); \
  }
#define SETUNIFORMVECTOR(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderConstant(unsigned int threadIndex, const QString & name, const TYPE & value) { \
    const auto & uniforms = m_d->m_threadResources[threadIndex].currentProgram->uniforms; \
    auto it = uniforms.find(name); \
    if (it != std::end(uniforms)) FUNCTION(it->second.location, 1, value.data()); \
    return (it != std::end(uniforms)); \
  }
#define SETUNIFORMMATRIX(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderConstant(unsigned int threadIndex, const QString & name, const TYPE & value) { \
    const auto & uniforms = m_d->m_threadResources[threadIndex].currentProgram->uniforms; \
    auto it = uniforms.find(name); \
    if (it != std::end(uniforms)) FUNCTION(it->second.location, 1, GL_TRUE, value.data()); \
    return (it != std::end(uniforms)); \
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
    ProgramHandle programHandle;
    if (it == std::end(programList)) {
      /// New program: create it
      programHandle.handle = GLUtils::createResource(program.resourceType());
      programHandle.generation = 0;
    }
    else
      programHandle = it->second;

    /// Tag if program needs relinking
    bool needsRelinking = (programHandle.generation < program.generation());

    // Check if any of our shaders are dirty
    auto & shaderList = m_d->m_threadResources[threadIndex].shaders;
    for (size_t i = 0; i < program.shaderCount(); ++i) {
      const std::shared_ptr<ShaderGLSL> & shader = program.shader(i);
      ShaderHandle shaderHandle;

      // Find the correct shader
      auto it = shaderList.find(shader->resourceId());
      if (it == std::end(shaderList)) {
        /// New shader: create it
        shaderHandle.handle = GLUtils::createResource(shader->resourceType());
        shaderHandle.generation = 0;
        shaderList[shader->resourceId()] = shaderHandle;
      }
      else
        shaderHandle = it->second;

      /// Check if it needs updating
      if (shaderHandle.generation < shader->generation()) {
        // Set and compile source
        const QByteArray shaderData = shader->text().toAscii();
        const GLchar * text = shaderData.data();
        const GLint length = shaderData.size();
        glShaderSource(shaderHandle.handle, 1, &text, &length);
        glCompileShader(shaderHandle.handle);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shaderHandle.handle, GL_COMPILE_STATUS, &compiled);
        if (compiled == GL_TRUE) {
          // All seems okay, update resource handle
          shaderHandle.generation = shader->generation();
          shaderList[shader->resourceId()] = shaderHandle;

          // Attach to the program
          glAttachShader(programHandle.handle, shaderHandle.handle);
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
      GLERROR("RenderDriverGL::Bind ShaderProgram link");
      // Check for linking errors
      GLint status;
      glGetProgramiv(programHandle.handle, GL_LINK_STATUS, &status);

      /// Query for available attributes
      {
        GLint attrCount;
        GLchar attrName[128];
        glGetProgramiv(programHandle.handle, GL_ACTIVE_ATTRIBUTES, &attrCount);
        // Get attribute locations
        programHandle.attributes.clear();
        ProgramHandle::Variable attr;
        for (int idx = 0; idx < attrCount; ++idx) {
          glGetActiveAttrib(programHandle.handle, idx, 128, NULL, &attr.count, &attr.type, attrName);
          // @note apparently the location isn't necessarily the same as the index, so we have to get both here
          attr.location = glGetAttribLocation(programHandle.handle, attrName);
          programHandle.attributes[attrName] = attr;
        }
      }

      /// Query for available uniforms
      {
        GLint uniCount;
        GLchar uniName[128];
        glGetProgramiv(programHandle.handle, GL_ACTIVE_UNIFORMS, &uniCount);
        programHandle.uniforms.clear();
        ProgramHandle::Variable uniform;
        for (int idx = 0; idx < uniCount; ++idx) {
          glGetActiveUniform(programHandle.handle, idx, 128, NULL, &uniform.count, &uniform.type, uniName);
          // @note apparently the location isn't necessarily the same as the index, so we have to get both here
          uniform.location = glGetUniformLocation(programHandle.handle, uniName);
          programHandle.uniforms[uniName] = uniform;
        }
      }

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

    m_d->m_threadResources[threadIndex].currentProgram = &programList[program.resourceId()];
    glUseProgram(programHandle.handle);
    GLERROR("RenderDriverGL::Bind Shaderprogram use");
  }

  void RenderDriverGL::setVertexBuffer(unsigned int threadIndex, const HardwareBuffer & buffer)
  {
    m_d->bindBuffer(threadIndex, GL_ARRAY_BUFFER, buffer);
  }

  void RenderDriverGL::setIndexBuffer(unsigned int threadIndex, const HardwareBuffer & buffer)
  {
    m_d->bindBuffer(threadIndex, GL_ELEMENT_ARRAY_BUFFER, buffer);
  }

  void RenderDriverGL::setConstantBuffer(unsigned int threadIndex, const HardwareBuffer & buffer)
  {
    m_d->bindBuffer(threadIndex, GL_UNIFORM_BUFFER_EXT, buffer);
  }

  void RenderDriverGL::setVertexDescription(unsigned int threadIndex, const VertexDescription & description)
  {
    // Set buffer attributes from its bound VertexDescription
    for (size_t attrIndex = 0; attrIndex < description.attributeCount(); ++attrIndex) {
      VertexAttribute attr = description.attribute(attrIndex);

      auto & attributes = m_d->m_threadResources[threadIndex].currentProgram->attributes;
      auto location = attributes.find(attr.name);
      if (location == std::end(attributes)) {
        Radiant::warning("Unable to bind vertex attribute %s", attr.name.toAscii().data());
      }
      else {
        GLenum normalized = (attr.normalized ? GL_TRUE : GL_FALSE);
        GLenum type = GLUtils::getDataType(attr.type);

        /// @todo Warn if there's a type/count mismatch between the cached versions (in the ShaderHandle) and what we're trying to do
        glVertexAttribPointer(location->second.location, attr.count, type, normalized, description.vertexSize(), reinterpret_cast<GLvoid *>(attr.offset));
        glEnableVertexAttribArray(location->second.location);
        GLERROR("RenderDriverGL::Bind VertexAttributeBinding vertexAttribPointer");
      }
    }
  }

  void RenderDriverGL::setVertexBinding(unsigned int threadIndex, const VertexAttributeBinding & binding)
  {
    auto & bindings = m_d->m_threadResources[threadIndex].bindings;
    auto it = bindings.find(binding.resourceId());
    BindingHandle bindingHandle;
    if (it == std::end(bindings))
      bindingHandle.handle = GLUtils::createResource(ResourceType_VertexArray);
    else
      bindingHandle = it->second;

    /// @todo Recreate the VAO if the vertex description has changed
    if (bindingHandle.generation < binding.generation()) {
      // Recreate the binding
      GLUtils::destroyResource(ResourceType_VertexArray, bindingHandle.handle);
      bindingHandle.handle = GLUtils::createResource(ResourceType_VertexArray);
      // Bind
      glBindVertexArray(bindingHandle.handle);
      GLERROR("RenderDriverGL::Bind VertexAttributeBinding recreate");

      // Bind all vertex buffers
      for (size_t i = 0; i < binding.bindingCount(); ++i) {
        VertexAttributeBinding::Binding b = binding.binding(i);
        // Attach buffer
        setVertexBuffer(threadIndex, *b.buffer);
        setVertexDescription(threadIndex, *b.description);
      }
      // Update cache
      bindingHandle.generation = binding.generation();
      bindings[binding.resourceId()] = bindingHandle;
    }
    else {
      /// @todo Avoid double binding, since it can be pretty expensive
      glBindVertexArray(bindingHandle.handle);
      GLERROR("RenderDriverGL::Bind VertexAttributeBinding bind");

      // Make sure the buffers get updated if necessary
      for (size_t i = 0; i < binding.bindingCount(); ++i) {
        VertexAttributeBinding::Binding b = binding.binding(i);
        setVertexBuffer(threadIndex, *b.buffer);
      }
    }
  }

  void RenderDriverGL::setTexture(unsigned int threadIndex, unsigned int textureUnit, const Texture2 & texture)
  {
    auto & textures = m_d->m_threadResources[threadIndex].textures;
    auto it = textures.find(texture.resourceId());
    TextureHandle textureHandle;
    if (it == std::end(textures) )
      textureHandle.handle = GLUtils::createResource(ResourceType_Texture);
    else
      textureHandle = it->second;

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

    if (textureHandle.generation < texture.generation()) {
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

  void RenderDriverGL::releaseResource(RenderResource::Id id)
  {
    /// @note This should only be called from the main thread
    for (unsigned int thread = 0; thread < m_d->m_threadResources.size(); ++thread)
      m_d->m_threadResources[thread].releaseQueue.push_back(id);
  }
}

#undef GLERROR
