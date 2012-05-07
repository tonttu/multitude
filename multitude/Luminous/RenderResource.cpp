#include "Luminous/RenderResource.hpp"

#include <cassert>
#include <vector>

namespace Luminous
{
  class RenderResource::Impl
  {
  public:
    Impl(int threadCount)
      : gpuState(threadCount, GPU_Initializing)
      , cpuState(CPU_Clean)
    {
    }

    enum CPUState
    {
      CPU_Reallocated,
      CPU_Updated,
      CPU_Clean,
      CPU_Released,
    };
    enum GPUState
    {
      GPU_Uninitialized,
      GPU_Initializing,
      GPU_Reallocated,
      GPU_Updated,
      GPU_Clean,
      GPU_Deinitializing,
    };

    CPUState cpuState;
    std::vector<GPUState> gpuState;
  };

  RenderResource::RenderResource(int threadCount)
    : m_impl(new RenderResource::Impl(threadCount))
  {
  }

  RenderResource::~RenderResource()
  {
    assert(released());
    delete m_impl;
  }

  void RenderResource::reallocateGPU()
  {
    m_impl->cpuState = Impl::CPU_Reallocated;
  }

  void RenderResource::updateGPU()
  {
    m_impl->cpuState = Impl::CPU_Updated;
  }

  void RenderResource::update(int threadIndex)
  {
    switch (m_impl->gpuState[threadIndex])
    {
    case Impl::GPU_Clean:
      // GPU is clean, check the CPU state
      switch (m_impl->cpuState)
      {
      case Impl::CPU_Clean:
        // Nothing to do here
        break;
      case Impl::CPU_Reallocated:
        m_impl->gpuState[threadIndex] = Impl::GPU_Reallocated;
        break;

      case Impl::CPU_Updated:
        m_impl->gpuState[threadIndex] = Impl::GPU_Updated;
        break;

      case Impl::CPU_Released:
        m_impl->gpuState[threadIndex] = Impl::GPU_Deinitializing;
        break;

      default:
        // Unhandled case
        assert(false);
      }
    case Impl::GPU_Initializing:
      initializeResources(threadIndex);
      // fallthrough
    case Impl::GPU_Reallocated:
      reallocateResources(threadIndex);
      // fallthrough
    case Impl::GPU_Updated:
      updateResources(threadIndex);
      m_impl->gpuState[threadIndex] = Impl::GPU_Clean;
      break;

    case Impl::GPU_Deinitializing:
      deinitializeResources(threadIndex);
      m_impl->gpuState[threadIndex] = Impl::GPU_Uninitialized;
      break;

    default:
      // Unhandled case
      assert(false);
    }
  }

  bool RenderResource::released() const
  {
    for (int i = 0; i < threadCount(); ++i)
      if (m_impl->gpuState[i] != Impl::GPU_Uninitialized)
        return false;
    return true;
  }

  int RenderResource::threadCount() const
  {
    return static_cast<int>(m_impl->gpuState.size());
  }
  
  void RenderResource::initializeResources(int) {}
  void RenderResource::reallocateResources(int) {}
  void RenderResource::updateResources(int) {}
  void RenderResource::deinitializeResources(int) {}
}