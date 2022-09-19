/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "SwapGroups.hpp"

#include <Radiant/Platform.hpp>

#ifdef RADIANT_LINUX
#include <GL/glx.h>
#elif defined(RADIANT_WINDOWS)
#include <Windows.h>
#endif

namespace Luminous
{
#ifdef RADIANT_LINUX
  class SwapGroups::D
  {
  public:
    using GlXQueryMaxSwapGroupsNV = Bool (*)(Display * dpy, int screen, GLuint * maxGroups, GLuint * maxBarriers);
    using GlXJoinSwapGroupNV = Bool (*)(Display * dpy, GLXDrawable drawable, GLuint group);
    using GlXBindSwapBarrierNV = Bool (*)(Display * dpy, GLuint group, GLuint barrier);
    using GlXQuerySwapGroupNV = Bool (*)(Display * dpy, GLXDrawable drawable, GLuint * group, GLuint * barrier);
    using GlXQueryFrameCountNV = Bool (*)(Display * dpy, int screen, GLuint * count);
    using GlXResetFrameCountNV = Bool (*)(Display * dpy, int screen);

    GlXQueryMaxSwapGroupsNV glXQueryMaxSwapGroupsNV = nullptr;
    GlXJoinSwapGroupNV glXJoinSwapGroupNV = nullptr;
    GlXBindSwapBarrierNV glXBindSwapBarrierNV = nullptr;
    GlXQuerySwapGroupNV glXQuerySwapGroupNV = nullptr;
    GlXQueryFrameCountNV glXQueryFrameCountNV = nullptr;
    GlXResetFrameCountNV glXResetFrameCountNV = nullptr;

    Display * m_display = glXGetCurrentDisplay();
    int m_screen = 0;
  };

  SwapGroups::SwapGroups(int screen)
    : m_d(new D())
  {
    m_d->glXQueryMaxSwapGroupsNV = (D::GlXQueryMaxSwapGroupsNV)glXGetProcAddress((GLubyte*)"glXQueryMaxSwapGroupsNV");
    m_d->glXJoinSwapGroupNV = (D::GlXJoinSwapGroupNV)glXGetProcAddress((GLubyte*)"glXJoinSwapGroupNV");
    m_d->glXBindSwapBarrierNV = (D::GlXBindSwapBarrierNV)glXGetProcAddress((GLubyte*)"glXBindSwapBarrierNV");
    m_d->glXQuerySwapGroupNV = (D::GlXQuerySwapGroupNV)glXGetProcAddress((GLubyte*)"glXQuerySwapGroupNV");
    m_d->glXQueryFrameCountNV = (D::GlXQueryFrameCountNV)glXGetProcAddress((GLubyte*)"glXQueryFrameCountNV");
    m_d->glXResetFrameCountNV = (D::GlXResetFrameCountNV)glXGetProcAddress((GLubyte*)"glXResetFrameCountNV");
    m_d->m_screen = screen;
  }

  bool SwapGroups::isExtensionSupported() const
  {
    return m_d->glXJoinSwapGroupNV != nullptr;
  }

  bool SwapGroups::queryMaxSwapGroup(GLuint & maxGroups, GLuint & maxBarriers) const
  {
    return m_d->glXQueryMaxSwapGroupsNV(m_d->m_display, m_d->m_screen, &maxGroups, &maxBarriers);
  }

  bool SwapGroups::joinSwapGroup(GLuint group) const
  {
    auto drawable = glXGetCurrentDrawable();
    return m_d->glXJoinSwapGroupNV(m_d->m_display, drawable, group);
  }

  bool SwapGroups::bindSwapBarrier(GLuint group, GLuint barrier) const
  {
    return m_d->glXBindSwapBarrierNV(m_d->m_display, group, barrier);
  }

  bool SwapGroups::querySwapGroup(GLuint& group, GLuint & barrier) const
  {
    auto drawable = glXGetCurrentDrawable();
    return m_d->glXQuerySwapGroupNV(m_d->m_display, drawable, &group, &barrier);
  }

  bool SwapGroups::queryFrameCount(GLuint & count) const
  {
    return m_d->glXQueryFrameCountNV(m_d->m_display, m_d->m_screen, &count);
  }

  bool SwapGroups::resetFrameCount() const
  {
    return m_d->glXResetFrameCountNV(m_d->m_display, m_d->m_screen);
  }

#elif defined(RADIANT_WINDOWS)

  class SwapGroups::D
  {
  public:
    using WglQueryMaxSwapGroupsNV = BOOL (*)(HDC hDC, GLuint * maxGroups, GLuint * maxBarriers);
    using WglJoinSwapGroupNV = BOOL (*)(HDC hDC, GLuint group);
    using WglBindSwapBarrierNV = BOOL (*)(GLuint group, GLuint barrier);
    using WglQuerySwapGroupNV = BOOL (*)(HDC hDC, GLuint * group, GLuint * barrier);
    using WglQueryFrameCountNV = BOOL (*)(HDC hDC, GLuint * count);
    using WglResetFrameCountNV = BOOL (*)(HDC hDC);

    WglQueryMaxSwapGroupsNV wglQueryMaxSwapGroupsNV = nullptr;
    WglJoinSwapGroupNV wglJoinSwapGroupNV = nullptr;
    WglBindSwapBarrierNV wglBindSwapBarrierNV = nullptr;
    WglQuerySwapGroupNV wglQuerySwapGroupNV = nullptr;
    WglQueryFrameCountNV wglQueryFrameCountNV = nullptr;
    WglResetFrameCountNV wglResetFrameCountNV = nullptr;
  };

  SwapGroups::SwapGroups(int /*screen*/)
    : m_d(new D())
  {
    m_d->wglQueryMaxSwapGroupsNV = (D::WglQueryMaxSwapGroupsNV)wglGetProcAddress("wglQueryMaxSwapGroupsNV");
    m_d->wglJoinSwapGroupNV = (D::WglJoinSwapGroupNV)wglGetProcAddress("wglJoinSwapGroupNV");
    m_d->wglBindSwapBarrierNV = (D::WglBindSwapBarrierNV)wglGetProcAddress("wglBindSwapBarrierNV");
    m_d->wglQuerySwapGroupNV = (D::WglQuerySwapGroupNV)wglGetProcAddress("wglQuerySwapGroupNV");
    m_d->wglQueryFrameCountNV = (D::WglQueryFrameCountNV)wglGetProcAddress("wglQueryFrameCountNV");
    m_d->wglResetFrameCountNV = (D::WglResetFrameCountNV)wglGetProcAddress("wglResetFrameCountNV");
  }

  bool SwapGroups::isExtensionSupported() const
  {
    return m_d->wglJoinSwapGroupNV != nullptr;
  }

  bool SwapGroups::queryMaxSwapGroup(GLuint & maxGroups, GLuint & maxBarriers) const
  {
    auto dc = wglGetCurrentDC();
    return m_d->wglQueryMaxSwapGroupsNV(dc, &maxGroups, &maxBarriers);
  }

  bool SwapGroups::joinSwapGroup(GLuint group) const
  {
    auto dc = wglGetCurrentDC();
    return m_d->wglJoinSwapGroupNV(dc, group);
  }

  bool SwapGroups::bindSwapBarrier(GLuint group, GLuint barrier) const
  {
    return m_d->wglBindSwapBarrierNV(group, barrier);
  }

  bool SwapGroups::querySwapGroup(GLuint & group, GLuint & barrier) const
  {
    auto dc = wglGetCurrentDC();
    return m_d->wglQuerySwapGroupNV(dc, &group, &barrier);
  }

  bool SwapGroups::queryFrameCount(GLuint & count) const
  {
    auto dc = wglGetCurrentDC();
    return m_d->wglQueryFrameCountNV(dc, &count);
  }

  bool SwapGroups::resetFrameCount() const
  {
    auto dc = wglGetCurrentDC();
    return m_d->wglResetFrameCountNV(dc);
  }

#else
  class SwapGroups::D
  {};

  SwapGroups::SwapGroups(int /*screen*/)
  {}

  bool SwapGroups::isExtensionSupported() const
  {
    return false;
  }

  bool SwapGroups::queryMaxSwapGroup(GLuint & /*maxGroups*/, GLuint & /*maxBarriers*/) const
  {
    return false;
  }

  bool SwapGroups::joinSwapGroup(GLuint /*group*/) const
  {
    return false;
  }

  bool SwapGroups::bindSwapBarrier(GLuint /*group*/, GLuint /*barrier*/) const
  {
    return false;
  }

  bool SwapGroups::querySwapGroup(GLuint& /*group*/, GLuint& /*barrier*/) const
  {
    return false;
  }

  bool SwapGroups::queryFrameCount(GLuint & /*count*/) const
  {
    return false;
  }

  bool SwapGroups::resetFrameCount() const
  {
    return false;
  }

#endif

  SwapGroups::~SwapGroups()
  {}
}
