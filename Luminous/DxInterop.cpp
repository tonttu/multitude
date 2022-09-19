/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "DxInterop.hpp"

#include <Radiant/StringUtils.hpp>
#include <Radiant/Trace.hpp>

#include <Windows.h>

template <typename Func>
static void loadProc(Func & f, const char * name, bool & failed)
{
  PROC proc = wglGetProcAddress(name);
  if (!proc) {
    Radiant::error("wglGetProcAddress(\"%s\") failed: %s", name,
                   Radiant::StringUtils::getLastErrorMessage().toUtf8().data());
    failed = true;
  } else {
    f = (Func)proc;
  }
}

namespace Luminous
{
  bool DxInterop::init()
  {
    if (m_initialized)
      return m_supported;

    bool failed = false;
#define LOAD_PROC(name) loadProc(name, #name, failed)

    LOAD_PROC(wglDXSetResourceShareHandleNV);
    LOAD_PROC(wglDXOpenDeviceNV);
    LOAD_PROC(wglDXCloseDeviceNV);
    LOAD_PROC(wglDXRegisterObjectNV);
    LOAD_PROC(wglDXUnregisterObjectNV);
    LOAD_PROC(wglDXObjectAccessNV);
    LOAD_PROC(wglDXLockObjectsNV);
    LOAD_PROC(wglDXUnlockObjectsNV);

    m_initialized = true;
    m_supported = !failed;

    return m_supported;
  }

} // namespace Luminous
