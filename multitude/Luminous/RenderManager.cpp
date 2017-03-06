/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "RenderManager.hpp"
#include "RenderDriver.hpp"
#include "ContextArray.hpp"

#include <Radiant/Mutex.hpp>
#include <Radiant/Thread.hpp>
#include <Radiant/Timer.hpp>

#include <map>
#include <set>

namespace Luminous
{
  static RADIANT_TLS(unsigned) t_threadIndex = 0;

  typedef std::map<RenderResource::Id, RenderResource *> ResourceMap;

  namespace {
    ResourceMap s_resourceMap;
    Radiant::Mutex s_resourceMapMutex;

    RenderResource::Id s_resourceId = 1;      // Next available resource ID
    std::vector<Luminous::RenderDriver*> s_drivers; // Currently used drivers
    Radiant::Mutex s_contextArraysMutex(true);
    bool s_contextArraysChanged = false;
    Radiant::Timer s_timer;
    // unit is 0.1 seconds
    int s_frameTime = 0;
    int s_lastFrameTime = 0;
    std::set<ContextArray*> s_contextArrays;
  }

  template <typename T>
  T * getResource( RenderResource::Id id )
  {
    Radiant::Guard g(s_resourceMapMutex);
    auto descr = s_resourceMap.find(id);
    if (descr != std::end(s_resourceMap))
      return reinterpret_cast<T*>(descr->second);
    return nullptr;
  }

  void RenderManager::setDrivers(std::vector<Luminous::RenderDriver*> drivers)
  {
    Radiant::Guard g(s_contextArraysMutex);
    s_drivers = drivers;
    // Resizing context arrays might have a side-effect of adding or removing
    // other context arrays. We will detect this change, not use our invalidated
    // iterators, and just try again.
    do {
      s_contextArraysChanged = false;
      for (auto it = s_contextArrays.begin(), end = s_contextArrays.end(); it != end; ++it) {
        (*it)->resize(s_drivers.size());
        if (s_contextArraysChanged)
          break;
      }
    } while (s_contextArraysChanged);
  }

  RenderResource::Id RenderManager::createResource(RenderResource * resource)
  {
    assert(resource != nullptr);
    Radiant::Guard g(s_resourceMapMutex);
    auto id = s_resourceId++;
    s_resourceMap[id] = resource;
    return id;
  }

  void RenderManager::updateResource(RenderResource::Id id, RenderResource * resource)
  {
    assert(id != 0);
    assert(resource != nullptr);
    Radiant::Guard g(s_resourceMapMutex);
    s_resourceMap[id] = resource;
  }

  void RenderManager::destroyResource(RenderResource::Id id)
  {
    assert(id != 0);
    /* Widgets can be destroyed in any thread, which can trigger this function call
       from any thread. */
    Radiant::Guard g(s_resourceMapMutex);
    s_resourceMap.erase(id);
    auto it = s_drivers.begin(), end = s_drivers.end();
    while(it != end) {
      (*it)->releaseResource(id);
      ++it;
    }
  }

  void RenderManager::addContextArray(ContextArray * contextArray)
  {
    Radiant::Guard g(s_contextArraysMutex);
    s_contextArraysChanged = true;
    s_contextArrays.insert(contextArray);
  }

  void RenderManager::removeContextArray(ContextArray * contextArray)
  {
    Radiant::Guard g(s_contextArraysMutex);
    s_contextArraysChanged = true;
    s_contextArrays.erase(contextArray);
  }

  unsigned int RenderManager::driverCount()
  {
    return s_drivers.size();
  }

  int RenderManager::frameTime()
  {
    return s_frameTime;
  }

  int RenderManager::lastFrameTime()
  {
    return s_lastFrameTime;
  }

  void RenderManager::updateFrameTime()
  {
    s_lastFrameTime = s_frameTime;
    s_frameTime = s_timer.time() * 10;
  }

  void RenderManager::setThreadIndex(unsigned idx)
  {
    t_threadIndex = idx;
  }

  unsigned RenderManager::threadIndex()
  {
    return t_threadIndex;
  }

  Radiant::Mutex & RenderManager::resourceLock()
  {
    return s_resourceMapMutex;
  }

  // Only specialize for valid types
  template <> LUMINOUS_API Buffer * RenderManager::getResource( RenderResource::Id id ) { return Luminous::getResource<Buffer>(id); }
  template <> LUMINOUS_API VertexArray * RenderManager::getResource( RenderResource::Id id ) { return Luminous::getResource<VertexArray>(id); }
  template <> LUMINOUS_API VertexDescription * RenderManager::getResource( RenderResource::Id id ) { return Luminous::getResource<VertexDescription>(id); }
  template <> LUMINOUS_API Texture * RenderManager::getResource( RenderResource::Id id ) { return Luminous::getResource<Texture>(id); }
  template <> LUMINOUS_API Program * RenderManager::getResource( RenderResource::Id id ) { return Luminous::getResource<Program>(id); }
  template <> LUMINOUS_API Shader * RenderManager::getResource( RenderResource::Id id ) { return Luminous::getResource<Shader>(id); }
  template <> LUMINOUS_API RenderBuffer * RenderManager::getResource( RenderResource::Id id ) { return Luminous::getResource<RenderBuffer>(id); }
  template <> LUMINOUS_API FrameBuffer * RenderManager::getResource( RenderResource::Id id ) { return Luminous::getResource<FrameBuffer>(id); }
}
