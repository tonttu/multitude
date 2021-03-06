/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_RESOURCE_HANDLEGL_HPP
#define LUMINOUS_RESOURCE_HANDLEGL_HPP

#include "StateGL.hpp"
#include "Error.hpp"

#include <Radiant/TimeStamp.hpp>

#if RADIANT_DEBUG
# define GLERROR_TOSTR2(num) #num
# define GLERROR_TOSTR(num) GLERROR_TOSTR2(num)
# define GLERROR(txt) ::Luminous::glErrorToString(__FILE__ ":" GLERROR_TOSTR(__LINE__) ": " txt, __LINE__)
#else
# define GLERROR(txt)
#endif

namespace Luminous
{

  /// Base class for all OpenGL resources that reside in GPU memory.
  class ResourceHandleGL
  {
  public:
    /// Constructor
    /// @param state OpenGL state of the graphics driver
    inline ResourceHandleGL(StateGL & state);
    inline ~ResourceHandleGL();
    /// Move constructor
    /// @param r resource handle to move
    inline ResourceHandleGL(ResourceHandleGL && r);
    /// Move assignment operator
    /// @param r resource handle to move
    inline ResourceHandleGL & operator=(ResourceHandleGL && r);

    /// Update the last used timestamp to current frame-time. Every time the
    /// resource is accessed, this should be updated.
    /// @sa expired
    inline void touch();

    /// Check if the resource has expired. Resource is considered expired if
    /// the time since it was last accessed exceeds the defined expiration
    /// time. Expired resources are released by the graphics driver to conserve
    /// GPU memory.
    /// @sa setExpirationSeconds
    /// @sa touch
    inline bool expired() const;

    inline bool hasExternalRefs() const { return m_externalRefs != 0; }

    inline void setExpired(bool expired) { m_manuallyExpired = expired; }

    /// Set the expiration time in seconds for the resource.
    /// @param secs expiration time in seconds
    /// @sa touch
    /// @sa expired
    inline void setExpirationSeconds(unsigned int secs);

    inline int expirationSeconds() const { return m_expirationSeconds; }

    /// Get the raw OpenGL handle for the resource
    /// @return raw OpenGL handle
    GLuint handle() const { return m_handle; }

    /// Increase external usage count. expired() will always return false if
    /// there are active references to the object. Call this if you plan to
    /// use the object in a worker thread. Always call unref() later if you
    /// call ref().
    inline void ref() { ++m_externalRefs; }
    inline void unref() { --m_externalRefs; }

  protected:
    /// OpenGL state owned by the graphics driver
    StateGL & m_state;
    /// Raw OpenGL handle of the resource
    GLuint m_handle;

  private:
    Radiant::TimeStamp m_lastUsed;
    unsigned int m_expirationSeconds = 3;
    std::atomic<int> m_externalRefs{0};
    bool m_manuallyExpired = false;

  private:
    ResourceHandleGL(const ResourceHandleGL &);
    ResourceHandleGL & operator=(const ResourceHandleGL &);
  };

  /////////////////////////////////////////////////////////////////////////////

  ResourceHandleGL::ResourceHandleGL(StateGL & state)
    : m_state(state)
    , m_handle(0)
    , m_lastUsed(state.frameTime())
  {}

  ResourceHandleGL::~ResourceHandleGL()
  {
#ifdef RADIANT_DEBUG
    if (m_externalRefs > 0) {
      Radiant::error("~ResourceHandleGL called while the objects is in use");
    }
#endif
  }

  ResourceHandleGL::ResourceHandleGL(ResourceHandleGL && r)
    : m_state(r.m_state)
    , m_handle(r.m_handle)
    , m_lastUsed(r.m_lastUsed)
    , m_expirationSeconds(r.m_expirationSeconds)
  {
    r.m_handle = 0;
#ifdef RADIANT_DEBUG
    if (r.m_externalRefs > 0 || m_externalRefs > 0) {
      Radiant::error("ResourceHandleGL move constructor called while one of the objects is in use");
    }
#endif
  }

  ResourceHandleGL & ResourceHandleGL::operator=(ResourceHandleGL && r)
  {
    std::swap(m_handle, r.m_handle);
    std::swap(m_lastUsed, r.m_lastUsed);
    std::swap(m_expirationSeconds, r.m_expirationSeconds);
#ifdef RADIANT_DEBUG
    if (r.m_externalRefs > 0 || m_externalRefs > 0) {
      Radiant::error("ResourceHandleGL move assignment operator called while one of the objects is in use");
    }
#endif
    return *this;
  }

  void ResourceHandleGL::touch()
  {
    m_lastUsed = m_state.frameTime();
  }

  bool ResourceHandleGL::expired() const
  {
    if (m_externalRefs > 0)
      return false;

    if (m_manuallyExpired)
      return true;

    if(m_expirationSeconds > 0) {
      auto elapsedSeconds = (m_state.frameTime() - m_lastUsed).seconds();
      return elapsedSeconds > m_expirationSeconds;
    }

    return false;
  }

  void ResourceHandleGL::setExpirationSeconds(unsigned int secs)
  {
    m_expirationSeconds = secs;
  }
}

#endif // LUMINOUS_RESOURCE_HANDLEGL_HPP
