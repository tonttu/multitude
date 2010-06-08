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

#include "GLContext.hpp"

#include <Radiant/Trace.hpp>

namespace Luminous
{
  using namespace Radiant;

  GLContext::GLContext()
  {}

  GLContext::~GLContext()
  {}

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  GLDummyContext::GLDummyContext()
  {}

  GLDummyContext::~GLDummyContext()
  {}

  void GLDummyContext::makeCurrent()
  {
    error("GLDummyContext::makeCurrent # Method not implemented for this platform");
  }

  GLContext * GLDummyContext::createSharedContext()
  {
    error("GLDummyContext::createSharedContext # Method not implemented for this platform");
    return 0;
  }

  Radiant::Mutex * GLDummyContext::mutex()
  {
    error("GLDummyContext::mutex # Method not implemented for this platform");
    return 0;
  }

}

