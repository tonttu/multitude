#ifndef LUMINOUS_RESOURCE_HANDLEGL_HPP
#define LUMINOUS_RESOURCE_HANDLEGL_HPP

#include "StateGL.hpp"
#include "OpenGL/Error.hpp"

#include <Radiant/Timer.hpp>

#if RADIANT_DEBUG
# define GLERROR_TOSTR2(num) #num
# define GLERROR_TOSTR(num) GLERROR_TOSTR2(num)
# define GLERROR(txt) Luminous::glErrorToString(__FILE__ ":" GLERROR_TOSTR(__LINE__) ": " txt, __LINE__)
#else
# define GLERROR(txt)
#endif

namespace Luminous
{
  class ResourceHandleGL
  {
  public:
    inline ResourceHandleGL(StateGL & state);
    inline ResourceHandleGL(ResourceHandleGL &&);
    inline ResourceHandleGL & operator=(ResourceHandleGL &&);

    inline void touch();
    inline bool expired() const;

    inline void setExpirationSeconds(unsigned int secs);

  protected:
    StateGL & m_state;
    GLuint m_handle;

  private:
    /// @todo isn't this a bit overkill, maybe just use a frame number,
    ///       could put the current frame number to StateGL?
    Radiant::Timer m_lastUsed;
    unsigned int m_expirationSeconds;

  private:
    ResourceHandleGL(const ResourceHandleGL &);
    ResourceHandleGL & operator=(const ResourceHandleGL &);
  };

  /////////////////////////////////////////////////////////////////////////////

  ResourceHandleGL::ResourceHandleGL(StateGL & state)
    : m_state(state)
    , m_handle(0)
  {}

  ResourceHandleGL::ResourceHandleGL(ResourceHandleGL && r)
    : m_state(r.m_state)
    , m_handle(r.m_handle)
    , m_lastUsed(r.m_lastUsed)
    , m_expirationSeconds(r.m_expirationSeconds)
  {
    r.m_handle = 0;
  }

  ResourceHandleGL & ResourceHandleGL::operator=(ResourceHandleGL && r)
  {
    std::swap(m_handle, r.m_handle);
    m_lastUsed = r.m_lastUsed;
    m_expirationSeconds = r.m_expirationSeconds;
    return *this;
  }

  void ResourceHandleGL::touch()
  {
    m_lastUsed.start();
  }

  bool ResourceHandleGL::expired() const
  {
    return m_expirationSeconds > 0 && m_lastUsed.time() > m_expirationSeconds;
  }

  void ResourceHandleGL::setExpirationSeconds(unsigned int secs)
  {
    m_expirationSeconds = secs;
  }
}

#endif // LUMINOUS_RESOURCE_HANDLEGL_HPP
