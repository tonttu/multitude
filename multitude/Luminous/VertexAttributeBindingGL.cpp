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

  void VertexAttributeBindingGL::deinitializeResources(unsigned int threadIndex)
  {
    glDeleteVertexArrays(1, &m_impl->vao[threadIndex]);
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