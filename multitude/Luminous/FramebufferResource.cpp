#include <Luminous/FramebufferResource.hpp>
#include <Luminous/Error.hpp>

namespace Luminous
{

  FramebufferResource::FramebufferResource(Luminous::GLResources * r)
    : GLResource(r)
  {}

  FramebufferResource::~FramebufferResource()
  {}

  void FramebufferResource::setSize(Nimble::Vector2i size)
  {
    if(size == m_tex.size())
      return;

    m_tex.bind();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    Luminous::glErrorToString(__FILE__, __LINE__);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    Luminous::glErrorToString(__FILE__, __LINE__);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    Luminous::glErrorToString(__FILE__, __LINE__);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    Luminous::glErrorToString(__FILE__, __LINE__);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // <-  essential on Nvidia
    Luminous::glErrorToString(__FILE__, __LINE__);
  }

}
