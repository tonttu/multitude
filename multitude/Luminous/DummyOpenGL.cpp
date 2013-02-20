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
