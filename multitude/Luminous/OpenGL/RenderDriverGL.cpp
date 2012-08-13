#include "Luminous/OpenGL/ProgramGL.hpp"
#include "Luminous/OpenGL/RenderDriverGL.hpp"
#include "Luminous/OpenGL/StateGL.hpp"
#include "Luminous/OpenGL/TextureGL.hpp"
#include "Luminous/OpenGL/VertexArrayGL.hpp"
#include "Luminous/OpenGL/ResourceHandleGL.hpp"
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
#include <Radiant/RefPtr.hpp>
#include <Radiant/Timer.hpp>

#ifdef RADIANT_OSX
#include <OpenGL/gl3.h>
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

namespace Luminous
{
  struct RenderState
  {
    Luminous::ProgramGL * program;
    VertexArrayGL * vertexArray;
    BufferGL * uniformBuffer;
    std::array<TextureGL*, 8> textures;

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
    typedef std::map<RenderResource::Id, BufferGL> BufferList;
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

    // key is <vertex buffer id, index buffer id, shader>
    std::map<std::tuple<RenderResource::Id, RenderResource::Id, ProgramGL*>, VertexArray> m_vertexArrayCache;

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

    void render(const RenderCommand & cmd, GLint uniformHandle, GLint uniformBlockIndex);

    RenderCommand & createRenderCommand(bool translucent,
                                        VertexArray & vertexArray,
                                        Buffer & uniformBuffer,
                                        const std::map<QByteArray,const Texture *> & textures);

    // Utility function for resource cleanup
    template <typename ContainerType>
    void removeResource(ContainerType & container, const ReleaseQueue & releaseQueue);

    template <typename ContainerType>
    void removeResource(ContainerType & container);

    /// Get the current render queue segment where draw calls are to be added
    //RenderQueueSegment & currentRenderQueueSegment() { assert(!m_renderTargetStack.empty()); return m_renderTargetStack.top(); }
    RenderQueueSegment & currentRenderQueueSegment() { assert(!m_masterRenderQueue.empty()); return m_masterRenderQueue.back(); }

    /// Allocate a new render queue segment defined by the given pipeline command
    void newRenderQueueSegment(PipelineCommand * cmd)
    {
      /// @todo use a pool allocator to improve performance

      RenderQueueSegment queues;
      queues.pipelineCommand = cmd;
      //m_renderTargetStack.push(std::move(queues));
      m_masterRenderQueue.push_back(std::move(queues));
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
    removeResource(m_buffers, m_releaseQueue);
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

  void RenderDriverGL::D::render(const RenderCommand & cmd, GLint uniformHandle, GLint uniformBlockIndex)
  {
    for(auto uit = cmd.samplers.begin(); uit != cmd.samplers.end(); ++uit) {
      if(uit->first < 0) break;
      glUniform1i(uit->first, uit->second);
    }

    glBindBufferRange(GL_UNIFORM_BUFFER, uniformBlockIndex, uniformHandle,
                      cmd.uniformOffsetBytes, cmd.uniformSizeBytes);

    //Radiant::warning("RenderDriverGL::D::render # OFFSET %d SIZE: %d", cmd.uniformOffsetBytes, cmd.uniformSizeBytes);

    GLERROR("RenderDriverGL::flush # glBindBufferRange");

    if (cmd.primitiveType == Luminous::PrimitiveType_Line || cmd.primitiveType == Luminous::PrimitiveType_LineStrip) {
      glLineWidth(cmd.primitiveSize);
      GLERROR("RenderDriverGL::flush # glLineWidth");
    }

    if (cmd.primitiveType == Luminous::PrimitiveType_Point) {
      glPointSize(cmd.primitiveSize);
      GLERROR("RenderDriverGL::flush # glPointSize");
    }

    m_driver.setBlendMode(cmd.blendMode);
    m_driver.setDepthMode(cmd.depthMode);
    m_driver.setStencilMode(cmd.stencilMode);

    glDrawElementsBaseVertex(cmd.primitiveType, cmd.primitiveCount, GL_UNSIGNED_INT,
                             (GLvoid *)((sizeof(uint) * cmd.indexOffset)), cmd.vertexOffset);
    GLERROR("RenderDriverGL::flush # glDrawElementsBaseVertex");
  }

  /// This function assumes that m_state.program is already set
  RenderCommand & RenderDriverGL::D::createRenderCommand(bool translucent,
                                                         VertexArray & vertexArray,
                                                         Buffer & uniformBuffer,
                                                         const std::map<QByteArray,const Texture *> & textures)
  {
    m_state.vertexArray = &m_driver.handle(vertexArray, m_state.program);
    m_state.uniformBuffer = &m_driver.handle(uniformBuffer);

    int unit = 0;
    for(auto it = std::begin(textures), end = std::end(textures); it != end; ++it) {
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
        queue.queue.push_back(RenderCommand());
      cmd = &queue.queue[queue.usedSize++];
    }

    unit = 0;
    int slot = 0; // one day this will be different from unit
    for(auto it = std::begin(textures), end = std::end(textures); it != end; ++it, ++unit, ++slot) {
      cmd->samplers[slot] = std::make_pair(m_state.program->samplerLocation(it->first), unit);
    }
    cmd->samplers[slot].first = -1;

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

#define SETUNIFORM(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderUniform(const char * name, const TYPE & value) { \
    /* @todo These locations should be cached in the program handle for performance reasons */ \
    GLint location = glGetUniformLocation(m_d->m_stateGL.program(), name); \
    if (location != -1) FUNCTION(location, value); \
    return (location != -1); \
  }
#define SETUNIFORMVECTOR(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderUniform(const char * name, const TYPE & value) { \
    /* @todo These locations should be cached in the program handle for performance reasons */ \
    GLint location = glGetUniformLocation(m_d->m_stateGL.program(), name); \
    if (location != -1) FUNCTION(location, 1, value.data()); \
    return (location != -1); \
  }
#define SETUNIFORMMATRIX(TYPE, FUNCTION) \
  bool RenderDriverGL::setShaderUniform(const char * name, const TYPE & value) { \
    /* @todo These locations should be cached in the program handle for performance reasons */ \
    GLint location = glGetUniformLocation(m_d->m_stateGL.program(), name); \
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
    handle(program).bind(program);
    /// @todo apply uniforms
  }

  void RenderDriverGL::preFrame()
  {
    m_d->resetStatistics();
    m_d->removeResources();

    /// @todo Currently the RenderContext invalidates this cache every frame, even if it's not needed
    m_d->m_stateGL.setProgram(0);
    m_d->m_stateGL.setVertexArray(0);
  }

  void RenderDriverGL::postFrame()
  {
    m_d->updateStatistics();

    // No need to run this every frame
    if((m_d->m_frame & 0x1f) != 0x1f)
      return;
  }

  bool RenderDriverGL::initialize()
  {
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

    m_d->m_vertexArrayCache.clear();

    while(!m_d->m_rtStack.empty())
      m_d->m_rtStack.pop();

    m_d->m_masterRenderQueue.clear();
  }


  void RenderDriverGL::setVertexBuffer(const Buffer & buffer)
  {
    assert(buffer.type() == Buffer::Vertex);
    auto & bufferGL = handle(buffer);
    bufferGL.bind();
  }

  void RenderDriverGL::setIndexBuffer(const Buffer & buffer)
  {
    assert(buffer.type() == Buffer::Index);
    auto & bufferGL = handle(buffer);
    bufferGL.bind();
  }

  void RenderDriverGL::setUniformBuffer(const Buffer & buffer)
  {
    assert(buffer.type() == Buffer::Uniform);
    auto & bufferGL = handle(buffer);
    bufferGL.bind();
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

  void RenderDriverGL::setTexture(unsigned int textureUnit, const Texture & texture)
  {
    handle(texture).upload(texture, textureUnit, true);
  }

  void RenderDriverGL::clearState()
  {
    // Default modes
    setBlendMode(Luminous::BlendMode::Default());
    setDepthMode(Luminous::DepthMode::Default());
    setStencilMode(Luminous::StencilMode::Default());

    // Enable scissor test
    glEnable(GL_SCISSOR_TEST);
  }

  void RenderDriverGL::setBlendMode( const BlendMode & mode )
  {
    glEnable(GL_BLEND);
    glBlendColor(mode.constantColor().red(), mode.constantColor().green(), mode.constantColor().blue(), mode.constantColor().alpha() );
    GLERROR("RenderDriverGL::setBlendMode # glBlendColor");
    glBlendEquation(mode.equation());
    GLERROR("RenderDriverGL::setBlendMode # glBlendEquation");
    glBlendFunc(mode.sourceFunction(), mode.destFunction());
    GLERROR("RenderDriverGL::setBlendMode # glBlendFunc");
  }

  void RenderDriverGL::setDepthMode(const DepthMode & mode)
  {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(mode.function());
    GLERROR("RenderDriverGL::setDepthMode # glDepthFunc");
    glDepthRange(mode.range().low(), mode.range().high());
    GLERROR("RenderDriverGL::setDepthMode # glDepthRange");
  }

  void RenderDriverGL::setStencilMode( const StencilMode & mode )
  {
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(mode.function(), mode.refValue(), mode.maskValue());
    GLERROR("RenderDriverGL::setStencilMode # glStencilFunc");
    glStencilOp(mode.stencilFailOperation(), mode.depthFailOperation(), mode.passOperation());
    GLERROR("RenderDriverGL::setStencilMode # glStencilOp");
  }

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
    BufferGL & bufferGL = handle(buffer);

    return bufferGL.map(offset, length, access);
  }

  void RenderDriverGL::unmapBuffer(const Buffer & buffer, int offset, std::size_t length)
  {
    BufferGL & bufferGL = handle(buffer);

    bufferGL.unmap(offset, length);
  }

  RenderCommand & RenderDriverGL::createRenderCommand(bool translucent,
                                                      Buffer & vertexBuffer,
                                                      Buffer & indexBuffer,
                                                      Buffer & uniformBuffer,
                                                      const Luminous::Program & shader,
                                                      const std::map<QByteArray, const Texture *> &textures)
  {
    auto & state = m_d->m_state;
    state.program = &handle(shader);
    state.program->link(shader);

    const auto key = std::make_tuple(vertexBuffer.resourceId(), indexBuffer.resourceId(), state.program);
    // There seems bug in VC++2010 tuple/map implementation, see
    // http://stackoverflow.com/questions/3254427/c0x-are-tuples-of-tuples-allowed
    // VertexArray & vertexArray = m_d->m_vertexArrayCache[key];
    auto it = m_d->m_vertexArrayCache.find(key);
    if(it == m_d->m_vertexArrayCache.end()) {
      VertexArray vertexArray;
      vertexArray.addBinding(vertexBuffer, state.program->vertexDescription());
      vertexArray.setIndexBuffer(indexBuffer);
      it = m_d->m_vertexArrayCache.insert(std::make_pair(key, std::move(vertexArray))).first;
    }

    return m_d->createRenderCommand(translucent, it->second, uniformBuffer,textures);
  }

  RenderCommand & RenderDriverGL::createRenderCommand(bool translucent,
                                                      VertexArray & vertexArray,
                                                      Buffer & uniformBuffer,
                                                      const Luminous::Program & shader,
                                                      const std::map<QByteArray, const Texture *> &textures)
  {
    auto & state = m_d->m_state;
    state.program = &handle(shader);
    state.program->link(shader);

    return m_d->createRenderCommand(translucent, vertexArray, uniformBuffer, textures);
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

    // Reset the OpenGL state to default
    clearState();

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
      it = m_d->m_buffers.insert(std::make_pair(buffer.resourceId(), BufferGL(m_d->m_stateGL, buffer))).first;
      it->second.setExpirationSeconds(buffer.expiration());
    }

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

  void RenderDriverGL::setVertexArray(const VertexArray & vertexArray)
  {
    auto & vertexArrayGL = handle(vertexArray);

    vertexArrayGL.bind();
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

}

#undef GLERROR
