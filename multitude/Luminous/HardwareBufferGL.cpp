#include "Luminous/HardwareBufferGL.hpp"
#include "Luminous/Luminous.hpp"

#include <vector>
#include <cassert>

namespace Luminous
{
  class HardwareBufferGL::Impl
  {
  public:
    Impl(BufferType type, BufferUsage usage, int threadCount)
      : buffers(threadCount, 0)
      , type(type)
      , usage(usage)
    {

    }

    std::vector<GLuint> buffers;
    std::vector<char> data;
    BufferType type;
    BufferUsage usage;
  };

  HardwareBufferGL::HardwareBufferGL(BufferType type, int threadCount)
    : RenderResource(threadCount)
    , m_impl(new HardwareBufferGL::Impl(type, BU_Unknown, threadCount))
  {
  }

  HardwareBufferGL::~HardwareBufferGL()
  {
    /// @todo assert if everything has been released
    delete m_impl;
  }

  void HardwareBufferGL::reallocate(size_t bytes, BufferUsage usage)
  {
    m_impl->data.resize(bytes);
    m_impl->usage = usage;

    // Trigger GPU reallocation
    reallocateGPU();
  }

  size_t HardwareBufferGL::size()
  {
    return m_impl->data.size();
  }

  void HardwareBufferGL::read(char * data, size_t offset, size_t bytes) const
  {
    assert(offset + bytes < m_impl->data.size());
    /// @todo For _READ types we need to sync with the GPU somehow
    /// Contents could be different in different threads, so user should select thread perhaps?
    const char * start = &(*(m_impl->data.cbegin() + offset));
    std::copy(start, start + bytes, data);
  }

  void HardwareBufferGL::write(const char * data, size_t bytes, size_t offset)
  {
    assert(offset + bytes < m_impl->data.size());
    char * start = &(*(m_impl->data.begin() + offset));
    std::copy(data, data + bytes, start);

    // Trigger GPU update
    updateGPU();
  }

  void HardwareBufferGL::bind(int threadIndex)
  {
    assert(threadIndex < m_impl->buffers.size());
    GLenum type = GLUtils::getBufferType(m_impl->type);
    glBindBuffer(type, m_impl->buffers[threadIndex]);
  }

  void HardwareBufferGL::unbind(int threadIndex)
  {
    assert(threadIndex < m_impl->buffers.size());
    GLenum type = GLUtils::getBufferType(m_impl->type);
    glBindBuffer(type, 0);
  }

  BufferType HardwareBufferGL::type() const
  {
    return m_impl->type;
  }

  BufferUsage HardwareBufferGL::usage() const
  {
    return m_impl->usage;
  }

  void HardwareBufferGL::initializeResources(int threadIndex)
  {
    assert(threadIndex < m_impl->buffers.size());
    glGenBuffers(1, &m_impl->buffers[threadIndex]);
  }

  void HardwareBufferGL::deinitializeResources(int threadIndex)
  {
    assert(threadIndex < m_impl->buffers.size());
    glDeleteBuffers(1, &m_impl->buffers[threadIndex]);
    m_impl->buffers[threadIndex] = 0;
  }

  void HardwareBufferGL::reallocateResources(int threadIndex)
  {
    assert(threadIndex < m_impl->buffers.size());
    GLenum type = GLUtils::getBufferType(m_impl->type);
    glBindBuffer(type, m_impl->buffers[threadIndex]);
    glBufferData(type, m_impl->data.size(), 0, m_impl->usage);
  }

  void HardwareBufferGL::updateResources(int threadIndex)
  {
    assert(threadIndex < m_impl->buffers.size());
    /// @todo Should we bother with doing partial updates?
    GLenum type = GLUtils::getBufferType(m_impl->type);
    glBindBuffer(type, m_impl->buffers[threadIndex]);
    glBufferSubData(type, 0, m_impl->data.size(), m_impl->data.data());
  }

  GLuint HardwareBufferGL::handle(int threadIndex) const
  {
    assert(threadIndex < m_impl->buffers.size());
    return m_impl->buffers[threadIndex];
  }
}