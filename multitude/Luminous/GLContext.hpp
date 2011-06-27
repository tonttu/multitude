/* COPYRIGHT
 */

#ifndef LUMINOUS_GLCONTEXT_HPP
#define LUMINOUS_GLCONTEXT_HPP

#include "Export.hpp"

#include <Radiant/Mutex.hpp>

namespace Luminous
{
  /** Abstract interface for OpenGL contexts. This class is implemented separately for each
      operating system. At the moment it is properly supported only under Linux/X11/ThreadedRendering.

      This class is still experimental, and its API and operation may yet change.
 */
  class LUMINOUS_API GLContext
  {
  public:
    GLContext();
    virtual ~GLContext();

    /// Makes this rendering context the current context for this thread
    virtual void makeCurrent() = 0;
    /// Creates a new GLContext object that shares texture ids, VBOs etc with this context
    virtual GLContext * createSharedContext() = 0;

    /// A mutex that can be used to lock the OpenGL access
    /** @return If the context is not shared, then this function returns null,
        and one should not use the mutex. */
    virtual Radiant::Mutex * mutex() = 0;

    /** A guard implementation that checks if the mutex is non-null. */
    class Guard : public Patterns::NotCopyable
    {
    public:
      /** Constructs a Guard object, and locks the argument mutex.

          @param m The mutex to lock. The mutex may be null, in which case nothing happens.
      */
      Guard(GLContext * c)
      {
        if(c) {
          m_mutex = c->mutex();
          if(m_mutex) m_mutex->lock();
        }
        else
          m_mutex = 0;
      }

      /** Deletes this Guard object, and frees the mutex if it is non-null. */
      ~Guard() { if(m_mutex) m_mutex->unlock(); }
    private:
      Radiant::Mutex * m_mutex;
    };
  };

  ////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////

  /** A dummy OpenGL context. This class can be used in place of a real OpenGL context. */
  class LUMINOUS_API GLDummyContext : public GLContext
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
