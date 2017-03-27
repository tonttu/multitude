/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "SwapGroups.hpp"
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
#include "Luminous/Texture.hpp"
#include "Luminous/PixelFormat.hpp"
#include "Luminous/BlendMode.hpp"
#include "Luminous/DepthMode.hpp"
#include "Luminous/StencilMode.hpp"
#include "Luminous/GPUAssociation.hpp"
#include "RenderQueues.hpp"

#include <Nimble/Matrix4.hpp>
#include <memory>
#include <Radiant/Timer.hpp>
#include <Radiant/Platform.hpp>

#include <cassert>
#include <map>
#include <vector>
#include <algorithm>
#include <tuple>
#include <queue>
#include <stack>

#include <QStringList>
#include <QVector>

namespace Luminous
{
  //////////////////////////////////////////////////////////////////////////
  // RenderDriver implementation
  class RenderDriverGL::D
  {
  public:
    D(unsigned int threadIndex, RenderDriverGL & driver, OpenGLAPI& opengl)
      : m_driver(driver)
      , m_stateGL(threadIndex, driver)
      , m_currentBuffer(0)
      , m_threadIndex(threadIndex)
      , m_frame(0)
      , m_fps(0.0)
      , m_gpuId(static_cast<unsigned int>(-1))
      , m_opengl(opengl)
    {
      m_state.program = nullptr;
      m_state.textures[0] = nullptr;
      m_state.uniformBuffer = nullptr;
      m_state.vertexArray = nullptr;
    }

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
    typedef std::map<RenderResource::Id, FrameBufferGL> FrameBufferList;

    /// Resources, different maps for each type because it eliminates the need
    /// for dynamic_cast or similar, and also makes resource sharing possible
    /// for only specific resource types
    ProgramList m_programs;
    TextureList m_textures;
    BufferList m_buffers;
    VertexArrayList m_vertexArrays;
    RenderBufferList m_renderBuffers;
    FrameBufferList m_frameBuffers;

    RenderState m_state;

    // Stack of active frame buffers
    std::stack<FrameBufferGL*, std::vector<FrameBufferGL*> > m_fboStack;
    // Master rendering queue that consists of segments of rendering commands
    std::deque<RenderQueueSegment> m_masterRenderQueue;

    // Pools for avoiding mallocs
    OpaqueRenderQueuePool m_opaquePool;
    TranslucentRenderQueuePool m_translucentPool;

    // Resources to be released
    typedef std::vector<RenderResource::Id> ReleaseQueue;
    ReleaseQueue m_releaseQueue;

    unsigned int m_threadIndex;

    /// Render statistics
    Radiant::Timer m_frameTimer;  // Time since begin of frame
    uint64_t m_frame;             // Current frame number
    double m_fps;                 // Frames per second

    // GPU id (AMD_gpu_association or other unique way of referring to the GPU)
    unsigned int m_gpuId;

    OpenGLAPI& m_opengl;

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
    //RenderQueueSegment & currentRenderQueueSegment() { assert(!m_frameBufferStack.empty()); return m_frameBufferStack.top(); }
    RenderQueueSegment & currentRenderQueueSegment() { assert(!m_masterRenderQueue.empty()); return m_masterRenderQueue.back(); }

    /// Allocate a new render queue segment defined by the given pipeline command
    void newRenderQueueSegment(PipelineCommand * cmd)
    {
      /// @todo Maybe look into a pool allocator to improve performance. Should profile more
      m_masterRenderQueue.emplace_back(cmd, m_opaquePool, m_translucentPool);
    }

    void debugOutputStats()
    {
      static int foo = 0;
      if(foo++ % 60 == 0) {

        int segments = static_cast<int>(m_masterRenderQueue.size());
        int stateChanges = 0;
        int programs = static_cast<int>(m_programs.size());
        int textures = static_cast<int>(m_textures.size());
        int buffers = static_cast<int>(m_buffers.size());
        int vertexArrays = static_cast<int>(m_vertexArrays.size());

        for(auto i = m_masterRenderQueue.begin(); i != m_masterRenderQueue.end(); ++i) {
          const RenderQueueSegment & segment = *i;
          stateChanges += static_cast<int>(segment.opaqueQueue.size()) +
                          static_cast<int>(segment.translucentQueue.queue->size());
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
    Radiant::Guard g(RenderManager::resourceLock());
    removeResource(m_vertexArrays, m_releaseQueue);
    removeBufferResource(m_buffers, m_releaseQueue);
    removeResource(m_textures, m_releaseQueue);
    removeResource(m_programs);
    removeResource(m_renderBuffers, m_releaseQueue);
    removeResource(m_frameBuffers, m_releaseQueue);
    m_releaseQueue.clear();
  }

  void RenderDriverGL::D::setState(const RenderState & state)
  {
    state.program->bind();

    for(std::size_t t = 0; t < state.textures.size(); ++t) {
      if(!state.textures[t]) break;
      else {
        state.textures[t]->bind(static_cast<int>(t));
      }
    }

    if(state.vertexArray) {
      state.vertexArray->bind();
    } else if (m_stateGL.setVertexArray(0)) {
      m_opengl.glBindVertexArray(0);
      GLERROR("RenderDriverGL::setState # glBindVertexArray");
    }
  }

  void RenderDriverGL::D::applyUniform(GLint location, const Luminous::ShaderUniform & uniform)
  {
    assert (location >= 0);

    // Set the uniform
    switch (uniform.type())
    {
    case ShaderUniform::Int:          m_opengl.glUniform1iv(location, 1, (const int*)uniform.data()); break;
    case ShaderUniform::Int2:         m_opengl.glUniform2iv(location, 1, (const int*)uniform.data()); break;
    case ShaderUniform::Int3:         m_opengl.glUniform3iv(location, 1, (const int*)uniform.data()); break;
    case ShaderUniform::Int4:         m_opengl.glUniform4iv(location, 1, (const int*)uniform.data()); break;
    case ShaderUniform::UnsignedInt:  m_opengl.glUniform1uiv(location, 1, (const unsigned int*)uniform.data()); break;
    case ShaderUniform::UnsignedInt2: m_opengl.glUniform2uiv(location, 1, (const unsigned int*)uniform.data()); break;
    case ShaderUniform::UnsignedInt3: m_opengl.glUniform3uiv(location, 1, (const unsigned int*)uniform.data()); break;
    case ShaderUniform::UnsignedInt4: m_opengl.glUniform4uiv(location, 1, (const unsigned int*)uniform.data()); break;
    case ShaderUniform::Float:        m_opengl.glUniform1fv(location, 1, (const float*)uniform.data()); break;
    case ShaderUniform::Float2:       m_opengl.glUniform2fv(location, 1, (const float*)uniform.data()); break;
    case ShaderUniform::Float3:       m_opengl.glUniform3fv(location, 1, (const float*)uniform.data()); break;
    case ShaderUniform::Float4:       m_opengl.glUniform4fv(location, 1, (const float*)uniform.data()); break;
    case ShaderUniform::Float2x2:     m_opengl.glUniformMatrix2fv(location, 1, GL_TRUE, (const float*)uniform.data()); break;
    case ShaderUniform::Float3x3:     m_opengl.glUniformMatrix3fv(location, 1, GL_TRUE, (const float*)uniform.data()); break;
    case ShaderUniform::Float4x4:     m_opengl.glUniformMatrix4fv(location, 1, GL_TRUE, (const float*)uniform.data()); break;
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
      m_opengl.glUniform1i(uit->first, uit->second);
      GLERROR("RenderDriverGL::render # glUniform1i");
    }

    // Apply style-uniforms
    for(auto uniform : cmd.uniforms) {
      if (uniform.first < 0) break;
      applyUniform(uniform.first, uniform.second);
    }

    m_opengl.glBindBufferRange(GL_UNIFORM_BUFFER, uniformBlockIndex, uniformHandle,
                               cmd.uniformOffsetBytes, cmd.uniformSizeBytes);

    //Radiant::warning("RenderDriverGL::D::render # OFFSET %d SIZE: %d", cmd.uniformOffsetBytes, cmd.uniformSizeBytes);

    GLERROR("RenderDriverGL::render # glBindBufferRange");

    // Set linewidth
    if (cmd.primitiveType == Luminous::PRIMITIVE_LINE || cmd.primitiveType == Luminous::PRIMITIVE_LINE_STRIP) {
      m_opengl.glLineWidth(cmd.primitiveSize);
      GLERROR("RenderDriverGL::render # glLineWidth");
    }

    // Set point width
    if (cmd.primitiveType == Luminous::PRIMITIVE_POINT) {
      m_opengl.glPointSize(cmd.primitiveSize);
      GLERROR("RenderDriverGL::render # glPointSize");
    }

    if (cmd.indexed) {
      // Draw using the index buffer
      m_opengl.glDrawElementsBaseVertex(cmd.primitiveType, cmd.primitiveCount, GL_UNSIGNED_INT,
                                        (GLvoid *)((sizeof(uint) * cmd.indexOffset)), cmd.vertexOffset);
      GLERROR("RenderDriverGL::render # glDrawElementsBaseVertex");
    }
    else {
      // Draw non-indexed
      m_opengl.glDrawArrays(cmd.primitiveType, cmd.vertexOffset, cmd.primitiveCount);
      GLERROR("RenderDriverGL::render # glDrawArrays");
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
    m_state.vertexArray = &m_driver.handle(vertexArray, m_state.program);
    m_state.uniformBuffer = &m_driver.handle(uniformBuffer);

    /// @todo why did we do this, makes no sense?
    //if(vertexArray.indexBuffer() != 0) m_state.vertexArray->bind();

    // In case of non-shared buffers, we'll re-upload if anything has changed
    m_state.uniformBuffer->upload(uniformBuffer, Buffer::UNIFORM);

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
    }
    m_state.textures[unit] = nullptr;

    RenderQueueSegment & rt = currentRenderQueueSegment();

    RenderCommand * cmd;

    if(translucent) {
      TranslucentRenderQueue & queue = rt.getTranslucentQueue();
      auto & pair = queue.queue->newEntry();
      pair.first = m_state;
      cmd = &pair.second;
    } else {
      OpaqueRenderQueue & queue = rt.getOpaqueQueue(m_state);
      cmd = &queue.queue->newEntry();
    }

    // Assign the samplers
    {
      unit = 0;
      std::size_t slot = 0; // one day this will be different from unit... when that day comes fix resetCommand
      if (textures != nullptr) {
        auto it = std::begin(*textures);
        auto end = std::end(*textures);
        while (it != end && slot < cmd->samplers.size()) {
          auto location = m_state.program->uniformLocation(it->first);
          if (location >= 0) {
            cmd->samplers[slot++] = std::make_pair(location, unit++);
          }
          else {
            Radiant::warning("RenderDriverGL - Cannot bind sampler %s - No such sampler found", it->first.data());
          }
          ++it;
        }
      }
      cmd->samplers[slot].first = -1;
    }

    // Assign the uniforms
    {
      size_t slot = 0;
      if (uniforms) {
        auto it = std::begin(*uniforms);
        auto end = std::end(*uniforms);
        while (it != end && slot < cmd->uniforms.size()) {
          GLint location = m_state.program->uniformLocation(it->first);
          if (location >= 0) {
            assert(it->second.type() != ShaderUniform::Unknown);
            cmd->uniforms[slot++] = std::make_pair(location, it->second);
          }
          else {
            Radiant::warning("RenderDriverGL - Cannot bind uniform %s - No such uniform", it->first.data());
          }

          ++it;
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
  RenderDriverGL::RenderDriverGL(unsigned int threadIndex, OpenGLAPI& opengl)
    : m_d(new RenderDriverGL::D(threadIndex, *this, opengl))
  {
  }

  RenderDriverGL::~RenderDriverGL()
  {
    delete m_d;
  }

  void RenderDriverGL::clear(ClearMask mask, const Radiant::ColorPMA & color, double depth, int stencil)
  {
    m_d->newRenderQueueSegment(new CommandClearGL(m_d->m_opengl, mask, color, depth, stencil));
  }

  void RenderDriverGL::draw(PrimitiveType type, unsigned int offset, unsigned int primitives)
  {
    m_d->m_opengl.glDrawArrays(type, (GLint) offset, (GLsizei) primitives);
    GLERROR("RenderDriverGL::draw glDrawArrays");
  }

  void RenderDriverGL::drawIndexed(PrimitiveType type, unsigned int offset, unsigned int primitives)
  {
    /// @todo allow other index types (unsigned byte, unsigned short and unsigned int)
    m_d->m_opengl.glDrawElements(type, (GLsizei) primitives, GL_UNSIGNED_INT, (const GLvoid *)((sizeof(uint) * offset)));
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
    m_d->m_frameBuffers.clear();

    while(!m_d->m_fboStack.empty())
      m_d->m_fboStack.pop();

    m_d->m_masterRenderQueue.clear();
  }

  ProgramGL & RenderDriverGL::handle(const Program & program)
  {
    auto it = m_d->m_programs.find(program.hash());
    if(it == m_d->m_programs.end()) {
      it = m_d->m_programs.insert(std::make_pair(program.hash(), ProgramGL(m_d->m_stateGL, program))).first;
      it->second.setExpirationSeconds(program.expiration());
    }

    it->second.link(program);
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
    if (texture.isValid()) {
      it->second.upload(texture, 0, false);
    }

    return it->second;
  }

  TextureGL * RenderDriverGL::findHandle(const Texture & texture)
  {
    auto it = m_d->m_textures.find(texture.resourceId());
    if(it == m_d->m_textures.end()) {
      return nullptr;
    } else {
      return &it->second;
    }
  }

  void RenderDriverGL::setDefaultState()
  {
    m_d->m_opengl.glEnable(GL_SAMPLE_SHADING);
    GLERROR("RenderDriverGL::setDefaultState # glEnable");

    // Default modes
    setBlendMode(Luminous::BlendMode::Default());
    setDepthMode(Luminous::DepthMode::Default());
    setStencilMode(Luminous::StencilMode::Default());
    setCullMode(Luminous::CullMode::Default());

    // By default render to back buffer
    std::vector<GLenum> buffers;
    buffers.push_back(GL_BACK_LEFT);
    setDrawBuffers(buffers);

    // Enable scissor test
    m_d->m_opengl.glEnable(GL_SCISSOR_TEST);
    GLERROR("RenderDriverGL::setDefaultState # glEnable");

    // Invalidate the current cached OpenGL state so it gets reset on the next
    // draw command
    m_d->m_stateGL.setProgram((unsigned)-1);
    m_d->m_stateGL.setVertexArray((unsigned)-1);
    m_d->m_stateGL.setFramebuffer(GL_FRAMEBUFFER, (unsigned)-1);
  }

  void RenderDriverGL::setBlendMode( const BlendMode & mode )
  {
    m_d->newRenderQueueSegment(new CommandSetBlendMode(m_d->m_opengl, mode));
  }

  void RenderDriverGL::setDepthMode(const DepthMode & mode)
  {
    m_d->newRenderQueueSegment(new CommandSetDepthMode(m_d->m_opengl, mode));
  }

  void RenderDriverGL::setStencilMode( const StencilMode & mode )
  {
    m_d->newRenderQueueSegment(new CommandSetStencilMode(m_d->m_opengl, mode));
  }

  void RenderDriverGL::setCullMode(const CullMode & mode)
  {
    m_d->newRenderQueueSegment(new CommandCullMode(m_d->m_opengl, mode));
  }

  void RenderDriverGL::setFrontFace(FaceWinding winding)
  {
    m_d->newRenderQueueSegment(new CommandFrontFace(m_d->m_opengl, winding));
  }

  void RenderDriverGL::enableClipDistance(const QList<int> & planes)
  {
    m_d->newRenderQueueSegment(new CommandClipDistance(m_d->m_opengl, planes, true));
  }

  void RenderDriverGL::disableClipDistance(const QList<int> & planes)
  {
    m_d->newRenderQueueSegment(new CommandClipDistance(m_d->m_opengl, planes, false));
  }

  void RenderDriverGL::setDrawBuffers(const std::vector<GLenum> & buffers)
  {
    m_d->newRenderQueueSegment(new CommandDrawBuffers(m_d->m_opengl, buffers));
  }

  void RenderDriverGL::setViewport(const Nimble::Recti & rect)
  {
    m_d->newRenderQueueSegment(new CommandViewportGL(m_d->m_opengl, rect));
  }

  void RenderDriverGL::setScissor(const Nimble::Recti & rect)
  {
    m_d->m_opengl.glEnable(GL_SCISSOR_TEST);
    GLERROR("RenderDriverGL::setScissor # glEnable");
    m_d->newRenderQueueSegment(new CommandScissorGL(m_d->m_opengl, rect));
  }

  void RenderDriverGL::blit(const Nimble::Recti &src, const Nimble::Recti &dst,
                            Luminous::ClearMask mask, Luminous::Texture::Filter filter)
  {
    m_d->newRenderQueueSegment(new CommandBlitGL(m_d->m_opengl, src, dst, mask, filter));
  }

  void RenderDriverGL::setRenderBuffers(bool colorBuffer, bool depthBuffer, bool stencilBuffer)
  {
    m_d->newRenderQueueSegment(new CommandChangeRenderBuffersGL(m_d->m_opengl, colorBuffer, depthBuffer, stencilBuffer));
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
      m_d->m_opengl.glBindBuffer(b.target, it->first);
      GLERROR("RenderDriverGL::flush # glBindBuffer");
      m_d->m_opengl.glUnmapBuffer(b.target);
      GLERROR("RenderDriverGL::flush # glUnmapBuffer");
    }
    m_d->m_stateGL.bufferMaps().clear();

    m_d->m_opaquePool.flush();
    m_d->m_translucentPool.flush();

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

        if(opaque.queue->size() == 0)
          continue;

        m_d->setState(state);

        GLint uniformHandle = state.uniformBuffer->handle();
        GLint uniformBlockIndex = 0;

        for(int i = static_cast<int>(opaque.queue->size()) - 1; i >= 0; --i) {
          m_d->render((*opaque.queue)[i], uniformHandle, uniformBlockIndex);
        }

        // TODO: Was there any use of frame?
        //if(opaque.usedSize * 10 > opaque.queue.capacity())
        //  opaque.frame = m_d->m_frame;

        //opaque.usedSize = 0;
      }

      for(std::size_t i = 0; i < queues.translucentQueue.queue->size(); ++i) {
        auto p = (*queues.translucentQueue.queue)[(int)i];
        const RenderState & state = p.first;
        const RenderCommand & cmd = p.second;
        m_d->setState(state);
        m_d->render(cmd, state.uniformBuffer->handle(), 0);
      }

      //if(queues.translucentQueue.usedSize * 10 > queues.translucentQueue.queue.capacity())
      //  queues.translucentQueue.frame = m_d->m_frame;

      //queues.translucentQueue.usedSize = 0;

      // Remove the processed segment from the master queue
      m_d->m_masterRenderQueue.pop_front();
    }

    // VAO should be bound only when rendering something or modifying the VAO state
    if (m_d->m_stateGL.setVertexArray(0)) {
      m_d->m_opengl.glBindVertexArray(0);
      GLERROR("RenderDriverGL::flush # glBindVertexArray");
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

  VertexArrayGL & RenderDriverGL::handle(const VertexArray & vertexArray, ProgramGL * program)
  {
    auto it = m_d->m_vertexArrays.find(vertexArray.resourceId());
    if(it == m_d->m_vertexArrays.end()) {
      it = m_d->m_vertexArrays.insert(std::make_pair(vertexArray.resourceId(), VertexArrayGL(m_d->m_stateGL))).first;
      it->second.setExpirationSeconds(vertexArray.expiration());
    }

    VertexArrayGL & vertexArrayGL = it->second;

    vertexArrayGL.touch();

    /// @todo should this be done somewhere else? Should the old VertexArrayGL be destroyed?
    if(vertexArrayGL.generation() < vertexArray.generation())
      vertexArrayGL.upload(vertexArray, program);

    // Check if any of the associated buffers have changed
    for (size_t i = 0; i < vertexArray.bindingCount(); ++i) {
      auto & binding = vertexArray.binding(i);
      auto * buffer = RenderManager::getResource<Buffer>(binding.buffer);
      assert(buffer != nullptr);
      auto & buffergl = handle(*buffer);
      buffergl.upload(*buffer, Buffer::VERTEX);
    }

    RenderResource::Id indexBufferId = vertexArray.indexBuffer();
    if (indexBufferId) {
      auto * buffer = RenderManager::getResource<Buffer>(indexBufferId);
      assert(buffer);
      auto & buffergl = handle(*buffer);
      buffergl.upload(*buffer, Buffer::INDEX);
    }

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

  FrameBufferGL & RenderDriverGL::handle(const FrameBuffer &target)
  {
    auto it = m_d->m_frameBuffers.find(target.resourceId());
    if(it == m_d->m_frameBuffers.end()) {
      it = m_d->m_frameBuffers.insert(std::make_pair(target.resourceId(), FrameBufferGL(m_d->m_stateGL))).first;
      it->second.setExpirationSeconds(target.expiration());
    }

    // Update the OpenGL state
    /// @todo use generation to remove unneeded state changes?
    it->second.sync(target);

    return it->second;
  }

  void RenderDriverGL::pushFrameBuffer(const FrameBuffer &target)
  {
    FrameBufferGL & rtGL = handle(target);

    m_d->m_fboStack.push(&rtGL);

    auto cmd = new CommandChangeFrameBufferGL(m_d->m_opengl, rtGL);

    m_d->newRenderQueueSegment(cmd);
  }

  void RenderDriverGL::popFrameBuffer()
  {
    assert(!m_d->m_fboStack.empty());

    m_d->m_fboStack.pop();

    // We might have emptied the stack if this was the default frame buffer
    // popped from endFrame(). In that case, just don't activate a new target.
    if(!m_d->m_fboStack.empty()) {

      auto rt = m_d->m_fboStack.top();

      auto cmd = new CommandChangeFrameBufferGL(m_d->m_opengl, *rt);

      m_d->newRenderQueueSegment(cmd);
    }
  }

  int64_t RenderDriverGL::uploadLimit() const
  {
    return m_d->m_stateGL.uploadLimit();
  }

  int64_t RenderDriverGL::uploadMargin() const
  {
    return m_d->m_stateGL.uploadMargin();
  }

  void RenderDriverGL::setUploadLimits(int64_t limit, int64_t margin)
  {
    m_d->m_stateGL.setUploadLimits(limit,margin);
  }

  int RenderDriverGL::uniformBufferOffsetAlignment() const
  {
    int alignment;
    m_d->m_opengl.glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
    if(m_d->m_opengl.glGetError() == GL_NO_ERROR)
      return alignment;
    Radiant::warning("RenderDriverGL::uniformBufferOffsetAlignment # Unable to get uniform buffer offset alignment: defaulting to 256");
    return 256;
  }

  void RenderDriverGL::setUpdateFrequency(float fps)
  {
    m_d->m_stateGL.setUpdateFrequency(Nimble::Math::Round(fps));
  }

  void RenderDriverGL::setGPUId(unsigned int gpuId)
  {
    m_d->m_gpuId = gpuId;
  }

  unsigned int RenderDriverGL::gpuId() const
  {
    return m_d->m_gpuId;
  }

  bool RenderDriverGL::setupSwapGroup(int group)
  {
    // Do nothing if the extension is not supported
    if(!SwapGroups::isExtensionSupported())
      return false;

    // Query the number of available swap groups and barriers
    GLuint maxGroups, maxBarriers;
    if(SwapGroups::queryMaxSwapGroup(maxGroups, maxBarriers)) {

      // If we have any swap groups, join the first one
      if(maxGroups > 0) {
        return SwapGroups::joinSwapGroup(group);
      }

    }

    return false;
  }

  OpenGLAPI& RenderDriverGL::opengl()
  {
    return m_d->m_opengl;
  }

}

#undef GLERROR
