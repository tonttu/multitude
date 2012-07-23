#include "Luminous/RenderDriverGL.hpp"
#include "Luminous/RenderManager.hpp"
#include "Luminous/VertexAttributeBinding.hpp"
#include "Luminous/VertexDescription.hpp"
#include "Luminous/HardwareBuffer.hpp"
#include "Luminous/ShaderProgram.hpp"
#include "Luminous/ShaderUniform.hpp"
#include "Luminous/Texture2.hpp"
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
#include <algorithm>

#include <QStringList>

#if RADIANT_DEBUG
# define GLERROR(txt) Utils::glCheck(txt)
#else
# define GLERROR(txt)
#endif

// Since we got rid of GLEW on OSX...
#ifdef RADIANT_OSX
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#define glGenVertexArrays glGenVertexArraysAPPLE
#endif

// Utility functions
namespace
{
  GLuint createResource(Luminous::RenderResource::Type type)
  {
    GLuint resource;
    switch (type)
    {
    case Luminous::RenderResource::VertexArray:    glGenVertexArrays(1, &resource); return resource;
    case Luminous::RenderResource::Buffer:         glGenBuffers(1, &resource); return resource;
    case Luminous::RenderResource::ShaderProgram:  return glCreateProgram();
    case Luminous::RenderResource::VertexShader:   return glCreateShader(GL_VERTEX_SHADER);
    case Luminous::RenderResource::FragmentShader: return glCreateShader(GL_FRAGMENT_SHADER);
    case Luminous::RenderResource::GeometryShader: return glCreateShader(GL_GEOMETRY_SHADER_EXT);
    case Luminous::RenderResource::Texture:        glGenTextures(1, & resource); return resource;
    default:
      Radiant::error("RenderDriverGL: Can't create GL resource: unknown type %d", type);
      assert(false);
      return 0;
    }
  }

  void destroyResource(Luminous::RenderResource::Type type, GLuint resource)
  {
    switch (type)
    {
    case Luminous::RenderResource::VertexArray:    glDeleteVertexArrays(1, &resource); break;
    case Luminous::RenderResource::Buffer:         glDeleteBuffers(1, &resource); break;
    case Luminous::RenderResource::ShaderProgram:  glDeleteProgram(resource); break;
    case Luminous::RenderResource::VertexShader:   glDeleteShader(resource); break;
    case Luminous::RenderResource::FragmentShader: glDeleteShader(resource); break;
    case Luminous::RenderResource::GeometryShader: glDeleteShader(resource); break;
    case Luminous::RenderResource::Texture:        glDeleteTextures(1, & resource); break;
    default:
      Radiant::error("RenderDriverGL: Can't destroy GL resource: unknown type %d", type);
      assert(false);
    }
  }
}

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
    RenderResource::Type type;
    const T * resource;
    Radiant::Timer lastUsed;
    GLuint handle;
    uint64_t generation;

    inline bool operator==(const ResourceHandle<T> & o) const { return handle == o.handle; }
    inline bool operator!=(const ResourceHandle<T> & o) const { return handle != o.handle; }
    inline bool operator<(const ResourceHandle<T> & o) const { return handle < o.handle; }
  };

  struct BufferHandle : public ResourceHandle<HardwareBuffer>
  {
    BufferHandle(const HardwareBuffer * ptr = nullptr) : ResourceHandle(ptr), usage(HardwareBuffer::StaticDraw), size(0), uploaded(0) {}
    
    HardwareBuffer::Usage usage;
    size_t size;        // Size (in bytes)
    size_t uploaded;    // Uploaded bytes (for incremental upload)
  };
  
  struct TextureHandle : public ResourceHandle<Texture>
  {
    TextureHandle(const Texture * ptr = nullptr) : ResourceHandle(ptr), target(0), size(0), uploaded(0) {}
    GLenum target;
    size_t size;      // Total size (in bytes)
    size_t uploaded;  // Bytes uploaded (for incremental uploading)
  };

  struct ProgramHandle : public ResourceHandle<ShaderProgram>
  {
    ProgramHandle(const ShaderProgram * ptr = nullptr) : ResourceHandle(ptr) {}
    std::map<QByteArray, GLuint> textures;
    std::set<GLuint> shaders;

    UniformDescription baseDescription;
  };

  struct BufferMapping
  {
    BufferMapping() : target(0), access(0), offset(0), length(0), data(0) {}
    GLenum target;
    GLenum access;
    int offset;
    std::size_t length;
    void * data;
  };

  // Generic handle
  typedef ResourceHandle<ShaderGLSL> ShaderHandle;
  typedef ResourceHandle<VertexAttributeBinding> BindingHandle;


  struct RenderState
  {
    Luminous::ProgramHandle * program;
    BindingHandle * binding;
    BufferHandle * uniformBuffer;
    BufferHandle * index;
    std::array<TextureHandle*, 8> textures;
    bool operator<(const RenderState & o) const
    {
      if(program != o.program)
        return program < o.program;
      if(binding != o.binding)
        return binding < o.binding;
      if(uniformBuffer != o.uniformBuffer)
        return uniformBuffer < o.uniformBuffer;
      if(index != o.index)
        return index < o.index;
      for(std::size_t i = 0; i < textures.size(); ++i)
        if((!textures[i] || !o.textures[i]) || (textures[i] != o.textures[i]))
          return textures[i] < o.textures[i];

      return false;
    }
  };

  //////////////////////////////////////////////////////////////////////////
  // RenderDriver implementation
  class RenderDriverGL::D
  {
  public:
    D()
      : m_currentProgram(0)
      , m_currentBinding(0)
      , m_currentBuffer(0)
      , m_uploadedBytes(0)
      , m_frame(0)
      , m_fps(0.0)
    {}

    typedef std::vector<GLuint> AttributeList;
    AttributeList m_activeAttributes;

    GLuint m_currentProgram;  // Currently bound shader program
    GLuint m_currentBinding;  // Currently bound vertex binding
    GLuint m_currentBuffer;   // Currently bound buffer object

    std::map<GLuint, BufferMapping> m_bufferMaps;

    typedef std::map<RenderResource::Hash, ProgramHandle> ProgramList;
    typedef std::map<RenderResource::Hash, ShaderHandle> ShaderList;
    typedef std::map<RenderResource::Hash, TextureHandle> TextureList;
    typedef std::map<RenderResource::Id, BufferHandle> BufferList;
    typedef std::map<RenderResource::Id, BindingHandle> BindingList;

    /// Resources
    ProgramList m_programs;
    ShaderList m_shaders;
    TextureList m_textures;
    BufferList m_buffers;
    BindingList m_bindings;

    RenderState m_state;

    std::map<RenderState, std::pair<int, std::vector<Luminous::RenderCommand>>> m_opaqueQueue;
    std::vector<std::pair<RenderState, Luminous::RenderCommand>> m_transparentQueue;

    // Resources to be released
    typedef std::vector<RenderResource::Id> ReleaseQueue;
    ReleaseQueue m_releaseQueue;

    /// Render statistics
    int32_t m_uploadedBytes;      // Uploaded bytes this frame
    int32_t m_totalBytes;         // Total bytes currently in GPU memory for this thread
    Radiant::Timer m_frameTimer;  // Time since begin of frame
    uint64_t m_frame;             // Current frame number
    double m_fps;                 // Frames per second

  public:

    /// Reset thread statistics
    void resetStatistics()
    {
      m_uploadedBytes = 0;
      m_frameTimer.start();
    }

    /// Update render statistics
    void updateStatistics()
    {
      const double frameTime = m_frameTimer.time();

      m_frame++;
      m_fps = 1.0 / frameTime;
    }

    /// Cleanup any queued-for-deletion or expired resources
    void removeResources()
    {
      removeResource(m_bindings, m_releaseQueue);
      removeResource(m_buffers, m_releaseQueue);
      removeResource(m_programs);
      removeResource(m_shaders);
      removeResource(m_textures);
      m_releaseQueue.clear();
    }

    void bindShaderProgram(const ProgramHandle & programHandle)
    {
      // Avoid re-applying the same shader
      if (m_currentProgram != programHandle.handle)
      {
        m_currentProgram = programHandle.handle;
        glUseProgram(programHandle.handle);
        GLERROR("RenderDriverGL::setShaderProgram glUseProgram");
      }
    }

    void bindTexture(const TextureHandle & textureHandle, int textureUnit)
    {
      glActiveTexture(GL_TEXTURE0 + textureUnit);
      glBindTexture(textureHandle.target, textureHandle.handle);
      GLERROR("RenderDriverGL::D::bindTexture glBindTexture");
    }

    bool updateShaders(const ShaderProgram & program, ProgramHandle & programHandle)
    {
      bool needsRelinking = false;

      for (size_t i = 0; i < program.shaderCount(); ++i)
      {
        RenderResource::Id shaderId = program.shader(i);
        ShaderGLSL * shader = RenderManager::getResource<ShaderGLSL>(shaderId);
        const RenderResource::Hash shaderHash = shader->hash();

        ShaderHandle * handle;

        // Find the correct shader
        auto it = m_shaders.find(shaderHash);
        if (it == std::end(m_shaders)) {
          /// New shader: create it
          ShaderHandle shaderHandle(shader);
          shaderHandle.handle = createResource(shader->resourceType());
          shaderHandle.generation = 0;
          m_shaders[shaderHash] = shaderHandle;
          handle = &m_shaders[shaderHash];

          // Set and compile source
          const QByteArray shaderData = shader->text().toAscii();
          const GLchar * text = shaderData.data();
          const GLint length = shaderData.size();
          glShaderSource(handle->handle, 1, &text, &length);
          glCompileShader(handle->handle);
          GLERROR("RenderDriverGL::setShaderProgram glCompileShader");
          GLint compiled = GL_FALSE;
          glGetShaderiv(handle->handle, GL_COMPILE_STATUS, &compiled);
          if (compiled != GL_TRUE) {
            Radiant::error("Failed to compile shader %s", shader->filename().toUtf8().data());

            // Dump info log
            GLsizei len;
            glGetShaderiv(handle->handle, GL_INFO_LOG_LENGTH, &len);
            std::vector<GLchar> log(len);
            glGetShaderInfoLog(handle->handle, len, &len, log.data());
            Radiant::error("%s", log.data());

            continue;
          }
        }
        else
          handle = &(it->second);

        // Reset usage timer
        handle->lastUsed.start();

        if(programHandle.shaders.find(handle->handle) == programHandle.shaders.end()) {
          programHandle.shaders.insert(handle->handle);
          // Attach to the program
          glAttachShader(programHandle.handle, handle->handle);
          GLERROR("RenderDriverGL::setShaderProgram glAttachShader");
          needsRelinking = true;
        }
      }
      return needsRelinking;
    }

    void relinkShaderProgram(const ShaderProgram & program, ProgramHandle & programHandle)
    {
      glLinkProgram(programHandle.handle);
      GLERROR("RenderDriverGL::setShaderProgram glLinkProgram");
      // Check for linking errors
      GLint status;
      glGetProgramiv(programHandle.handle, GL_LINK_STATUS, &status);

      if (status == GL_FALSE) {
        Radiant::error("Failed to link shader program (shaders %s)", program.shaderFilenames().join(", ").toUtf8().data());
        GLsizei len;
        glGetProgramiv(programHandle.handle, GL_INFO_LOG_LENGTH, &len);
        std::vector<GLchar> log(len);
        glGetProgramInfoLog(programHandle.handle, len, &len, log.data());
        Radiant::error("%s", log.data());
        // Can't link: abort binding
        return;
      }

      // Update handle generation
      programHandle.generation = program.generation();
    }

    void applyShaderUniforms(const ShaderProgram & program)
    {
      auto & programHandle = m_programs[program.hash()];

      // Set all shader uniforms attached to this shader
      for (size_t i = 0; i < program.uniformCount(); ++i) {
        ShaderUniform & uniform = program.uniform(i);

        // Update location cache if necessary
        if (uniform.index == -1)
          uniform.index = glGetUniformLocation(programHandle.handle, uniform.name.toAscii().data());

        // Set the uniform
        switch (uniform.type())
        {
        case ShaderUniform::Int: glUniform1iv(uniform.index, 1, (const int*)uniform.data()); break;
        case ShaderUniform::Int2: glUniform2iv(uniform.index, 1, (const int*)uniform.data()); break;
        case ShaderUniform::Int3: glUniform3iv(uniform.index, 1, (const int*)uniform.data()); break;
        case ShaderUniform::Int4: glUniform4iv(uniform.index, 1, (const int*)uniform.data()); break;
        case ShaderUniform::UnsignedInt: glUniform1uiv(uniform.index, 1, (const unsigned int*)uniform.data()); break;
        case ShaderUniform::UnsignedInt2: glUniform2uiv(uniform.index, 1, (const unsigned int*)uniform.data()); break;
        case ShaderUniform::UnsignedInt3: glUniform3uiv(uniform.index, 1, (const unsigned int*)uniform.data()); break;
        case ShaderUniform::UnsignedInt4: glUniform4uiv(uniform.index, 1, (const unsigned int*)uniform.data()); break;
        case ShaderUniform::Float: glUniform1fv(uniform.index, 1, (const float*)uniform.data()); break;
        case ShaderUniform::Float2: glUniform2fv(uniform.index, 1, (const float*)uniform.data()); break;
        case ShaderUniform::Float3: glUniform3fv(uniform.index, 1, (const float*)uniform.data()); break;
        case ShaderUniform::Float4: glUniform4fv(uniform.index, 1, (const float*)uniform.data()); break;
        case ShaderUniform::Float2x2: glUniformMatrix2fv(uniform.index, 1, GL_TRUE, (const float*)uniform.data()); break;
        case ShaderUniform::Float3x3: glUniformMatrix3fv(uniform.index, 1, GL_TRUE, (const float*)uniform.data()); break;
        case ShaderUniform::Float4x4: glUniformMatrix4fv(uniform.index, 1, GL_TRUE, (const float*)uniform.data()); break;
        default:
          Radiant::error("RenderDriverGL: Unknown shader uniform type %d", uniform.type());
          assert(false);
        }
      }
    }

    UniformDescription uniformDescription(const ProgramHandle & programHandle, const QByteArray & blockName)
    {
      /// @todo error checking/handling

      UniformDescription desc;

      GLuint blockIndex = glGetUniformBlockIndex(programHandle.handle, blockName.data());
      if(blockIndex < 0)
        return desc;

      int uniformCount = 0;
      glGetProgramiv(programHandle.handle, GL_ACTIVE_UNIFORMS, &uniformCount);

      for(GLuint uniformIndex = 0; uniformIndex < uniformCount; ++uniformIndex) {
        GLint nameLength = 0, size, blockIndex = 0, offset = 0;
        glGetActiveUniformsiv(programHandle.handle, 1, &uniformIndex, GL_UNIFORM_NAME_LENGTH, &nameLength);
        glGetActiveUniformsiv(programHandle.handle, 1, &uniformIndex, GL_UNIFORM_SIZE, &size);
        glGetActiveUniformsiv(programHandle.handle, 1, &uniformIndex, GL_UNIFORM_BLOCK_INDEX, &blockIndex);
        glGetActiveUniformsiv(programHandle.handle, 1, &uniformIndex, GL_UNIFORM_OFFSET, &offset);

        if(blockIndex == GLint(blockIndex) && nameLength > 0 && offset >= 0) {
          QByteArray ba(nameLength, '0');
          GLsizei len = 0;
          glGetActiveUniformName(programHandle.handle, uniformIndex, ba.size(), &len, ba.data());
          ba.resize(len);
          desc.addAttribute(ba, offset, size);
        }
      }

      return desc;
    }

    ProgramHandle & getLinkedShaderProgram(const ShaderProgram & program)
    {
      ProgramHandle & programHandle = m_programs[program.hash()];
      if(programHandle.handle == 0) {
        /// New program: create it and add to cache
        programHandle.resource = &program;
        programHandle.handle = createResource(program.resourceType());
        programHandle.generation = 0;

        updateShaders(program, programHandle);
        relinkShaderProgram(program, programHandle);
        programHandle.baseDescription = uniformDescription(programHandle, "BaseBlock");
      }

      // Reset usage timer
      programHandle.lastUsed.start();

      return programHandle;
    }

    TextureHandle & textureHandle(const Texture & texture, int textureUnit, bool alwaysBind = false)
    {
      TextureHandle & textureHandle = m_textures[texture.hash()];
      bool newTexture = false;
      if(textureHandle.handle == 0) {
        textureHandle.resource = &texture;
        textureHandle.handle = createResource(RenderResource::Texture);
        newTexture = true;
      }

      // Reset usage timer
      textureHandle.lastUsed.start();

      /// Specify the external format (number of channels)
      GLenum extFormat;
      if (texture.format().numChannels() == 1)      extFormat = GL_RED;
      else if (texture.format().numChannels() == 2) extFormat = GL_RG;
      else if (texture.format().numChannels() == 3) extFormat = GL_RGB;
      else if (texture.format().numChannels() == 4) extFormat = GL_RGBA;

      bool bound = false;

      if((/*texture.mode() == Texture::Streaming &&*/ textureHandle.generation < texture.generation())
         || newTexture) {
        // Start the uploading
        textureHandle.uploaded = 0;

        if (texture.dimensions() == 1)      textureHandle.target = GL_TEXTURE_1D;
        else if (texture.dimensions() == 2) textureHandle.target = GL_TEXTURE_2D;
        else if (texture.dimensions() == 3) textureHandle.target = GL_TEXTURE_3D;

        bindTexture(textureHandle, textureUnit);
        bound = true;

        if (texture.dimensions() == 1)      glTexImage1D(GL_TEXTURE_1D, 0, texture.format().layout(), texture.width(), 0, extFormat , texture.format().type(), nullptr );
        else if (texture.dimensions() == 2) glTexImage2D(GL_TEXTURE_2D, 0, texture.format().layout(), texture.width(), texture.height(), 0, extFormat, texture.format().type(), nullptr );
        else if (texture.dimensions() == 3) glTexImage3D(GL_TEXTURE_3D, 0, texture.format().layout(), texture.width(), texture.height(), texture.depth(), 0, extFormat, texture.format().type(), nullptr );

        /// @todo Get these from the texture settings
        glTexParameteri(textureHandle.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(textureHandle.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        textureHandle.size = texture.width() * texture.height() * texture.depth() * texture.format().bytesPerPixel();
        textureHandle.uploaded = 0;

        // Update handle cache
        textureHandle.generation = texture.generation();
      }

      if(!bound && alwaysBind) {
        bindTexture(textureHandle, textureUnit);
        bound = true;
      }

      // Calculate bytes that still need uploading
      int32_t toUpload = textureHandle.size - textureHandle.uploaded;
      if (toUpload > 0) {
        size_t uploaded;

        if(!bound)
          bindTexture(textureHandle, textureUnit);

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
          int64_t bytesFree = std::min<int64_t>(m_uploadedBytes + toUpload, upload_bytes_limit) - m_uploadedBytes;
          int64_t bytesPerScanline = texture.width() * texture.format().bytesPerPixel();
          // Number of scanlines to upload (minimum of 1 so we always have progress)
          const size_t scanLines = std::max<int32_t>(1, bytesFree / bytesPerScanline);
          // Start line (where we left of)
          const size_t startLine = textureHandle.uploaded / bytesPerScanline;

          // Upload data
          glTexSubImage2D(GL_TEXTURE_2D, 0, 0, startLine, texture.width(), scanLines, extFormat, texture.format().type(), texture.data() + textureHandle.uploaded);
          uploaded = scanLines * bytesPerScanline;
        }
        else if (texture.dimensions() == 3) {
          /// @todo incremental upload
          glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, texture.width(), texture.height(), texture.depth(), extFormat, texture.format().type(), texture.data() );
          uploaded = texture.width() * texture.height() * texture.depth() * texture.format().bytesPerPixel();
        }

        // Update upload-limiter
        m_uploadedBytes += uploaded;
        // Update handle
        textureHandle.uploaded += uploaded;
      }

      return textureHandle;
    }


    void setVertexAttributes(const VertexAttributeBinding & binding)
    {
      // Bind all vertex buffers
      for (size_t i = 0; i < binding.bindingCount(); ++i) {
        VertexAttributeBinding::Binding b = binding.binding(i);
        // Attach buffer
        auto * buffer = RenderManager::getResource<HardwareBuffer>(b.buffer);
        assert(buffer != nullptr);
        bindBuffer(GL_ARRAY_BUFFER, *buffer);
        setVertexDescription(b.description);
      }
    }

    void setVertexDescription(const VertexDescription & description)
    {
      // Set buffer attributes from its bound VertexDescription
      for (size_t attrIndex = 0; attrIndex < description.attributeCount(); ++attrIndex) {
        VertexAttribute attr = description.attribute(attrIndex);
        GLint location = glGetAttribLocation(m_currentProgram, attr.name.toAscii().data());
        if (location == -1) {
          Radiant::warning("Unable to bind vertex attribute %s", attr.name.toAscii().data());
        }
        else {
          GLenum normalized = (attr.normalized ? GL_TRUE : GL_FALSE);

          glVertexAttribPointer(location, attr.count, attr.type, normalized, description.vertexSize(), reinterpret_cast<GLvoid *>(attr.offset));
          GLERROR("RenderDriverGL::Bind VertexAttributeBinding glVertexAttribPointer");
          glEnableVertexAttribArray(location);
          GLERROR("RenderDriverGL::Bind VertexAttributeBinding glEnableVertexAttribArray");
        }
      }
    }

    BindingHandle & getBindingHandle(const VertexAttributeBinding & binding, BufferHandle ** indexHandle,
                                     const ProgramHandle * programHandle)
    {
      GLERROR("RenderDriverGL::getBindingHandle");
      BindingHandle & bindingHandle = m_bindings[binding.resourceId()];

      bool update = false;

      if(bindingHandle.handle == 0) {
        // New resource
        bindingHandle.resource = &binding;
        update = true;
      }
      else {
        // Reset usage timer
        bindingHandle.lastUsed.start();

        // Check if we need to recreate
        if(bindingHandle.generation < binding.generation()) {
          destroyResource(RenderResource::VertexArray, bindingHandle.handle);
          update = true;
        }
      }

      if(update) {
        bindingHandle.handle = createResource(RenderResource::VertexArray);
        bindingHandle.generation = binding.generation();

        // Bind and setup all buffers/attributes
        glBindVertexArray(bindingHandle.handle);
        if(programHandle) bindShaderProgram(*programHandle);
        setVertexAttributes(binding);

        const HardwareBuffer * index = RenderManager::getResource<HardwareBuffer>(binding.indexBuffer());
        if (index != nullptr) {
          BufferHandle & iHandle = getBufferHandle(GL_ELEMENT_ARRAY_BUFFER, *index);
          bindBuffer(GL_ELEMENT_ARRAY_BUFFER, iHandle.handle);
          if(indexHandle)
            *indexHandle = &iHandle;
        }
      } else if(indexHandle) {
        const HardwareBuffer * index = RenderManager::getResource<HardwareBuffer>(binding.indexBuffer());
        if(index)
          *indexHandle = &getBufferHandle(GL_ELEMENT_ARRAY_BUFFER, *index);
      }

      return bindingHandle;
    }

    void setVertexBinding(const BindingHandle & binding, const BufferHandle & indexBuffer)
    {
      // Bind
      if (m_currentBinding != binding.handle) {
        glBindVertexArray(binding.handle);
        m_currentBinding = binding.handle;
      }
      bindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer.handle);
    }

    inline void bindBuffer(GLenum bufferTarget, GLuint buffer)
    {
      /// @todo doesn't take bufferTarget into account
      /*if(buffer == m_currentBuffer)
        return;
      m_currentBuffer = buffer;*/
      glBindBuffer(bufferTarget, buffer);
      GLERROR("RenderDriverGL::bindBuffer glBindBuffer");
    }

    BufferHandle & getBufferHandle(GLenum bufferTarget, const HardwareBuffer & buffer)
    {
      BufferHandle & bufferHandle = m_buffers[buffer.resourceId()];
      if(bufferHandle.handle == 0) {
        bufferHandle.handle = createResource(RenderResource::Buffer);
        bufferHandle.resource = &buffer;
      }

      // Reset usage timer
      bufferHandle.lastUsed.start();

      // Update if dirty
      if (bufferHandle.generation < buffer.generation()) {
        bindBuffer(bufferTarget, bufferHandle.handle);

        // Update or reallocate the resource
        /// @todo incremental upload
        if (buffer.size() != bufferHandle.size || buffer.usage() != bufferHandle.usage) {
          glBufferData(bufferTarget, buffer.size(), buffer.data(), buffer.usage());
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

      return bufferHandle;
    }

    BufferHandle & bindBuffer(GLenum bufferTarget, const HardwareBuffer & buffer)
    {
      BufferHandle & bufferHandle = getBufferHandle(bufferTarget, buffer);
      bindBuffer(bufferTarget, bufferHandle.handle);
      return bufferHandle;
    }

    // Utility function for resource cleanup
    template <typename ContainerType>
    void removeResource(ContainerType & container, const ReleaseQueue & releaseQueue)
    {
      auto it = std::begin(container);
      while (it != std::end(container)) {
        if (std::find( std::begin(releaseQueue), std::end(releaseQueue), it->first) != std::end(releaseQueue) ||  // First, check if resource has been deleted
          (it->second.resource->expiration() > 0 && it->second.lastUsed.time() > it->second.resource->expiration()))      // If not, we can check if it has expired
        {
          // Release the GL resource
          destroyResource(it->second.type, it->second.handle);
          // Remove from container
          it = container.erase(it);
        }
        else
          it++;
      }
    }

    // Only cleans up expired resources
    template <typename ContainerType>
    void removeResource(ContainerType & container)
    {
      auto it = std::begin(container);
      while(it != std::end(container)) {
        const auto & handle = it->second;
        if(handle.resource->expiration() > 0 && handle.lastUsed.time() > handle.resource->expiration()) {
          // Release the GL resource
          destroyResource(handle.type, handle.handle);
          // Remove from container
          it = container.erase(it);
        } else ++it;
      }
    }
  };


  //////////////////////////////////////////////////////////////////////////
  //
  RenderDriverGL::RenderDriverGL()
    : m_d(new RenderDriverGL::D())
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
    glDrawArrays(type, (GLint) offset, (GLsizei) primitives);
    GLERROR("RenderDriverGL::draw glDrawArrays");
  }

  void RenderDriverGL::drawIndexed(PrimitiveType type, unsigned int offset, unsigned int primitives)
  {
    /// @todo allow other index types (unsigned byte, unsigned short and unsigned int)
    glDrawElements(type, (GLsizei) primitives, GL_UNSIGNED_INT, (const GLvoid *)((sizeof(uint) * offset)));
    GLERROR("RenderDriverGL::draw glDrawElements");
  }

#define SETUNIFORM(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderUniform(const char * name, const TYPE & value) { \
    /* @todo These locations should be cached in the program handle for performance reasons */ \
    GLint location = glGetUniformLocation(m_d->m_currentProgram, name); \
    if (location != -1) FUNCTION(location, value); \
    return (location != -1); \
  }
#define SETUNIFORMVECTOR(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderUniform(const char * name, const TYPE & value) { \
    /* @todo These locations should be cached in the program handle for performance reasons */ \
    GLint location = glGetUniformLocation(m_d->m_currentProgram, name); \
    if (location != -1) FUNCTION(location, 1, value.data()); \
    return (location != -1); \
  }
#define SETUNIFORMMATRIX(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderUniform(const char * name, const TYPE & value) { \
    /* @todo These locations should be cached in the program handle for performance reasons */ \
    GLint location = glGetUniformLocation(m_d->m_currentProgram, name); \
    if (location != -1) FUNCTION(location, 1, GL_TRUE, value.data()); \
    return (location != -1); \
  }

  SETUNIFORM(int, glUniform1i);
  SETUNIFORM(unsigned int, glUniform1ui);
  SETUNIFORM(float, glUniform1f);
  SETUNIFORMVECTOR(Nimble::Vector2i, glUniform2iv);
  SETUNIFORMVECTOR(Nimble::Vector3i, glUniform3iv);
  SETUNIFORMVECTOR(Nimble::Vector4i, glUniform4iv);
  SETUNIFORMVECTOR(Nimble::Vector2T<unsigned int>, glUniform2uiv);
  SETUNIFORMVECTOR(Nimble::Vector3T<unsigned int>, glUniform3uiv);
  SETUNIFORMVECTOR(Nimble::Vector4T<unsigned int>, glUniform4uiv);
  SETUNIFORMVECTOR(Nimble::Vector2f, glUniform2fv);
  SETUNIFORMVECTOR(Nimble::Vector3f, glUniform3fv);
  SETUNIFORMVECTOR(Nimble::Vector4f, glUniform4fv);
  SETUNIFORMMATRIX(Nimble::Matrix2f, glUniformMatrix2fv);
  SETUNIFORMMATRIX(Nimble::Matrix3f, glUniformMatrix3fv);
  SETUNIFORMMATRIX(Nimble::Matrix4f, glUniformMatrix4fv);
#undef SETUNIFORM
#undef SETUNIFORMVECTOR
#undef SETUNIFORMMATRIX

  void RenderDriverGL::setShaderProgram(const ShaderProgram & program)
  {
    auto & handle = m_d->getLinkedShaderProgram(program);
    m_d->bindShaderProgram(handle);
    m_d->applyShaderUniforms(program);
  }


  void RenderDriverGL::preFrame()
  {
    m_d->resetStatistics();
    m_d->removeResources();

    /// @todo Currently the RenderContext invalidates this cache every frame, even if it's not needed
    m_d->m_currentProgram = 0;
    m_d->m_currentBinding = 0;
  }

  void RenderDriverGL::postFrame()
  {
    m_d->updateStatistics();
  }

  void RenderDriverGL::setVertexBuffer(const HardwareBuffer & buffer)
  {
    m_d->bindBuffer(GL_ARRAY_BUFFER, buffer);
  }

  void RenderDriverGL::setIndexBuffer(const HardwareBuffer & buffer)
  {
    m_d->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
  }

  void RenderDriverGL::setUniformBuffer(const HardwareBuffer & buffer)
  {
    m_d->bindBuffer(GL_UNIFORM_BUFFER_EXT, buffer);
  }

  void RenderDriverGL::setVertexBinding(const VertexAttributeBinding & binding)
  {
#ifdef LUMINOUS_OPENGLES
    // OpenGL ES doesn't have VAOs, so we'll just have to bind the buffers and attributes every time
    m_d->setVertexAttributes(binding);
    auto * indexBuffer = RenderManager::getResource<HardwareBuffer>(binding.indexBuffer());
    if (indexBuffer)
      setIndexBuffer(*indexBuffer);
#else
    BufferHandle * indexHandle;
    BindingHandle & bindingHandle = m_d->getBindingHandle(binding, &indexHandle, nullptr);
    m_d->setVertexBinding(bindingHandle, *indexHandle);
#endif
  }

  void RenderDriverGL::setTexture(unsigned int textureUnit, const Texture & texture)
  {
    m_d->textureHandle(texture, textureUnit, true);
  }

  void RenderDriverGL::clearState()
  {
    //m_d->reset();
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

  void * RenderDriverGL::mapBuffer(const HardwareBuffer & buffer, int offset, std::size_t length,
                                   Radiant::FlagsT<HardwareBuffer::MapAccess> access)
  {
    // It doesn't really matter what target we use, but maybe we minimize state
    // changes by using the correct one
    GLenum target = buffer.type();
    if(!target)
      target = GL_ARRAY_BUFFER;

    BufferHandle & bufferHandle = m_d->getBufferHandle(target, buffer);
    BufferMapping & map = m_d->m_bufferMaps[bufferHandle.handle];

    if(map.data) {
      if(map.access == access.asInt() && map.offset == offset && map.length == length)
        return map.data;
      m_d->bindBuffer(target, bufferHandle.handle);
      glUnmapBuffer(target);
      GLERROR("RenderDriverGL::mapBuffer glUnmapBuffer");
    }

    m_d->bindBuffer(target, bufferHandle.handle);
    map.access = access.asInt();
    map.target = target;
    map.offset = offset;
    map.length = length;
    map.data = glMapBufferRange(map.target, map.offset, map.length, map.access);
    GLERROR("RenderDriverGL::mapBuffer glMapBufferRange");
    return map.data;
  }

  void RenderDriverGL::unmapBuffer(const HardwareBuffer & buffer)
  {
    auto it = m_d->m_buffers.find(buffer.resourceId());
    if(it == m_d->m_buffers.end()) {
      Radiant::warning("RenderDriverGL::unmapBuffer # Buffer %lu not bound", buffer.resourceId());
      return;
    }

    auto mit = m_d->m_bufferMaps.find(it->second.handle);
    if(mit == m_d->m_bufferMaps.end()) {
      Radiant::warning("RenderDriverGL::unmapBuffer # Buffer %lu not mapped", buffer.resourceId());
      return;
    }

    m_d->bindBuffer(mit->second.target, it->second.handle);
    glUnmapBuffer(mit->second.target);

    m_d->m_bufferMaps.erase(mit);
  }

  RenderCommand & RenderDriverGL::createRenderCommand(VertexAttributeBinding & binding,
                                                      HardwareBuffer & uniformBuffer,
                                                      const Luminous::Style & style)
  {
    assert(style.fill.shader);
    m_d->m_currentProgram = 0;

    auto & state = m_d->m_state;
    state.program = &m_d->getLinkedShaderProgram(*style.fill.shader);
    state.binding = &m_d->getBindingHandle(binding, &state.index, state.program);
    state.uniformBuffer = &m_d->getBufferHandle(GL_UNIFORM_BUFFER, uniformBuffer);

    int unit = 0;
    for(auto it = style.fill.tex.begin(); it != style.fill.tex.end(); ++it) {
      TextureHandle & tex = m_d->textureHandle(*it->second, unit);
      state.textures[unit++] = &tex;
    }
    state.textures[unit] = nullptr;

    std::pair<int, std::vector<Luminous::RenderCommand>> & pair = m_d->m_opaqueQueue[state];
    if(pair.first >= pair.second.size())
      pair.second.push_back(RenderCommand());

    RenderCommand & cmd = pair.second[pair.first++];

    unit = 0;
    int slot = 0; // one day this will be different from unit
    for(auto it = style.fill.tex.begin(); it != style.fill.tex.end(); ++it, ++unit, ++slot) {
      auto texit = state.program->textures.find(it->first);
      if(texit == state.program->textures.end()) {
        GLint loc = glGetUniformLocation(state.program->handle, it->first.data());
        state.program->textures[it->first] = loc;
        cmd.samplers[slot] = std::make_pair(loc, unit);
      } else {
        cmd.samplers[slot] = std::make_pair(texit->second, unit);
      }
    }
    cmd.samplers[slot].first = -1;

    return cmd;
  }

  void RenderDriverGL::flush()
  {
    for(auto it = m_d->m_bufferMaps.begin(); it != m_d->m_bufferMaps.end(); ++it) {
      const BufferMapping & b = it->second;
      m_d->bindBuffer(b.target, it->first);
      glUnmapBuffer(b.target);
    }
    m_d->m_bufferMaps.clear();

    static int foo = 0;
    if(foo++ % 60 == 0)
      Radiant::info("%d state changes", m_d->m_opaqueQueue.size());

    glEnable(GL_DEPTH_TEST);

    for(auto it = m_d->m_opaqueQueue.begin(); it != m_d->m_opaqueQueue.end();) {
      const RenderState & state = it->first;
      std::pair<int, std::vector<Luminous::RenderCommand>> & opaque = it->second;

      if(opaque.first == 0) {
        it = m_d->m_opaqueQueue.erase(it);
        continue;
      }

      m_d->bindShaderProgram(*state.program);

      for(int t = 0; t < state.textures.size(); ++t) {
        if(!state.textures[t]) break;
        else m_d->bindTexture(*state.textures[t], t);
      }

      if(state.binding && state.index)
        m_d->setVertexBinding(*state.binding, *state.index);

      GLint uniformHandle = state.uniformBuffer->handle;
      GLint uniformBlockIndex = 0;

      for(int i = 0, s = opaque.first; i < s; ++i) {
        const RenderCommand & cmd = opaque.second[i];

        for(auto uit = cmd.samplers.begin(); uit != cmd.samplers.end(); ++uit) {
          if(uit->first < 0) break;
          glUniform1i(uit->first, uit->second);
        }

        glBindBufferRange(GL_UNIFORM_BUFFER, uniformBlockIndex, uniformHandle,
                          cmd.uniformOffsetBytes, cmd.uniformSizeBytes);
        GLERROR("RenderDriverGL::flush # glBindBufferRange");

        glDrawElementsBaseVertex(cmd.primitiveType, cmd.primitiveCount, GL_UNSIGNED_INT,
                                 (GLvoid *)((sizeof(uint) * cmd.indexOffset)), cmd.vertexOffset);
        GLERROR("RenderDriverGL::flush # glDrawElementsBaseVertex");
      }
      opaque.first = 0;
      ++it;
    }
    /*for(auto it = m_transparentQueue.begin(); it != m_transparentQueue.end(); ++it) {
    }*/
    m_d->m_transparentQueue.clear();
  }

  void RenderDriverGL::releaseResource(RenderResource::Id id)
  {
    /// @note This should only be called from the main thread
    m_d->m_releaseQueue.push_back(id);
  }
}

#undef GLERROR
