/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Luminous/RenderResource.hpp"
#include "Luminous/RenderManager.hpp"
#include "Luminous/RenderDriver.hpp"

#include <type_traits>

namespace Luminous
{
  RenderResource::RenderResource(Type type)
    : m_generation(0)
    , m_id(RenderManager::createResource(this))
    , m_type(type)
    , m_expiration(3)   // Default expiration time: 3 seconds
  {
  }

  RenderResource::~RenderResource()
  {
    if(m_id != Id(-1))
      RenderManager::destroyResource( resourceId() );
  }

  RenderResource::RenderResource(RenderResource && rr)
    : m_generation(rr.m_generation)
    , m_id(rr.m_id)
    , m_type(rr.m_type)
    , m_expiration(rr.m_expiration)
  {
    rr.m_id = Id(-1);
    RenderManager::updateResource(m_id, this);
  }

  RenderResource & RenderResource::operator=(RenderResource && rr)
  {
    RenderManager::destroyResource(resourceId());
    m_generation = rr.m_generation;
    m_id = rr.m_id;
    m_type = rr.m_type;
    m_expiration = rr.m_expiration;
    rr.m_id = Id(-1);
    RenderManager::updateResource(m_id, this);
    return *this;
  }

  RenderResource::RenderResource(const RenderResource & rr)
    : m_generation(rr.m_generation)
    , m_id(RenderManager::createResource(this))
    , m_type(rr.m_type)
    , m_expiration(rr.m_expiration)
  {
  }

  RenderResource & RenderResource::operator=(const RenderResource & rr)
  {
    if(this != &rr)
    {
      RenderManager::destroyResource(resourceId());
      m_generation = rr.m_generation;
      m_type = rr.m_type;
      m_expiration = rr.m_expiration;
      m_id = RenderManager::createResource(this);
    }
    return *this;
  }
}
