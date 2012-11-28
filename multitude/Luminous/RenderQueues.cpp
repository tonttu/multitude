#include "PipelineCommand.hpp"
#include "RenderQueues.hpp"

namespace Luminous
{

  OpaqueRenderQueuePool::OpaqueRenderQueuePool() {}
  OpaqueRenderQueuePool::~OpaqueRenderQueuePool()
  {
    for(auto p : m_pool) delete p.second;
  }

  void 
  OpaqueRenderQueuePool::giveBack(const RenderState & state,
                                  CachedVector<RenderCommand>* vec)
  {
    auto key = std::make_tuple(state.program, state.vertexArray, state.uniformBuffer);
    auto predIt = m_sizePredictions.find(key);
    if(predIt == m_sizePredictions.end()) {
      m_sizePredictions.insert(PredType::value_type(key, vec->size()));
    } else if(predIt->second > vec->size()) {
      m_sizePredictions.insert(predIt, PredType::value_type(key, vec->size()));
    }
    // After reset, vec has at least capacity equal to its size()
    m_pool.insert(PoolType::value_type(vec->size(), vec));
    vec->reset();
  }

  CachedVector<RenderCommand>* 
  OpaqueRenderQueuePool::giveVec(const RenderState & state)
  {
    auto key = std::make_tuple(state.program, state.vertexArray, state.uniformBuffer);
    auto predIt = m_sizePredictions.find(key);
    if(predIt == m_sizePredictions.end())
      return new CachedVector<RenderCommand>();
    else {
      auto vecIt = m_pool.lower_bound(predIt->second);
      if(vecIt != m_pool.end()) {
        CachedVector<RenderCommand>* v = vecIt->second;
        m_pool.erase(vecIt);
        return v;
      } else {
        return new CachedVector<RenderCommand>(predIt->second);
      }
    }
  }

  void OpaqueRenderQueuePool::flush()
  {
    //Todo: flush only old ones
    for(auto entry : m_pool) delete entry.second;
    m_pool.clear();
  }

  void TranslucentRenderQueuePool::flush()
  {
    //Todo: flush only old ones
    for(auto entry : m_pool) delete entry;
    m_pool.clear();
  }

  TranslucentRenderQueuePool::TranslucentRenderQueuePool() {}

  TranslucentRenderQueuePool::~TranslucentRenderQueuePool()
  {
    for(auto entry : m_pool) delete entry;
  }

  void TranslucentRenderQueuePool::
  giveBack(CachedVector<std::pair<RenderState, RenderCommand> >* vec)
  {
    m_pool.push_back(vec);
    vec->reset();
  }

  CachedVector<std::pair<RenderState, RenderCommand> >*
  TranslucentRenderQueuePool::giveVec()
  {
    if(m_pool.empty()) return new CachedVector<std::pair<RenderState, RenderCommand> >();
    auto p = m_pool.front();
    m_pool.pop_front();
    return p;
  }

  void resetCommand(RenderCommand& cmd)
  {
    cmd.samplers[0].first = -1;
    cmd.uniforms[0].first = -1;
  }

  void resetCommand(std::pair<RenderState, RenderCommand>& p)
  {
    resetCommand(p.second);
  }

  RenderQueueSegment::RenderQueueSegment(OpaqueRenderQueuePool& oPool,
                                         TranslucentRenderQueuePool& tPool)
    : pipelineCommand(0)
    , m_opaquePool(oPool)
    , m_translucentPool(tPool)
    , translucentQueue(m_translucentPool.giveVec()) {}

  RenderQueueSegment::RenderQueueSegment(PipelineCommand * cmd,
                                         OpaqueRenderQueuePool& oPool,
                                         TranslucentRenderQueuePool& tPool)
    : pipelineCommand(cmd)
    , m_opaquePool(oPool)
    , m_translucentPool(tPool)
    , translucentQueue(m_translucentPool.giveVec())
  {
  }

  OpaqueRenderQueue & RenderQueueSegment::getOpaqueQueue(const RenderState & state)
  {
    typedef std::map<RenderState, OpaqueRenderQueue> OQ;
    auto it = opaqueQueue.find(state);
    if(it == opaqueQueue.end()) {
      CachedVector<RenderCommand> *vec = m_opaquePool.giveVec(state);
      auto p = opaqueQueue.insert(OQ::value_type(state, OpaqueRenderQueue(vec)));
      return p.first->second;
    }
    return it->second;
  }

  TranslucentRenderQueue & RenderQueueSegment::getTranslucentQueue()
  {
    return translucentQueue;
  }

  RenderQueueSegment::~RenderQueueSegment()
  {
    for(auto p : opaqueQueue) {
      m_opaquePool.giveBack(p.first, p.second.queue);
    }
    m_translucentPool.giveBack(translucentQueue.queue);
    delete pipelineCommand;
  }


} //namespace Luminous
