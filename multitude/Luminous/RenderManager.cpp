/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Finland, http://multitouch.fi
 *
 * All rights reserved, 2007-2010
 *
 * You may use this file only for purposes for which you have a
 * specific, written permission from MultiTouch Oy.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 */

#include "RenderManager.hpp"
#include "RenderDriver.hpp"
#include "HardwareBuffer.hpp"
#include "ShaderProgram.hpp"
#include "Texture2.hpp"

namespace Luminous
{
  class RenderManager::D {
  public:
    D(Luminous::RenderDriver & driver)
      : driver(driver)
      , resourceId(0)
    {
    }

    RenderResource::Id resourceId;
    Luminous::RenderDriver & driver;
    static RenderManager * s_instance;
  };

  RenderManager * RenderManager::D::s_instance = 0;

  RenderManager::RenderManager(Luminous::RenderDriver & driver)
    : m_d(new RenderManager::D(driver))
  {
    assert(RenderManager::D::s_instance == 0);
    RenderManager::D::s_instance = this;
  }

  RenderManager::~RenderManager()
  {
    assert(RenderManager::D::s_instance != 0);
    delete m_d;

    RenderManager::D::s_instance = 0;
  }

  RenderResource::Id RenderManager::createResourceId()
  {
    return m_d->resourceId++;
  }

  RenderDriver & RenderManager::driver()
  {
    return m_d->driver;
  }

  RenderManager & RenderManager::instance()
  {
    assert(RenderManager::D::s_instance != 0);
    return *RenderManager::D::s_instance;
  }  
}
