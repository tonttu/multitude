/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "Luminous/StencilMode.hpp"

namespace Luminous
{
  StencilMode::StencilMode()
    : m_frontStencilFail(Keep)
    , m_frontDepthFail(Keep)
    , m_frontPass(Keep)
    , m_frontFunction(Always)
    , m_frontRefValue(0)
    , m_frontMaskValue(0xffffffff)
    , m_backStencilFail(Keep)
    , m_backDepthFail(Keep)
    , m_backPass(Keep)
    , m_backFunction(Always)
    , m_backRefValue(0)
    , m_backMaskValue(0xffffffff)
  {
  }

  void StencilMode::setFunction(Face face, Function function, int ref, unsigned int mask)
  {
    if(face == FRONT || face == FRONT_AND_BACK) {
      m_frontFunction = function;
      m_frontRefValue = ref;
      m_frontMaskValue = mask;
    }

    if(face == BACK || face == FRONT_AND_BACK) {
      m_backFunction = function;
      m_backRefValue = ref;
      m_backMaskValue = mask;
    }
  }

  void StencilMode::setOperation(Face face, Operation stencilFail, Operation depthFail, Operation pass)
  {
    if(face == FRONT || face == FRONT_AND_BACK) {
      m_frontStencilFail = stencilFail;
      m_frontDepthFail = depthFail;
      m_frontPass = pass;
    }

    if(face == BACK || face == FRONT_AND_BACK) {
      m_backStencilFail = stencilFail;
      m_backDepthFail = depthFail;
      m_backPass = pass;
    }
  }

  bool StencilMode::equal(const StencilMode &o) const
  {
    return
        m_frontStencilFail == o.m_frontStencilFail &&
        m_frontDepthFail == o.m_frontDepthFail &&
        m_frontPass == o.m_frontPass &&
        m_frontFunction == o.m_frontFunction &&
        m_frontRefValue == o.m_frontRefValue &&
        m_frontMaskValue == o.m_frontMaskValue &&
        m_backStencilFail == o.m_backStencilFail &&
        m_backDepthFail == o.m_backDepthFail &&
        m_backPass == o.m_backPass &&
        m_backFunction == o.m_backFunction &&
        m_backRefValue == o.m_backRefValue &&
        m_backMaskValue == o.m_backMaskValue;
  }

}
