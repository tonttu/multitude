/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others, 2007-2013
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "DummyOpenGL.hpp"

#include <Radiant/Trace.hpp>

namespace Luminous
{

  void dumymWarn(const char * funcname, const char * file, int line)
  {
    Radiant::error("Unimplemented OpenGL call: %s in %s:%d", funcname, file, line);
  }
  int dummyEnum(const char * file, int line)
  {
    Radiant::error("Unimplemented OpenGL call: %s:%d", file, line);
    return 0;
  }

}
