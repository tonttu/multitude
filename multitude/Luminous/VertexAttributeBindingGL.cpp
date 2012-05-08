#include "Luminous/VertexAttributeBindingGL.hpp"
#include "Luminous/HardwareBufferGL.hpp"
#include "Luminous/VertexDescription.hpp"
#include "Luminous/Luminous.hpp"

#include <vector>
#include <algorithm>

namespace Luminous
{
  class VertexAttributeBindingGL::Impl
  {
  public:
    Impl(unsigned int threadCount)
      : vao(threadCount, 0)
    {
    }

    struct Binding
    {
      Binding()
      {}

      Binding(std::shared_ptr<HardwareBuffer> buffer, std::shared_ptr<VertexDescription> description)
        : buffer(buffer)
        , description(description)
      {}

      bool operator==(const std::shared_ptr<HardwareBuffer> & rhs) const { return buffer == rhs; }

      std::shared_ptr<HardwareBuffer> buffer;
      std::shared_ptr<VertexDescription> description;
    };
    
    typedef std::vector< Binding > Bindings;
    Bindings bindings;

    std::vector<GLuint> vao;
  };

  VertexAttributeBindingGL::VertexAttributeBindingGL(unsigned int threadCount)
    : RenderResource(threadCount)
    , m_impl(new VertexAttributeBindingGL::Impl(threadCount))
  {

  }
  VertexAttributeBindingGL::~VertexAttributeBindingGL()
  {
    delete m_impl;
  }

  void VertexAttributeBindingGL::initializeResources(unsigned int threadIndex)
  {
    glGenVertexArrays(1, &m_impl->vao[threadIndex]);
  }

  void VertexAttributeBindingGL::updateResources(unsigned int threadIndex)
  {
    glBindVertexArray(m_impl->vao[threadIndex]);

    // @todo Set vertex attributes
  }

  void VertexAttributeBindingGL::deinitializeResources(unsigned int threadIndex)
  {
    glDeleteVertexArrays(1, &m_impl->vao[threadIndex]);
  }

  void VertexAttributeBindingGL::addBinding(const std::shared_ptr<HardwareBuffer> & buffer, const std::shared_ptr<VertexDescription> & description)
  {
    // Add the binding if it doesn't already exist
    Impl::Bindings::const_iterator it = std::find(m_impl->bindings.begin(), m_impl->bindings.end(), buffer);
    if (it == m_impl->bindings.end()) {
      m_impl->bindings.push_back(Impl::Binding(buffer, description));
      updateGPU();
    }
  }

  void VertexAttributeBindingGL::removeBinding(const std::shared_ptr<HardwareBuffer> & buffer)
  {
    Impl::Bindings::iterator it = std::find(m_impl->bindings.begin(), m_impl->bindings.end(), buffer);
    if (it != m_impl->bindings.end()) {
      m_impl->bindings.erase( it );
      updateGPU();
    }
  }

  void VertexAttributeBindingGL::clear()
  {
    if (!m_impl->bindings.empty()) {
      m_impl->bindings.clear();
      updateGPU();
    }
  }

  void VertexAttributeBindingGL::bind(unsigned int threadIndex)
  {
    glBindVertexArray(m_impl->vao[threadIndex]);
  }

  void VertexAttributeBindingGL::unbind(unsigned int threadIndex)
  {
    glBindVertexArray(0);
  }
}