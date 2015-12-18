#include "SwapGroups.hpp"

#include <Radiant/Platform.hpp>

namespace Luminous {

#ifdef RADIANT_LINUX

#include <GL/glxew.h>

  bool SwapGroups::isExtensionSupported()
  {
    return glxewIsSupported("GLX_NV_swap_group");
  }

  bool SwapGroups::queryMaxSwapGroup(GLuint& maxGroups, GLuint& maxBarriers)
  {
    auto dpy = glXGetCurrentDisplay();
    auto screen = DefaultScreen(dpy);

    return glXQueryMaxSwapGroupsNV(dpy, screen, &maxGroups, &maxBarriers);
  }

  bool SwapGroups::joinSwapGroup(GLuint group)
  {
    auto dpy = glXGetCurrentDisplay();
    auto drawable = glXGetCurrentDrawable();

    return glXJoinSwapGroupNV(dpy, drawable, group);
  }

  bool SwapGroups::bindSwapBarrier(GLuint group, GLuint barrier)
  {
    auto dpy = glXGetCurrentDisplay();

    return glXBindSwapBarrierNV(dpy, group, barrier);
  }

  bool SwapGroups::querySwapGroup(GLuint& group, GLuint& barrier)
  {
    auto dpy = glXGetCurrentDisplay();
    auto drawable = glXGetCurrentDrawable();

    return glXQuerySwapGroupNV(dpy, drawable, &group, &barrier);
  }

#elif defined(RADIANT_WINDOWS)

#include <GL/wglew.h>

  bool SwapGroups::isExtensionSupported()
  {
    return wglewIsSupported("WGL_NV_swap_group");
  }

  bool SwapGroups::queryMaxSwapGroup(GLuint& maxGroups, GLuint& maxBarriers)
  {
    auto dc = wglGetCurrentDC();

    return wglQueryMaxSwapGroupsNV(dc, &maxGroups, &maxBarriers);
  }

  bool SwapGroups::joinSwapGroup(GLuint group)
  {
    auto dc = wglGetCurrentDC();

    return wglJoinSwapGroupNV(dc, group);
  }

  bool SwapGroups::bindSwapBarrier(GLuint group, GLuint barrier)
  {
    return wglBindSwapBarrierNV(group, barrier);
  }

  bool SwapGroups::querySwapGroup(GLuint& group, GLuint& barrier)
  {
    auto dc = wglGetCurrentDC();

    return wglQuerySwapGroupNV(dc, &group, &barrier);
  }

#else

  bool SwapGroups::isExtensionSupported()
  {
    return false;
  }

  bool SwapGroups::queryMaxSwapGroup(GLuint& maxGroups, GLuint& maxBarriers)
  {
    return false;
  }

  bool SwapGroups::joinSwapGroup(GLuint group)
  {
    return false;
  }

  bool SwapGroups::bindSwapBarrier(GLuint group, GLuint barrier)
  {
    return false;
  }

  bool SwapGroups::querySwapGroup(GLuint& group, GLuint& barrier)
  {
    return false;
  }

#endif

}
