#include "AdaptiveVSync.hpp"

#include <Radiant/Platform.hpp>
#include <Radiant/Trace.hpp>

#ifdef RADIANT_LINUX
#include  <GL/glxew.h>
#endif

namespace Luminous
{

  namespace AdaptiveVSync
  {

    void enable()
    {
#ifdef RADIANT_LINUX
      Display *dpy = glXGetCurrentDisplay();
      GLXDrawable drawable = glXGetCurrentDrawable();
      const int interval = -1;

      Radiant::warning("ENABLING ADAPTIVE VSYNC (display: %p drawable: %ld", dpy, drawable);

      glXSwapIntervalEXT(dpy, drawable, interval);
#endif
    }

  }

}
