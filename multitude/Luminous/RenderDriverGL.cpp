#include "Luminous/ProgramGL.hpp"
#include "Luminous/RenderDriverGL.hpp"
#include "Luminous/StateGL.hpp"
#include "Luminous/TextureGL.hpp"
#include "Luminous/VertexArrayGL.hpp"
#include "Luminous/ResourceHandleGL.hpp"
#include "Luminous/RenderManager.hpp"
#include "Luminous/VertexArray.hpp"
#include "Luminous/VertexDescription.hpp"
#include "Luminous/Buffer.hpp"
#include "Luminous/Program.hpp"
#include "Luminous/ShaderUniform.hpp"
#include "Luminous/Texture2.hpp"
#include "Luminous/PixelFormat.hpp"
#include "Luminous/BlendMode.hpp"
#include "Luminous/DepthMode.hpp"
#include "Luminous/StencilMode.hpp"
#include "PipelineCommand.hpp"

#include <Nimble/Matrix4.hpp>
#include <memory>
#include <Radiant/Timer.hpp>
#include <Radiant/Platform.hpp>

#if defined (RADIANT_OSX)
#  include <OpenGL/gl3.h>
#elif defined (RADIANT_WINDOWS)
#  include <GL/wglew.h>
#elif defined (RADIANT_LINUX)
#  include <GL/glxew.h>
#endif

#include <cassert>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>
#include <queue>
#include <stack>

#include <QStringList>
#include <QVector>

// GL_NVX_gpu_memory_info (NVIDIA)
#define GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX          0x9047
#define GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    0x9048
#define GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049
#define GPU_MEMORY_INFO_EVICTION_COUNT_NVX            0x904A
#define GPU_MEMORY_INFO_EVICTED_MEMORY_NVX            0x904B

// GL_ATI_meminfo
#define VBO_FREE_MEMORY_ATI                     0x87FB
#define TEXTURE_FREE_MEMORY_ATI                 0x87FC
#define RENDERBUFFER_FREE_MEMORY_ATI            0x87FD

namespace Luminous
{
  struct RenderState
  {
    Luminous::ProgramGL * program;
    VertexArrayGL * vertexArray;
    BufferGL * uniformBuffer;
    std::array<TextureGL*, 8> textures;
    BlendMode blendMode;
    StencilMode stencilMode;
    DepthMode depthMode;

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

  struct OpaqueRenderQueue// : public Patterns::NotCopyable
  {
    OpaqueRenderQueue() : frame(0), usedSize(0) {}

    int frame;
    std::size_t usedSize;
    std::vector<RenderCommand> queue;
  };

  struct TranslucentRenderQueue
  {
    typedef std::vector<std::pair<RenderState, RenderCommand> > Queue;
    TranslucentRenderQueue() : frame(0), usedSize(0) {}

    int frame;
    std::size_t usedSize;
    Queue queue;
  };


  // A segment of the master render queue. A segment contains two separate
  // command queues, one for opaque draw calls and one for translucent draw
  // calls. The translucent draw calls are never re-ordered in order to
  // guarantee correct output. The opaque queue can be re-ordered to maximize
  // performance by minimizing state-changes etc. The segments themselves are
  // never re-ordered to guarantee correct output.
  struct RenderQueueSegment
  {
    RenderQueueSegment()
    {}

    RenderQueueSegment(PipelineCommand * cmd)
      : pipelineCommand(cmd)
    {
    }

    RenderQueueSegment(RenderQueueSegment && o)
      : pipelineCommand(o.pipelineCommand)
      , opaqueQueue(o.opaqueQueue)
      , translucentQueue(o.translucentQueue)
    {
      o.pipelineCommand = 0;
    }

    ~RenderQueueSegment()
    {
      delete pipelineCommand;
    }

    //RenderTargetGL * renderTarget;
    PipelineCommand * pipelineCommand;
    std::map<RenderState, OpaqueRenderQueue> opaqueQueue;
    TranslucentRenderQueue translucentQueue;
  };

  //////////////////////////////////////////////////////////////////////////
  // RenderDriver implementation
  class RenderDriverGL::D
  {
  public:
    D(unsigned int threadIndex, RenderDriverGL & driver)
      : m_driver(driver)
      , m_stateGL(threadIndex, driver)
      , m_currentBuffer(0)
      , m_threadIndex(threadIndex)
      , m_frame(0)
      , m_fps(0.0)
    {}

    typedef std::vector<GLuint> AttributeList;
    AttributeList m_activeAttributes;

    RenderDriverGL & m_driver;

    StateGL m_stateGL;
    GLuint m_currentBuffer;   // Currently bound buffer object

    typedef std::map<RenderResource::Hash, ProgramGL> ProgramList;
    typedef std::map<RenderResource::Id, TextureGL> TextureList;
    typedef std::map<RenderResource::Id, std::shared_ptr<BufferGL> > BufferList;
    typedef std::map<RenderResource::Id, VertexArrayGL> VertexArrayList;
    typedef std::map<RenderResource::Id, RenderBufferGL> RenderBufferList;
    typedef std::map<RenderResource::Id, RenderTargetGL> RenderTargetList;

    /// Resources, different maps for each type because it eliminates the need
    /// for dynamic_cast or similar, and also makes resource sharing possible
    /// for only specific resource types
    ProgramList m_programs;
    TextureList m_textures;
    BufferList m_buffers;
    VertexArrayList m_vertexArrays;
    RenderBufferList m_renderBuffers;
    RenderTargetList m_renderTargets;

    RenderState m_state;

    // Stack of active render targets
    std::stack<RenderTargetGL*, std::vector<RenderTargetGL*> > m_rtStack;
    // Master rendering queue that consists of segments of rendering commands
    std::deque<RenderQueueSegment> m_masterRenderQueue;

    // Resources to be released
    typedef std::vector<RenderResource::Id> ReleaseQueue;
    ReleaseQueue m_releaseQueue;

    unsigned int m_threadIndex;

    /// Render statistics
    int32_t m_totalBytes;         // Total bytes currently in GPU memory for this thread
    Radiant::Timer m_frameTimer;  // Time since begin of frame
    uint64_t m_frame;             // Current frame number
    double m_fps;                 // Frames per second

  public:

    /// Reset thread statistics
    void resetStatistics();

    /// Update render statistics
    void updateStatistics();

    /// Cleanup any queued-for-deletion or expired resources
    void removeResources();

    void setState(const RenderState & state);

    void applyUniform(GLint location, const Luminous::ShaderUniform & uniform);

    void render(const RenderCommand & cmd, GLint uniformHandle, GLint uniformBlockIndex);


    RenderCommand & createRenderCommand(bool translucent,
                                        const Program & shader,
                                        const VertexArray & vertexArray,
                                        const Buffer & uniformBuffer,
                                        const std::map<QByteArray,const Texture *> * textures,
                                        const std::map<QByteArray, ShaderUniform> * uniforms);

    // Utility function for resource cleanup
    template <typename ContainerType>
    void removeResource(ContainerType & container, const ReleaseQueue & releaseQueue);

    template <typename ContainerType>
    void removeResource(ContainerType & container);

    void removeBufferResource(BufferList & buffers, const ReleaseQueue & releaseQueue);

    /// Get the current render queue segment where draw calls are to be added
    //RenderQueueSegment & currentRenderQueueSegment() { assert(!m_renderTargetStack.empty()); return m_renderTargetStack.top(); }
    RenderQueueSegment & currentRenderQueueSegment() { assert(!m_masterRenderQueue.empty()); return m_masterRenderQueue.back(); }

    /// Allocate a new render queue segment defined by the given pipeline command
    void newRenderQueueSegment(PipelineCommand * cmd)
    {
      /// @todo Maybe look into a pool allocator to improve performance. Should profile more
      m_masterRenderQueue.emplace_back(cmd);
    }

    void debugOutputStats()
    {
      static int foo = 0;
      if(foo++ % 60 == 0) {

        int segments = m_masterRenderQueue.size();
        int stateChanges = 0;
        int programs = m_programs.size();
        int textures = m_textures.size();
        int buffers = m_buffers.size();
        int vertexArrays = m_vertexArrays.size();

        for(auto i = m_masterRenderQueue.begin(); i != m_masterRenderQueue.end(); ++i) {
          const RenderQueueSegment & segment = *i;
          stateChanges += segment.opaqueQueue.size() + segment.translucentQueue.queue.size();
        }

        Radiant::info("Render stats: %2d Segments, %2d State changes, %2d Programs, %2d Textures, %2d Buffer Objects, %2d VertexArrays",
                      segments, stateChanges, programs, textures, buffers, vertexArrays);


      }
    }

  };

  /////////////////////////////////////////////////////////////////////////////

  void RenderDriverGL::D::resetStatistics()
  {
    m_stateGL.clearUploadedBytes();
    m_frameTimer.start();
  }

  void RenderDriverGL::D::updateStatistics()
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
  void RenderDriverGL::D::removeResources()
  {
    removeResource(m_vertexArrays, m_releaseQueue);
    removeBufferResource(m_buffers, m_releaseQueue);
    removeResource(m_textures, m_releaseQueue);
    removeResource(m_programs);
    removeResource(m_renderBuffers, m_releaseQueue);
    removeResource(m_renderTargets, m_releaseQueue);
    m_releaseQueue.clear();
  }

  void RenderDriverGL::D::setState(const RenderState & state)
  {
    state.program->bind();

    for(std::size_t t = 0; t < state.textures.size(); ++t) {
      if(!state.textures[t]) break;
      else state.textures[t]->bind(t);
    }

    if(state.vertexArray)
      state.vertexArray->bind();
  }

  void RenderDriverGL::D::applyUniform(GLint location, const Luminous::ShaderUniform & uniform)
  {
    assert (location >= 0);
    
    // Set the uniform
    switch (uniform.type())
    {
    case ShaderUniform::Int:          glUniform1iv(location, 1, (const int*)uniform.data()); break;
    case ShaderUniform::Int2:         glUniform2iv(location, 1, (const int*)uniform.data()); break;
    case ShaderUniform::Int3:         glUniform3iv(location, 1, (const int*)uniform.data()); break;
    case ShaderUniform::Int4:         glUniform4iv(location, 1, (const int*)uniform.data()); break;
    case ShaderUniform::UnsignedInt:  glUniform1uiv(location, 1, (const unsigned int*)uniform.data()); break;
    case ShaderUniform::UnsignedInt2: glUniform2uiv(location, 1, (const unsigned int*)uniform.data()); break;
    case ShaderUniform::UnsignedInt3: glUniform3uiv(location, 1, (const unsigned int*)uniform.data()); break;
    case ShaderUniform::UnsignedInt4: glUniform4uiv(location, 1, (const unsigned int*)uniform.data()); break;
    case ShaderUniform::Float:        glUniform1fv(location, 1, (const float*)uniform.data()); break;
    case ShaderUniform::Float2:       glUniform2fv(location, 1, (const float*)uniform.data()); break;
    case ShaderUniform::Float3:       glUniform3fv(location, 1, (const float*)uniform.data()); break;
    case ShaderUniform::Float4:       glUniform4fv(location, 1, (const float*)uniform.data()); break;
    case ShaderUniform::Float2x2:     glUniformMatrix2fv(location, 1, GL_TRUE, (const float*)uniform.data()); break;
    case ShaderUniform::Float3x3:     glUniformMatrix3fv(location, 1, GL_TRUE, (const float*)uniform.data()); break;
    case ShaderUniform::Float4x4:     glUniformMatrix4fv(location, 1, GL_TRUE, (const float*)uniform.data()); break;
    default:
      Radiant::error("RenderDriverGL: Unknown shader uniform type %d", uniform.type());
      assert(false);
    }
    GLERROR("RenderDriverGL::applyUniform # glUniform");
  }

  void RenderDriverGL::D::render(const RenderCommand & cmd, GLint uniformHandle, GLint uniformBlockIndex)
  {
    // Set texture samplers
    for(auto uit = cmd.samplers.begin(); uit != cmd.samplers.end(); ++uit) {
      if(uit->first < 0) break;
      glUniform1i(uit->first, uit->second);
    }

    // Apply style-uniforms
    for(auto uniform : cmd.uniforms) {
      if (uniform.first < 0) break;
      applyUniform(uniform.first, uniform.second);
    }

    glBindBufferRange(GL_UNIFORM_BUFFER, uniformBlockIndex, uniformHandle,
                      cmd.uniformOffsetBytes, cmd.uniformSizeBytes);

    //Radiant::warning("RenderDriverGL::D::render # OFFSET %d SIZE: %d", cmd.uniformOffsetBytes, cmd.uniformSizeBytes);

    GLERROR("RenderDriverGL::flush # glBindBufferRange");

    // Set linewidth
    if (cmd.primitiveType == Luminous::PrimitiveType_Line || cmd.primitiveType == Luminous::PrimitiveType_LineStrip) {
      glLineWidth(cmd.primitiveSize);
      GLERROR("RenderDriverGL::flush # glLineWidth");
    }

    // Set point width
    if (cmd.primitiveType == Luminous::PrimitiveType_Point) {
      glPointSize(cmd.primitiveSize);
      GLERROR("RenderDriverGL::flush # glPointSize");
    }

    if (cmd.indexed) {
      // Draw using the index buffer
      glDrawElementsBaseVertex(cmd.primitiveType, cmd.primitiveCount, GL_UNSIGNED_INT,
                               (GLvoid *)((sizeof(uint) * cmd.indexOffset)), cmd.vertexOffset);
      GLERROR("RenderDriverGL::flush # glDrawElementsBaseVertex");
    }
    else {
      // Draw non-indexed
      glDrawArrays(cmd.primitiveType, cmd.vertexOffset, cmd.primitiveCount);
      GLERROR("RenderDriverGL::flush # glDrawArrays");
    }

  }
  
  RenderCommand & RenderDriverGL::D::createRenderCommand(bool translucent,
                                                         const Program & shader,
                                                         const VertexArray & vertexArray,
                                                         const Buffer & uniformBuffer,
                                                         const std::map<QByteArray,const Texture *> * textures,
                                                         const std::map<QByteArray, ShaderUniform> * uniforms)
  {
    m_state.program = &m_driver.handle(shader);
    m_state.program->link(shader);
    m_state.vertexArray = &m_driver.handle(vertexArray, m_state.program);
    m_state.uniformBuffer = &m_driver.handle(uniformBuffer);

    // In case of non-shared buffers, we'll re-upload if anything has changed
    m_state.uniformBuffer->upload(uniformBuffer, Buffer::Uniform);

    int unit = 0;
    if (textures != nullptr) {

#if defined(RADIANT_DEBUG)
      for(auto & tex: *textures)
        assert(tex.second->isValid());
#endif

      for(auto it = std::begin(*textures), end = std::end(*textures); it != end; ++it) {
        const Texture * texture = it->second;
        if(!texture->isValid())
          continue;

        TextureGL * textureGL;

        translucent |= texture->translucent();
        textureGL = &m_driver.handle(*texture);
        textureGL->upload(*texture, unit, false);

        m_state.textures[unit++] = textureGL;
      }
      m_state.textures[unit] = nullptr;
    }

    RenderQueueSegment & rt = currentRenderQueueSegment();

    RenderCommand * cmd;

    if(translucent) {
      TranslucentRenderQueue & translucentQueue = rt.translucentQueue;
      if(translucentQueue.usedSize >= translucentQueue.queue.size())
        translucentQueue.queue.resize(translucentQueue.queue.size()+1);
      auto & pair = translucentQueue.queue[translucentQueue.usedSize++];
      pair.first = m_state;
      cmd = &pair.second;
    } else {
      OpaqueRenderQueue & queue = rt.opaqueQueue[m_state];
      if(queue.usedSize >= queue.queue.size())
        queue.queue.emplace_back();
      cmd = &queue.queue[queue.usedSize++];
    }


    // Assign the samplers
    {
      /// @todo Prevent overflow if more than cmd->samplers.size() textures
      unit = 0;
      int slot = 0; // one day this will be different from unit
      if (textures != nullptr) {
        for(auto it = std::begin(*textures), end = std::end(*textures); it != end; ++it, ++unit, ++slot) {
          cmd->samplers[slot] = std::make_pair(m_state.program->uniformLocation(it->first), unit);
        }
      }
      cmd->samplers[slot].first = -1;
    }

    // Assign the uniforms
    {
      /// @todo Prevent overflow if more than cmd->uniforms.size() uniforms
      size_t slot = 0;
      if (uniforms) {
        for (auto it = std::begin(*uniforms), end = std::end(*uniforms); it != end; ++it) {
          GLint location = m_state.program->uniformLocation(it->first);
          if (location == -1) {
            Radiant::warning("RenderDriverGL - Cannot bind uniform %s - No such uniform", it->first.data());
            continue;
          }
          assert(it->second.type() != ShaderUniform::Unknown);
          cmd->uniforms[slot++] = std::make_pair(location, it->second);
        }
      }
      cmd->uniforms[slot].first = -1;
    }

    return *cmd;
  }

  template <typename ContainerType>
  void RenderDriverGL::D::removeResource(ContainerType & container, const ReleaseQueue & releaseQueue)
  {
    auto it = std::begin(container);
    while (it != std::end(container)) {
      const auto & handle = it->second;
      // First, check if resource has been deleted
      // If not, we can check if it has expired
      if(std::find( std::begin(releaseQueue), std::end(releaseQueue), it->first) !=
         std::end(releaseQueue) || handle.expired()) {
        // Remove from container
        it = container.erase(it);
      } else ++it;
    }
  }

  template <typename ContainerType>
  void RenderDriverGL::D::removeResource(ContainerType & container)
  {
    auto it = std::begin(container);
    while(it != std::end(container)) {
      const auto & handle = it->second;
      if(handle.expired()) {
        // Remove from container
        it = container.erase(it);
      } else ++it;
    }
  }

  void RenderDriverGL::D::removeBufferResource(BufferList &buffers, const ReleaseQueue & releaseQueue)
  {
    auto it = std::begin(buffers);
    while (it != std::end(buffers)) {
      std::shared_ptr<BufferGL> & buffer = it->second;

      // Check if we have the only copy of the buffer (no VertexArrayGLs
      // reference it) and it has expired.
      const bool remove = (buffer.unique() && buffer->expired());

      if(std::find( std::begin(releaseQueue), std::end(releaseQueue), it->first) !=
         std::end(releaseQueue) || remove) {
        // Remove from container
        it = buffers.erase(it);
      } else ++it;
    }
  }

  //////////////////////////////////////////////////////////////////////////
  //
  RenderDriverGL::RenderDriverGL(unsigned int threadIndex)
    : m_d(new RenderDriverGL::D(threadIndex, *this))
  {
  }

  RenderDriverGL::~RenderDriverGL()
  {
    delete m_d;
  }

  void RenderDriverGL::clear(ClearMask mask, const Radiant::Color & color, double depth, int stencil)
  {
    m_d->newRenderQueueSegment(new CommandClearGL(mask, color, depth, stencil));
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
  
  void RenderDriverGL::preFrame()
  {
    m_d->resetStatistics();
    m_d->removeResources();

    /// @todo Currently the RenderContext invalidates this cache every frame, even if it's not needed
    //m_d->m_stateGL.setProgram(0);
    //m_d->m_stateGL.setVertexArray(0);

    // Update the frame time in current state
    m_d->m_stateGL.setFrameTime(Radiant::TimeStamp::currentTime());
  }

  void RenderDriverGL::postFrame()
  {
    m_d->updateStatistics();
  }

  bool RenderDriverGL::initialize()
  {
    setDefaultState();
    return true;
  }

  void RenderDriverGL::deInitialize()
  {
    m_d->m_programs.clear();
    m_d->m_textures.clear();
    m_d->m_buffers.clear();
    m_d->m_vertexArrays.clear();
    m_d->m_renderBuffers.clear();
    m_d->m_renderTargets.clear();

    while(!m_d->m_rtStack.empty())
      m_d->m_rtStack.pop();

    m_d->m_masterRenderQueue.clear();
  }
  
  ProgramGL & RenderDriverGL::handle(const Program & program)
  {
    auto it = m_d->m_programs.find(program.hash());
    if(it == m_d->m_programs.end()) {
      it = m_d->m_programs.insert(std::make_pair(program.hash(), ProgramGL(m_d->m_stateGL, program))).first;
      it->second.setExpirationSeconds(program.expiration());
    }

    return it->second;
  }

  TextureGL & RenderDriverGL::handle(const Texture & texture)
  {
    auto it = m_d->m_textures.find(texture.resourceId());
    if(it == m_d->m_textures.end()) {
      // libstdc++ doesn't have this yet
      //it = m_d->m_textures.emplace(texture.hash(), m_d->m_stateGL).first;
      it = m_d->m_textures.insert(std::make_pair(texture.resourceId(), TextureGL(m_d->m_stateGL))).first;
      it->second.setExpirationSeconds(texture.expiration());
    }

    /// @todo avoid bind somehow?
    it->second.upload(texture, 0, false);

    return it->second;
  }

  void RenderDriverGL::setDefaultState()
  {
    // Default modes
    setBlendMode(Luminous::BlendMode::Default());
    setDepthMode(Luminous::DepthMode::Default());
    setStencilMode(Luminous::StencilMode::Default());

    // Enable scissor test
    glEnable(GL_SCISSOR_TEST);

    // Invalidate the current cached OpenGL state so it gets reset on the next
    // draw command
    m_d->m_stateGL.setProgram((unsigned)-1);
    m_d->m_stateGL.setVertexArray((unsigned)-1);
    m_d->m_stateGL.setFramebuffer(GL_FRAMEBUFFER, (unsigned)-1);
  }

  void RenderDriverGL::setBlendMode( const BlendMode & mode )
  {
    m_d->newRenderQueueSegment(new CommandSetBlendMode(mode));
  }

  void RenderDriverGL::setDepthMode(const DepthMode & mode)
  {
    m_d->newRenderQueueSegment(new CommandSetDepthMode(mode));
  }

  void RenderDriverGL::setStencilMode( const StencilMode & mode )
  {
    m_d->newRenderQueueSegment(new CommandSetStencilMode(mode));
  }

  void RenderDriverGL::setCullMode(const CullMode & mode)
  {
    m_d->newRenderQueueSegment(new CommandCullMode(mode));
  }

  void RenderDriverGL::setFrontFace(FaceWinding winding)
  {
    m_d->newRenderQueueSegment(new CommandFrontFace(winding));
  }

  void RenderDriverGL::setViewport(const Nimble::Recti & rect)
  {
    m_d->newRenderQueueSegment(new CommandViewportGL(rect));
  }

  void RenderDriverGL::setScissor(const Nimble::Recti & rect)
  {
    glEnable(GL_SCISSOR_TEST);
    m_d->newRenderQueueSegment(new CommandScissorGL(rect));
  }

  void RenderDriverGL::blit(const Nimble::Recti & src, const Nimble::Recti & dst)
  {
    m_d->newRenderQueueSegment(new CommandBlitGL(src, dst));
  }

  void RenderDriverGL::setRenderBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer)
  {
    m_d->newRenderQueueSegment(new CommandChangeRenderBuffersGL(colorBuffer, depthBuffer, stencilBuffer));
  }

  void * RenderDriverGL::mapBuffer(const Buffer & buffer, Buffer::Type type, int offset, std::size_t length,
                                   Radiant::FlagsT<Buffer::MapAccess> access)
  {
    BufferGL & bufferGL = handle(buffer);

    return bufferGL.map(type, offset, length, access);
  }

  void RenderDriverGL::unmapBuffer(const Buffer & buffer, Buffer::Type type, int offset, std::size_t length)
  {
    BufferGL & bufferGL = handle(buffer);

    bufferGL.unmap(type, offset, length);
  }

  RenderCommand & RenderDriverGL::createRenderCommand(bool translucent,
                                                      const VertexArray & vertexArray,
                                                      const Buffer & uniformBuffer,
                                                      const Luminous::Program & shader,
                                                      const std::map<QByteArray, const Texture *> * textures,
                                                      const std::map<QByteArray, ShaderUniform> * uniforms)
  {
    return m_d->createRenderCommand(translucent, shader, vertexArray, uniformBuffer, textures, uniforms);
  }

  void RenderDriverGL::flush()
  {
    for(auto it = m_d->m_stateGL.bufferMaps().begin(); it != m_d->m_stateGL.bufferMaps().end(); ++it) {
      const BufferMapping & b = it->second;
      glBindBuffer(b.target, it->first);
      glUnmapBuffer(b.target);
    }
    m_d->m_stateGL.bufferMaps().clear();

    // Debug: output some render stats
    //m_d->debugOutputStats();

    /// @note this shouldn't be needed and only results in unnecessary state changes.
    /// Every state-change is tracked already or the state is reset by the customOpenGL guard
    // Reset the OpenGL state to default
    // setDefaultState();

    // Iterate over the segments of the master render queue executing the
    // stored render commands
    while(!m_d->m_masterRenderQueue.empty()) {

      RenderQueueSegment & queues = m_d->m_masterRenderQueue.front();

      // Execute the pipeline command that defines this segment
      assert(queues.pipelineCommand);
      queues.pipelineCommand->execute();

      for(auto it = queues.opaqueQueue.begin(), end = queues.opaqueQueue.end(); it != end; ++it) {
        const RenderState & state = it->first;
        OpaqueRenderQueue & opaque = it->second;

        if(opaque.usedSize == 0)
          continue;

        m_d->setState(state);

        GLint uniformHandle = state.uniformBuffer->handle();
        GLint uniformBlockIndex = 0;

        for(int i = opaque.usedSize - 1; i >= 0; --i) {
          m_d->render(opaque.queue[i], uniformHandle, uniformBlockIndex);
        }

        if(opaque.usedSize * 10 > opaque.queue.capacity())
          opaque.frame = m_d->m_frame;

        opaque.usedSize = 0;
      }

      auto it = queues.translucentQueue.queue.begin();
      for(auto end = it + queues.translucentQueue.usedSize; it != end; ++it) {
        const RenderState & state = it->first;
        const RenderCommand & cmd = it->second;

        m_d->setState(state);
        m_d->render(cmd, state.uniformBuffer->handle(), 0);
      }

      if(queues.translucentQueue.usedSize * 10 > queues.translucentQueue.queue.capacity())
        queues.translucentQueue.frame = m_d->m_frame;

      queues.translucentQueue.usedSize = 0;

      // Remove the processed segment from the master queue
      m_d->m_masterRenderQueue.pop_front();
    }
  }

  void RenderDriverGL::releaseResource(RenderResource::Id id)
  {
    /// @note This should only be called from the main thread
    m_d->m_releaseQueue.push_back(id);
  }

  BufferGL & RenderDriverGL::handle(const Buffer & buffer)
  {
    auto it = m_d->m_buffers.find(buffer.resourceId());
    if(it == m_d->m_buffers.end()) {
      // libstdc++ doesn't have this yet
      //it = m_d->m_textures.emplace(buffer.resourceId(), m_d->m_stateGL).first;
      it = m_d->m_buffers.insert(std::make_pair(buffer.resourceId(), std::make_shared<BufferGL>(m_d->m_stateGL, buffer))).first;
      it->second->setExpirationSeconds(buffer.expiration());
    }

    return *it->second;
  }

  std::shared_ptr<BufferGL> RenderDriverGL::bufferPtr(const Buffer & buffer)
  {
    // Never creates resources, only used internally
    auto it = m_d->m_buffers.find(buffer.resourceId());
    assert(it != m_d->m_buffers.end());

    return it->second;
  }

  VertexArrayGL & RenderDriverGL::handle(const VertexArray & vertexArray, ProgramGL * program)
  {
    auto it = m_d->m_vertexArrays.find(vertexArray.resourceId());
    if(it == m_d->m_vertexArrays.end()) {

      it = m_d->m_vertexArrays.insert(std::make_pair(vertexArray.resourceId(), VertexArrayGL(m_d->m_stateGL))).first;
      it->second.setExpirationSeconds(vertexArray.expiration());
      it->second.upload(vertexArray, program);
    }

    VertexArrayGL & vertexArrayGL = it->second;

    vertexArrayGL.touch();

    /// @todo should this be done somewhere else? Should the old VertexArrayGL be destroyed?
    if(vertexArrayGL.generation() < vertexArray.generation())
      vertexArrayGL.upload(vertexArray, program);

    return vertexArrayGL;
  }

  RenderBufferGL & RenderDriverGL::handle(const RenderBuffer &buffer)
  {
    auto it = m_d->m_renderBuffers.find(buffer.resourceId());
    if(it == m_d->m_renderBuffers.end()) {
      it = m_d->m_renderBuffers.insert(std::make_pair(buffer.resourceId(), RenderBufferGL(m_d->m_stateGL))).first;
      it->second.setExpirationSeconds(buffer.expiration());
    }

    // Update OpenGL state
    it->second.sync(buffer);

    return it->second;
  }

  RenderTargetGL & RenderDriverGL::handle(const RenderTarget &target)
  {
    auto it = m_d->m_renderTargets.find(target.resourceId());
    if(it == m_d->m_renderTargets.end()) {
      it = m_d->m_renderTargets.insert(std::make_pair(target.resourceId(), RenderTargetGL(m_d->m_stateGL))).first;
      it->second.setExpirationSeconds(target.expiration());
    }

    // Update the OpenGL state
    /// @todo use generation to remove unneeded state changes?
    it->second.sync(target);

    return it->second;
  }

  void RenderDriverGL::pushRenderTarget(const RenderTarget &target)
  {
    RenderTargetGL & rtGL = handle(target);

    m_d->m_rtStack.push(&rtGL);

    auto cmd = new CommandChangeRenderTargetGL(rtGL);

    m_d->newRenderQueueSegment(cmd);
  }

  void RenderDriverGL::popRenderTarget()
  {
    assert(!m_d->m_rtStack.empty());

    m_d->m_rtStack.pop();

    // We might have emptied the stack if this was the default render target
    // popped from endFrame(). In that case, just don't activate a new target.
    if(!m_d->m_rtStack.empty()) {

      auto rt = m_d->m_rtStack.top();

      auto cmd = new CommandChangeRenderTargetGL(*rt);

      m_d->newRenderQueueSegment(cmd);
    }
  }

  unsigned long RenderDriverGL::availableGPUMemory() const
  {
    static bool nv_supported = false, ati_supported = false, checked = false;
    GLint res[4] = {0};
    if(!checked) {
      checked = true;
      glGetIntegerv(GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, res);
      nv_supported = (glGetError() == GL_NO_ERROR);
      if(nv_supported)
        return res[0];

      glGetIntegerv(TEXTURE_FREE_MEMORY_ATI, res);
      ati_supported = (glGetError() == GL_NO_ERROR);
    } else if (nv_supported) {
      glGetIntegerv(GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, res);
    } else if (ati_supported) {
      glGetIntegerv(TEXTURE_FREE_MEMORY_ATI, res);
    }

    return res[0];
  }

  unsigned long RenderDriverGL::maxGPUMemory() const
  {
    GLint res[4] = {0};
    /// Try NVidia
    glGetIntegerv(GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, res);
    if(glGetError() == GL_NO_ERROR)
      return res[0];

    /// Try ATI
    glGetIntegerv(TEXTURE_FREE_MEMORY_ATI, res);
    if(glGetError() == GL_NO_ERROR) {
      /*
      param[0] - total memory free in the pool
      param[1] - largest available free block in the pool
      param[2] - total auxiliary memory free
      param[3] - largest auxiliary free block
      */
      return res[0];
    }

    return 0;
  }

  int RenderDriverGL::uniformBufferOffsetAlignment() const
  {
    int alignment;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
    if(glGetError() == GL_NO_ERROR)
      return alignment;
    Radiant::warning("RenderDriverGL::uniformBufferOffsetAlignment # Unable to get uniform buffer offset alignment: defaulting to 256");
    return 256;
  }

  void RenderDriverGL::setVSync(bool vsync)
  {
#if defined(RADIANT_LINUX)
    Display *dpy = glXGetCurrentDisplay();
    GLXDrawable drawable = glXGetCurrentDrawable();
    const int interval = (vsync ? 1 : 0);

    // VirtuaGL means that the X server we are connected to is not the
    // server that is actually connected to the display. Setting this
    // might crash the server. For example on NVIDIA Optimus laptops
    // we need to skip this.
    const char * vendor = glXGetClientString(dpy, GLX_VENDOR);
    if (vendor && std::string(vendor) == "VirtualGL") {
      Radiant::warning("RenderDriverGL::setVSync # Not setting vsync on VirtualGL GLX");
      return;
    }

    glXSwapIntervalEXT(dpy, drawable, interval);
#elif defined (RADIANT_WINDOWS)
    const int interval = (vsync ? 1 : 0);
    wglSwapIntervalEXT(interval);
#else
#  warning "setVSync not implemented on this platform"
#endif
  }
}

#undef GLERROR
