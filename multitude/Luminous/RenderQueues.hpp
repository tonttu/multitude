/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_RENDERQUEUES_HPP
#define LUMINOUS_RENDERQUEUES_HPP

/// @cond

#include "PipelineCommand.hpp"

#include <tuple>
#include <vector>

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

  void resetCommand(RenderCommand& cmd);
  void resetCommand(std::pair<RenderState, RenderCommand>& p);

  // Simple wrapper for reusable vector for avoiding mallocs.
  // Specifically tailored for RenderCommandQueues
  template <typename T>
  class CachedVector
  {
  public:
    CachedVector() : m_usedSize(0) {}
    CachedVector(std::size_t capacity) : m_vec(capacity), m_usedSize(0) {}

    T& newEntry()
    {
      if(m_usedSize == m_vec.size()) m_vec.emplace_back();
      else resetCommand(m_vec[m_usedSize]);

      return m_vec[m_usedSize++];
    }

    std::size_t size() const { return m_usedSize; }

    const T& operator[](int i) const { return m_vec[i]; }

    void reset()
    {
      if(2*m_usedSize < m_vec.size()) m_vec.resize(1.1*m_usedSize);
      m_usedSize = 0;
    }

  private:
    std::vector<T> m_vec;
    std::size_t m_usedSize;
  };

  //Todo: tag predictions and pool with time info
  class OpaqueRenderQueuePool
  {
  public:
    OpaqueRenderQueuePool();
    ~OpaqueRenderQueuePool();

    void flush();
    void giveBack(const RenderState & state, CachedVector<RenderCommand>* vec);
    CachedVector<RenderCommand>* giveVec(const RenderState & state);

  private:
    // Todo: prediction to pair <frame, size> and forget too old frames
    typedef std::multimap<std::size_t, CachedVector<RenderCommand>* > PoolType;
    typedef std::map<std::tuple<void*, void*, void*>, std::size_t> PredType;
    PredType m_sizePredictions;
    PoolType m_pool;
  };

  //Todo: tag predictions and pool with time info
  class TranslucentRenderQueuePool
  {
  public:
    TranslucentRenderQueuePool();
    ~TranslucentRenderQueuePool();

    void flush();
    void giveBack(CachedVector<std::pair<RenderState, RenderCommand> >* vec);
    CachedVector<std::pair<RenderState, RenderCommand> >* giveVec();

  private:
    std::list<CachedVector<std::pair<RenderState, RenderCommand> >* > m_pool;
  };


  struct OpaqueRenderQueue// : public Patterns::NotCopyable
  {
    explicit OpaqueRenderQueue(CachedVector<RenderCommand> * q)
      : frame(0), queue(q) {}

    int frame;
    CachedVector<RenderCommand> *queue;
  };

  struct TranslucentRenderQueue
  {
    TranslucentRenderQueue(CachedVector<std::pair<RenderState, RenderCommand> > *q)
      : frame(0), queue(q) {}

    int frame;
    CachedVector<std::pair<RenderState, RenderCommand> > *queue;
  };

  // A segment of the master render queue. A segment contains two separate
  // command queues, one for opaque draw calls and one for translucent draw
  // calls. The translucent draw calls are never re-ordered in order to
  // guarantee correct output. The opaque queue can be re-ordered to maximize
  // performance by minimizing state-changes etc. The segments themselves are
  // never re-ordered to guarantee correct output.
  struct RenderQueueSegment
  {
    RenderQueueSegment(OpaqueRenderQueuePool& oPool,
                       TranslucentRenderQueuePool& tPool);

    RenderQueueSegment(PipelineCommand * cmd,OpaqueRenderQueuePool& oPool,
                       TranslucentRenderQueuePool& tPool);

    OpaqueRenderQueue & getOpaqueQueue(const RenderState & state);
    TranslucentRenderQueue & getTranslucentQueue();

    ~RenderQueueSegment();

    //RenderTargetGL * renderTarget;
    PipelineCommand * pipelineCommand;
    OpaqueRenderQueuePool& m_opaquePool;
    TranslucentRenderQueuePool& m_translucentPool;
    std::map<RenderState, OpaqueRenderQueue> opaqueQueue;
    TranslucentRenderQueue translucentQueue;

  private:
    RenderQueueSegment(const RenderQueueSegment & o);
    RenderQueueSegment(RenderQueueSegment && o);
    RenderQueueSegment& operator=(RenderQueueSegment && o);
  };

} //namespace Luminous

/// @endcond

#endif
