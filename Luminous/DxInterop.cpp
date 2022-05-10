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
