/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef LUMINOUS_GLCONTEXT_HPP
#define LUMINOUS_GLCONTEXT_HPP

#include <Radiant/Mutex.hpp>

namespace Luminous
{
  /** Abstract interface for OpenGL contexts. This class is implemented separately for each
      operating system. At the moment it is properly supported only under Linux/X11/ThreadedRendering. */
  class GLContext
  {
  public:
    GLContext();
    virtual ~GLContext();

    /// Makes this rendering context the current context for this thread
    virtual void makeCurrent() = 0;
    /// Creates a new GLContext object that shares texture ids, VBOs etc with this context
    virtual GLContext * createSharedContext() = 0;

    /// A mutex that can be used to lock the OpenGL access
    /** If the context is not shared, then this function returns null,
        and one should not use the mutex. */
    virtual Radiant::Mutex * mutex() = 0;

    class Guard
    {
    public:
      Guard(Radiant::Mutex * m) : m_mutex(m) { if(m) m->lock(); }
      ~Guard() { if(m_mutex) m_mutex->unlock(); }
    private:
      Radiant::Mutex * m_mutex;
    };
  };

  ////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////

  class GLDummyContext : public GLContext
  {
  public:
    GLDummyContext();
    virtual ~GLDummyContext();

    virtual void makeCurrent();
    virtual GLContext * createSharedContext();
    virtual Radiant::Mutex * mutex();
  };
}

#endif // GLCONTEXT_HPP
