#include "Luminous/OpenGL/RenderDriverGL.hpp"
#include "Luminous/OpenGL/StateGL.hpp"
#include "Luminous/RenderManager.hpp"
#include "Luminous/VertexArray.hpp"
#include "Luminous/VertexDescription.hpp"
#include "Luminous/Buffer.hpp"
#include "Luminous/Program.hpp"
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
#include <QVector>

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
    case Luminous::RenderResource::Program:  return glCreateProgram();
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
    case Luminous::RenderResource::Program:  glDeleteProgram(resource); break;
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

  struct BufferHandle : public ResourceHandle<Buffer>
  {
    BufferHandle(const Buffer * ptr = nullptr) : ResourceHandle(ptr), usage(Buffer::StaticDraw), size(0), uploaded(0) {}
    
    Buffer::Usage usage;
    size_t size;        // Size (in bytes)
    size_t uploaded;    // Uploaded bytes (for incremental upload)
  };
  
  struct TextureHandle : public ResourceHandle<Texture>
  {
    TextureHandle(const Texture * ptr = nullptr) : ResourceHandle(ptr), target(0) {}
    GLenum target;
    QRegion dirtyRegion;
  };

  struct ProgramHandle : public ResourceHandle<Program>
  {
    ProgramHandle(const Program * ptr = nullptr) : ResourceHandle(ptr) {}
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
  typedef ResourceHandle<VertexArray> VertexArrayHandle;


  struct RenderState
  {
    Luminous::ProgramHandle * program;
    VertexArrayHandle * vertexArray;
    BufferHandle * uniformBuffer;
    std::array<TextureHandle*, 8> textures;
    bool operator<(const RenderState & o) const
    {
      if(program != o.program)
        return program < o.program;
      if(vertexArray != o.vertexArray)
        return vertexArray < o.vertexArray;
      if(uniformBuffer != o.uniformBuffer)
        return uniformBuffer < o.uniformBuffer;
      for(std::size_t i = 0; i < textures.size(); ++i)
        if((!textures[i] || !o.textures[i]) || (textures[i] != o.textures[i]))
          return textures[i] < o.textures[i];

      return false;
    }
  };

  struct OpaqueRenderQueue
  {
    OpaqueRenderQueue() : frame(0), usedSize(0) {}

    int frame;
    std::size_t usedSize;
    std::vector<RenderCommand> queue;
  };

  struct TranslucentRenderQueue
  {
    typedef std::vector<std::pair<RenderState, RenderCommand>> Queue;
    TranslucentRenderQueue() : frame(0), usedSize(0) {}

    int frame;
    std::size_t usedSize;
    Queue queue;
  };

  //////////////////////////////////////////////////////////////////////////
  // RenderDriver implementation
  class RenderDriverGL::D
  {
  public:
    D(unsigned int threadIndex)
      : m_currentBuffer(0)
      , m_uploadedBytes(0)
      , m_threadIndex(threadIndex)
      , m_frame(0)
      , m_fps(0.0)
    {}

    typedef std::vector<GLuint> AttributeList;
    AttributeList m_activeAttributes;

    StateGL m_stateGl;
    GLuint m_currentBuffer;   // Currently bound buffer object

    std::map<GLuint, BufferMapping> m_bufferMaps;

    typedef std::map<RenderResource::Hash, ProgramHandle> ProgramList;
    typedef std::map<RenderResource::Hash, ShaderHandle> ShaderList;
    typedef std::map<RenderResource::Hash, TextureHandle> TextureList;
    typedef std::map<RenderResource::Id, BufferHandle> BufferList;
    typedef std::map<RenderResource::Id, VertexArrayHandle> VertexArrayList;

    /// Resources
    ProgramList m_programs;
    ShaderList m_shaders;
    TextureList m_textures;
    BufferList m_buffers;
    VertexArrayList m_VertexArrays;

    RenderState m_state;

    std::map<RenderState, OpaqueRenderQueue> m_opaqueQueue;
    TranslucentRenderQueue m_translucentQueue;

    // Resources to be released
    typedef std::vector<RenderResource::Id> ReleaseQueue;
    ReleaseQueue m_releaseQueue;

    unsigned int m_threadIndex;

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

      /*
      static FILE * dbg = 0;
      if(!dbg) dbg = fopen("stats", "w");
      fprintf(dbg, "%lf\n", frameTime * 1000.0);
      fflush(dbg);
      */

      m_frame++;
      m_fps = 1.0 / frameTime;
    }

    /// Cleanup any queued-for-deletion or expired resources
    void removeResources()
    {
      removeResource(m_VertexArrays, m_releaseQueue);
      removeResource(m_buffers, m_releaseQueue);
      removeResource(m_programs);
      removeResource(m_shaders);
      removeResource(m_textures);
      m_releaseQueue.clear();
    }

    void bindShaderProgram(const ProgramHandle & programHandle)
    {
      // Avoid re-applying the same shader
      if(m_stateGl.setProgram(programHandle.handle)) {
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

    bool updateShaders(const Program & program, ProgramHandle & programHandle)
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

    void relinkShaderProgram(const Program & program, ProgramHandle & programHandle)
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

    void applyShaderUniforms(const Program & program)
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
      if(blockIndex == GL_INVALID_INDEX)
        return desc;

      int uniformCount = 0;
      glGetProgramiv(programHandle.handle, GL_ACTIVE_UNIFORMS, &uniformCount);

      for(GLuint uniformIndex = 0, c = uniformCount; uniformIndex < c; ++uniformIndex) {
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

    ProgramHandle & getLinkedShaderProgram(const Program & program)
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

      bool bound = false;

      if((/*texture.mode() == Texture::Streaming &&*/ textureHandle.generation < texture.generation()) ||
         newTexture) {
        if (texture.dimensions() == 1)      textureHandle.target = GL_TEXTURE_1D;
        else if (texture.dimensions() == 2) textureHandle.target = GL_TEXTURE_2D;
        else if (texture.dimensions() == 3) textureHandle.target = GL_TEXTURE_3D;

        bindTexture(textureHandle, textureUnit);
        bound = true;

        /// Specify the internal format (number of channels or explicitly requested format)
        GLenum intFormat = texture.internalFormat();
        if(intFormat == 0) {
          if (texture.dataFormat().numChannels() == 1)      intFormat = GL_RED;
          else if (texture.dataFormat().numChannels() == 2) intFormat = GL_RG;
          else if (texture.dataFormat().numChannels() == 3) intFormat = GL_RGB;
          else intFormat = GL_RGBA;
        }

        if (texture.dimensions() == 1)      glTexImage1D(GL_TEXTURE_1D, 0, intFormat, texture.width(), 0, texture.dataFormat().layout(), texture.dataFormat().type(), nullptr );
        else if (texture.dimensions() == 2) glTexImage2D(GL_TEXTURE_2D, 0, intFormat, texture.width(), texture.height(), 0, texture.dataFormat().layout(), texture.dataFormat().type(), nullptr );
        else if (texture.dimensions() == 3) glTexImage3D(GL_TEXTURE_3D, 0, intFormat, texture.width(), texture.height(), texture.depth(), 0, texture.dataFormat().layout(), texture.dataFormat().type(), nullptr );

        /// @todo Get these from the texture settings
        glTexParameteri(textureHandle.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(textureHandle.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Update handle cache
        textureHandle.generation = texture.generation();
        textureHandle.dirtyRegion = QRegion(0, 0, texture.width(), texture.height());
      }
      textureHandle.dirtyRegion += texture.takeDirtyRegion(m_threadIndex);

      if(!bound && alwaysBind) {
        bindTexture(textureHandle, textureUnit);
        bound = true;
      }

      /// @todo 1D / 3D textures don't work atm
      if(!textureHandle.dirtyRegion.isEmpty()) {
        if(!bound)
          bindTexture(textureHandle, textureUnit);

        // Set proper alignment
        int alignment = 8;
        while (texture.width() % alignment)
          alignment >>= 1;
        glPixelStorei(GL_PACK_ALIGNMENT, alignment);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, texture.lineSizePixels());

        int uploaded = 0;

        if (texture.dimensions() == 1) {
          /// @todo incremental upload
          glTexSubImage1D(GL_TEXTURE_1D, 0, 0, texture.width(), texture.dataFormat().layout(), texture.dataFormat().type(), texture.data());
          uploaded = texture.width() * texture.dataFormat().bytesPerPixel();
        }
        else if (texture.dimensions() == 2) {
          // See how much of the bytes we can upload in this frame
          int64_t bytesFree = upload_bytes_limit - m_uploadedBytes;

          foreach(const QRect & rect, textureHandle.dirtyRegion.rects()) {
            const int bytesPerScanline = rect.width() * texture.dataFormat().bytesPerPixel();
            // Number of scanlines to upload
            const size_t scanLines = std::min<int32_t>(rect.height(), bytesFree / bytesPerScanline);

            auto data = texture.data() + (rect.left() + rect.top() * texture.width()) *
                texture.dataFormat().bytesPerPixel();

            // Upload data
            glTexSubImage2D(GL_TEXTURE_2D, 0, rect.left(), rect.top(), rect.width(), scanLines,
                            texture.dataFormat().layout(), texture.dataFormat().type(), data);
            uploaded += bytesPerScanline * scanLines;

            if(scanLines != rect.height()) {
              textureHandle.dirtyRegion -= QRegion(rect.left(), rect.top(), rect.width(), scanLines);
              break;
            } else {
              textureHandle.dirtyRegion -= rect;
            }
          }
        }
        else if (texture.dimensions() == 3) {
          /// @todo incremental upload
          glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, texture.width(), texture.height(), texture.depth(),
                          texture.dataFormat().layout(), texture.dataFormat().type(), texture.data());
          uploaded = texture.width() * texture.height() * texture.depth() * texture.dataFormat().bytesPerPixel();
        }

        // Update upload-limiter
        m_uploadedBytes += uploaded;
      }

      return textureHandle;
    }


    void setVertexAttributes(const VertexArray & binding)
    {
      // Bind all vertex buffers
      for (size_t i = 0; i < binding.bindingCount(); ++i) {
        VertexArray::Binding b = binding.binding(i);
        // Attach buffer
        auto * buffer = RenderManager::getResource<Buffer>(b.buffer);
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
        GLint location = glGetAttribLocation(m_stateGl.program(), attr.name.toAscii().data());
        if (location == -1) {
          Radiant::warning("Unable to bind vertex attribute %s", attr.name.toAscii().data());
        }
        else {
          GLenum normalized = (attr.normalized ? GL_TRUE : GL_FALSE);

          glVertexAttribPointer(location, attr.count, attr.type, normalized, description.vertexSize(), reinterpret_cast<GLvoid *>(attr.offset));
          GLERROR("RenderDriverGL::Bind VertexArray glVertexAttribPointer");
          glEnableVertexAttribArray(location);
          GLERROR("RenderDriverGL::Bind VertexArray glEnableVertexAttribArray");
        }
      }
    }

    VertexArrayHandle & getVertexArrayHandle(const VertexArray & binding, BufferHandle ** indexHandle,
                             const ProgramHandle * programHandle)
    {
      GLERROR("RenderDriverGL::getVertexArrayHandle");
      VertexArrayHandle & vertexArrayHandle = m_VertexArrays[binding.resourceId()];

      bool update = false;

      if(vertexArrayHandle.handle == 0) {
        // New resource
        vertexArrayHandle.resource = &binding;
        update = true;
      }
      else {
        // Reset usage timer
        vertexArrayHandle.lastUsed.start();

        // Check if we need to recreate
        if(vertexArrayHandle.generation < binding.generation()) {
          destroyResource(RenderResource::VertexArray, vertexArrayHandle.handle);
          update = true;
        }
      }

      if(update) {
        vertexArrayHandle.handle = createResource(RenderResource::VertexArray);
        vertexArrayHandle.generation = binding.generation();

        // Bind and setup all buffers/attributes
        glBindVertexArray(vertexArrayHandle.handle);
        if(programHandle) bindShaderProgram(*programHandle);
        setVertexAttributes(binding);

        const Buffer * index = RenderManager::getResource<Buffer>(binding.indexBuffer());
        if (index != nullptr) {
          BufferHandle & iHandle = getBufferHandle(GL_ELEMENT_ARRAY_BUFFER, *index);
          bindBuffer(GL_ELEMENT_ARRAY_BUFFER, iHandle.handle);
          if(indexHandle)
            *indexHandle = &iHandle;
        }
      } else if(indexHandle) {
        const Buffer * index = RenderManager::getResource<Buffer>(binding.indexBuffer());
        if(index)
          *indexHandle = &getBufferHandle(GL_ELEMENT_ARRAY_BUFFER, *index);
      }

      return vertexArrayHandle;
    }

    void setVertexArray(const VertexArrayHandle & vertexArrayHandle)
    {
      // Bind
      if(m_stateGl.setVertexArray(vertexArrayHandle.handle)) {
        glBindVertexArray(vertexArrayHandle.handle);
      }
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

    BufferHandle & getBufferHandle(GLenum bufferTarget, const Buffer & buffer)
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

    BufferHandle & bindBuffer(GLenum bufferTarget, const Buffer & buffer)
    {
      BufferHandle & bufferHandle = getBufferHandle(bufferTarget, buffer);
      bindBuffer(bufferTarget, bufferHandle.handle);
      return bufferHandle;
    }

    void setState(const RenderState & state)
    {
      bindShaderProgram(*state.program);

      for(std::size_t t = 0; t < state.textures.size(); ++t) {
        if(!state.textures[t]) break;
        else bindTexture(*state.textures[t], t);
      }

      if(state.vertexArray)
        setVertexArray(*state.vertexArray);
    }

    void render(const RenderCommand & cmd, GLint uniformHandle, GLint uniformBlockIndex)
    {
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
  RenderDriverGL::RenderDriverGL(unsigned int threadIndex)
    : m_d(new RenderDriverGL::D(threadIndex))
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
    GLint location = glGetUniformLocation(m_d->m_stateGl.program(), name); \
    if (location != -1) FUNCTION(location, value); \
    return (location != -1); \
  }
#define SETUNIFORMVECTOR(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderUniform(const char * name, const TYPE & value) { \
    /* @todo These locations should be cached in the program handle for performance reasons */ \
    GLint location = glGetUniformLocation(m_d->m_stateGl.program(), name); \
    if (location != -1) FUNCTION(location, 1, value.data()); \
    return (location != -1); \
  }
#define SETUNIFORMMATRIX(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderUniform(const char * name, const TYPE & value) { \
    /* @todo These locations should be cached in the program handle for performance reasons */ \
    GLint location = glGetUniformLocation(m_d->m_stateGl.program(), name); \
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

  void RenderDriverGL::setShaderProgram(const Program & program)
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
    m_d->m_stateGl.setProgram(0);
    m_d->m_stateGl.setVertexArray(0);
  }

  void RenderDriverGL::postFrame()
  {
    m_d->updateStatistics();

    // No need to run this every frame
    if((m_d->m_frame & 0x1f) != 0x1f)
      return;

    const int frameLimit = 60;
    const int lastFrame = m_d->m_frame - frameLimit;

    if(m_d->m_translucentQueue.usedSize > 0) {
      Radiant::error("RenderDriverGL::postFrame # m_translucentQueue is not empty - forgot to call flush?");
      m_d->m_translucentQueue.usedSize = 0;
    }

    if(m_d->m_translucentQueue.frame < lastFrame) {
      TranslucentRenderQueue::Queue tmp;
      std::swap(m_d->m_translucentQueue.queue, tmp);
    }

    for(auto it = m_d->m_opaqueQueue.begin(), end = m_d->m_opaqueQueue.end(); it != end;) {
      if(it->second.frame < lastFrame) {
        it = m_d->m_opaqueQueue.erase(it);
      } else ++it;
    }
  }

  void RenderDriverGL::setVertexBuffer(const Buffer & buffer)
  {
    m_d->bindBuffer(GL_ARRAY_BUFFER, buffer);
  }

  void RenderDriverGL::setIndexBuffer(const Buffer & buffer)
  {
    m_d->bindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
  }

  void RenderDriverGL::setUniformBuffer(const Buffer & buffer)
  {
    m_d->bindBuffer(GL_UNIFORM_BUFFER_EXT, buffer);
  }

  void RenderDriverGL::setVertexBinding(const VertexArray & binding)
  {
#ifdef LUMINOUS_OPENGLES
    // OpenGL ES doesn't have VAOs, so we'll just have to bind the buffers and attributes every time
    m_d->setVertexAttributes(binding);
    auto * indexBuffer = RenderManager::getResource<Buffer>(binding.indexBuffer());
    if (indexBuffer)
      setIndexBuffer(*indexBuffer);
#else
    VertexArrayHandle & vertexArrayHandle = m_d->getVertexArrayHandle(binding, nullptr, nullptr);
    m_d->setVertexArray(vertexArrayHandle);
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

  void * RenderDriverGL::mapBuffer(const Buffer & buffer, int offset, std::size_t length,
                                   Radiant::FlagsT<Buffer::MapAccess> access)
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

  void RenderDriverGL::unmapBuffer(const Buffer & buffer)
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

  RenderCommand & RenderDriverGL::createRenderCommand(VertexArray & binding,
                                                      Buffer & uniformBuffer,
                                                      const Luminous::Style & style)
  {
    assert(style.fill.shader);

    bool translucent = style.fill.shader->translucent();

    auto & state = m_d->m_state;
    state.program = &m_d->getLinkedShaderProgram(*style.fill.shader);
    state.vertexArray = &m_d->getVertexArrayHandle(binding, nullptr, state.program);
    state.uniformBuffer = &m_d->getBufferHandle(GL_UNIFORM_BUFFER, uniformBuffer);

    int unit = 0;
    for(auto it = style.fill.tex.begin(); it != style.fill.tex.end(); ++it) {
      if(!it->second->isValid())
        continue;

      translucent |= it->second->translucent();
      TextureHandle & tex = m_d->textureHandle(*it->second, unit);
      state.textures[unit++] = &tex;
    }
    state.textures[unit] = nullptr;

    translucent = style.fill.translucent == Fill::Translucent ||
        ((style.fill.translucent == Fill::Auto) && (translucent || style.fill.color.w < 0.99999999f));

    RenderCommand * cmd;
    if(translucent) {
      if(m_d->m_translucentQueue.usedSize >= m_d->m_translucentQueue.queue.size())
        m_d->m_translucentQueue.queue.resize(m_d->m_translucentQueue.queue.size()+1);
      auto & pair = m_d->m_translucentQueue.queue[m_d->m_translucentQueue.usedSize++];
      pair.first = state;
      cmd = &pair.second;
    } else {
      OpaqueRenderQueue & queue = m_d->m_opaqueQueue[state];
      if(queue.usedSize >= queue.queue.size())
        queue.queue.push_back(RenderCommand());
      cmd = &queue.queue[queue.usedSize++];
    }

    unit = 0;
    int slot = 0; // one day this will be different from unit
    for(auto it = style.fill.tex.begin(); it != style.fill.tex.end(); ++it, ++unit, ++slot) {
      auto texit = state.program->textures.find(it->first);
      if(texit == state.program->textures.end()) {
        GLint loc = glGetUniformLocation(state.program->handle, it->first.data());
        state.program->textures[it->first] = loc;
        cmd->samplers[slot] = std::make_pair(loc, unit);
      } else {
        cmd->samplers[slot] = std::make_pair(texit->second, unit);
      }
    }
    cmd->samplers[slot].first = -1;

    return *cmd;
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
    if(foo++ % 60 == 0) {
      Radiant::info("%2d State changes, %2d Programs, %2d Shaders, %2d Textures, %2d Buffer Objects, %2d VertexArrays",
                    m_d->m_opaqueQueue.size() + m_d->m_translucentQueue.queue.size(),
                    m_d->m_programs.size(), m_d->m_shaders.size(), m_d->m_textures.size(),
                    m_d->m_buffers.size(), m_d->m_VertexArrays.size());
    }

    glEnable(GL_DEPTH_TEST);

    for(auto it = m_d->m_opaqueQueue.begin(), end = m_d->m_opaqueQueue.end(); it != end; ++it) {
      const RenderState & state = it->first;
      OpaqueRenderQueue & opaque = it->second;

      if(opaque.usedSize == 0)
        continue;

      m_d->setState(state);

      GLint uniformHandle = state.uniformBuffer->handle;
      GLint uniformBlockIndex = 0;

      for(int i = 0, s = opaque.usedSize; i < s; ++i) {
        m_d->render(opaque.queue[i], uniformHandle, uniformBlockIndex);
      }

      if(opaque.usedSize * 10 > opaque.queue.capacity())
        opaque.frame = m_d->m_frame;

      opaque.usedSize = 0;
    }

    for(auto it = m_d->m_translucentQueue.queue.begin(), end = it + m_d->m_translucentQueue.usedSize; it != end; ++it) {
      const RenderState & state = it->first;
      const RenderCommand & cmd = it->second;

      m_d->setState(state);
      m_d->render(cmd, state.uniformBuffer->handle, 0);
    }

    if(m_d->m_translucentQueue.usedSize * 10 > m_d->m_translucentQueue.queue.capacity())
      m_d->m_translucentQueue.frame = m_d->m_frame;

    m_d->m_translucentQueue.usedSize = 0;
  }

  void RenderDriverGL::releaseResource(RenderResource::Id id)
  {
    /// @note This should only be called from the main thread
    m_d->m_releaseQueue.push_back(id);
  }
}

#undef GLERROR
