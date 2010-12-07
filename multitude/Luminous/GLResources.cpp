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

#include "GLResources.hpp"
#include "GarbageCollector.hpp"
#include "GLResource.hpp"
#include "Luminous.hpp"

#include <Nimble/Math.hpp>

#include <Radiant/Mutex.hpp>
#include <Radiant/Thread.hpp>
#include <Radiant/Trace.hpp>

#include <cassert>

#include <stdlib.h>

#include <typeinfo>

namespace Luminous
{

  using namespace Radiant;

  GLResources::GLResources(Radiant::ResourceLocator & rl)
    : m_deallocationSum(0),
      m_allocationSum(0),
      m_consumingBytes(0),
      m_comfortableGPURAM((1 << 20) * 70), // 70 MB
      m_frame(0),
      m_brokenProxyTexture2D(false),
      m_resourceLocator(rl)
  {
    const char * envgp = getenv("MULTI_GPU_RAM");

    if(envgp) {

      m_comfortableGPURAM = Nimble::Math::Max(atol(envgp) * (1 << 20),
                          m_comfortableGPURAM);
    }

  }

  GLResources::~GLResources()
  {
    while(m_resources.size())
      eraseResource((*m_resources.begin()).first);

    if(m_consumingBytes != 0)
      Radiant::error("GLResources::~GLResources # The GPU memory is left at %ld -> "
                     "there is a bug in your application.",
                     m_consumingBytes);
  }

  bool GLResources::init()
  {

    const char * glvendor = (const char *) glGetString(GL_VENDOR);
    if(strstr(glvendor, "ATI")) {
      m_brokenProxyTexture2D = true;
    }
    else
      m_brokenProxyTexture2D = false;

    return true;
  }

  GLResource * GLResources::getResource(const Collectable * key, int deleteAfterFrames)
  {
    iterator it = m_resources.find(key);

    if(it == m_resources.end())
      return 0;

    deleteAfter(it->second, deleteAfterFrames);

    return (*it).second;
  }

  void GLResources::addResource(const Collectable * key, GLResource * resource)
  {
    iterator it = m_resources.find(key);

    if(it != m_resources.end()) {
      Radiant::error("GLResources::addResource # "
                     "There already is a resource %p for %p (%s) in %p",
                     it->second, key,
                     typeid(*it->second).name(), this);
      eraseResource(key);
    }

    m_resources[key] = resource;
    long bytes = resource->consumesBytes();
    m_consumingBytes += bytes;
    m_allocationSum += bytes;
  }

  bool GLResources::eraseResource(const Collectable * key)
  {
    iterator it = m_resources.find(key);

    if(it == m_resources.end()) {
      //Radiant::error("GLResources::eraseResource # No resource for %p", key);
      return false;
    }

    GLResource * resource = (*it).second;

    /* long bytes = resource->consumesBytes();

    m_consumingBytes  -= bytes;
    m_deallocationSum += bytes;
    */

    // Radiant::info("GLResources::eraseResource # Resource %s for %p erased", typeid(*resource).name(), key);

    delete resource;
    m_resources.erase(it);

    return true;
  }

  void GLResources::eraseResources()
  {
    /* Radiant::info("GLResources::eraseResources # checking %d deleted keys",
                  Luminous::GarbageCollector::size());
    */
    eraseOnce();
    m_frame++;

    // memory usage counters don't tell much, destroy resources always
    m_comfortableGPURAM = 0;

    for(iterator it = m_resources.begin();
    (m_consumingBytes >= m_comfortableGPURAM) &&
      (it != m_resources.end()); ) {

      GLResource * r = (*it).second;


      if(!r->persistent() && r->m_deleteOnFrame && r->m_deleteOnFrame < m_frame) {

        //Radiant::info("GLResources::eraseResource # Resource %s for %p erased", typeid(*(*it).second).name(), (*it).first);
        // Radiant::trace("GLResources::eraseResources # Removing old");

        delete (*it).second;
        iterator tmp = it;
        tmp++;
        m_resources.erase(it);
        it = tmp;
      }
      else
        it++;
    }
  }

  void GLResources::clear()
  {
    while(m_resources.size()) {
      GLResource * res = (*m_resources.begin()).second;
      delete res;
      m_resources.erase(m_resources.begin());
    }

    m_deallocationSum = 0;
    m_allocationSum = 0;
    m_consumingBytes = 0;
  }

  void GLResources::changeByteConsumption(long deallocated, long allocated)
  {
    m_deallocationSum += deallocated;
    m_allocationSum   += allocated;
    // m_consumingBytes  += (allocated - deallocated);

    // assert(m_consumingBytes >= 0);
  }

  bool GLResources::canUseGPUBandwidth(float priority)
  {
    /// @todo why?
    return true;

    // Overriding priority: Always go.
    if(priority >= 100.0f)
      return true;

    float perframe = 100000;
    float rel = m_allocationSum / perframe;

    // Not much used -> always go
    if(rel < 0.3f)
      return true;

    // Some bandwidth used -> select if uploads are to go.
    rel = (rel - 0.3f) / 0.007f;

    return rel < priority;
  }

  void GLResources::deleteAfter(GLResource * resource, int frames)
  {
    if(resource->m_deleteOnFrame == GLResource::PERSISTENT)
      ;
    else if(frames >= 0)
      resource->m_deleteOnFrame = m_frame + frames;
    else
      resource->m_deleteOnFrame = 0;
  }

  // Doesn't work under windows where pthread_t (id_t) is a struct
  //typedef std::map<Thread::id_t, GLResources *> ResourceMap;
  class TGLRes
  {
  public:
    TGLRes() : m_glr(0), m_window(0), m_area(0) {}
    GLResources       * m_glr;
    const MultiHead::Window * m_window;
    const MultiHead::Area   * m_area;
  };

#ifndef WIN32
  typedef std::map<Thread::id_t, TGLRes> ResourceMap;
#else
  typedef std::map<unsigned int, TGLRes> ResourceMap;
#endif

  static ResourceMap __resources;
  static MutexStatic __mutex;

  void GLResources::setThreadResources(GLResources * rsc,
                       const MultiHead::Window *w, const MultiHead::Area *a)
  {
    GuardStatic g(&__mutex);
    TGLRes tmp;
    tmp.m_glr = rsc;
    tmp.m_window = w;
    tmp.m_area = a;
#ifndef WIN32
    __resources[Thread::myThreadId()] = tmp;
#else
    __resources[0] = tmp;
#endif
  }

  GLResources * GLResources::getThreadResources()
  {
    GuardStatic g(&__mutex);

#ifndef WIN32
    ResourceMap::iterator it = __resources.find(Thread::myThreadId());
#else
    ResourceMap::iterator it = __resources.find(0);
#endif

    if(it == __resources.end()) {
      debug("No OpenGL resources for current thread");
      return 0;
    }

    return (*it).second.m_glr;
  }

  void GLResources::getThreadMultiHead(const MultiHead::Window ** w, const MultiHead::Area ** a)
  {
    GuardStatic g(&__mutex);

#ifndef WIN32
    ResourceMap::iterator it = __resources.find(Thread::myThreadId());
#else
    ResourceMap::iterator it = __resources.find(0);
#endif

    if(it == __resources.end()) {
      error("No OpenGL resources for current thread");
      return;
    }

    if(w)
      *w = (*it).second.m_window;
    if(a)
      *a = (*it).second.m_area;
  }

  const MultiHead::Area * GLResources::getThreadMultiHeadArea()
  {
    GuardStatic g(&__mutex);

#ifndef WIN32
    ResourceMap::iterator it = __resources.find(Thread::myThreadId());
#else
    ResourceMap::iterator it = __resources.find(0);
#endif

    if(it == __resources.end()) {
      error("No OpenGL resources for current thread");
      return 0;
    }

    return (*it).second.m_area;
  }

  const MultiHead::Window * GLResources::getThreadMultiHeadWindow()
  {
    GuardStatic g(&__mutex);

#ifndef WIN32
    ResourceMap::iterator it = __resources.find(Thread::myThreadId());
#else
    ResourceMap::iterator it = __resources.find(0);
#endif

    if(it == __resources.end()) {
      error("No OpenGL resources for current thread");
      return 0;
    }

    return (*it).second.m_window;
  }

  bool GLResources::isBrokenProxyTexture2D()
  {
    return m_brokenProxyTexture2D;
  }

  /// add globally removed objects
  void GLResources::eraseOnce()
  {
    const GarbageCollector::container & objs = GarbageCollector::previousObjects();
    for(GarbageCollector::const_iterator it = objs.begin(); it != objs.end(); ++it) {
      eraseResource(*it);
    }
  }
}

