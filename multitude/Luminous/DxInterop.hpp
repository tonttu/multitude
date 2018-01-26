#ifndef LUMINOUS_DXINTEROP_HPP
#define LUMINOUS_DXINTEROP_HPP

#include "Export.hpp"

#include <Windows.h>

/// From https://www.khronos.org/registry/OpenGL/api/GL/wgl.h
#ifndef WGL_NV_DX_interop
#define WGL_NV_DX_interop 1
#define WGL_ACCESS_READ_ONLY_NV           0x00000000
#define WGL_ACCESS_READ_WRITE_NV          0x00000001
#define WGL_ACCESS_WRITE_DISCARD_NV       0x00000002
#endif /* WGL_NV_DX_interop */

typedef unsigned int GLenum;
typedef int GLint;
typedef unsigned int GLuint;

namespace Luminous
{
  /// Wrapper for WGL_NV_DX_interop
  /// See https://www.khronos.org/registry/OpenGL/extensions/NV/WGL_NV_DX_interop.txt
  /// and https://www.khronos.org/registry/OpenGL/extensions/NV/WGL_NV_DX_interop2.txt
  class LUMINOUS_API DxInterop
  {
  public:
    /// Call in the render thread with active OpenGL context.
    /// Can be called several times.
    /// @returns false if the extension is not supported
    bool init();

    BOOL (WINAPI * wglDXSetResourceShareHandleNV)(void *dxObject, HANDLE shareHandle) = nullptr;
    HANDLE (WINAPI * wglDXOpenDeviceNV)(void *dxDevice) = nullptr;
    BOOL (WINAPI * wglDXCloseDeviceNV)(HANDLE hDevice) = nullptr;
    HANDLE (WINAPI * wglDXRegisterObjectNV)(HANDLE hDevice, void *dxObject, GLuint name, GLenum type, GLenum access) = nullptr;
    BOOL (WINAPI * wglDXUnregisterObjectNV)(HANDLE hDevice, HANDLE hObject) = nullptr;
    BOOL (WINAPI * wglDXObjectAccessNV)(HANDLE hObject, GLenum access) = nullptr;
    BOOL (WINAPI * wglDXLockObjectsNV)(HANDLE hDevice, GLint count, HANDLE *hObjects) = nullptr;
    BOOL (WINAPI * wglDXUnlockObjectsNV)(HANDLE hDevice, GLint count, HANDLE *hObjects) = nullptr;

  private:
    bool m_initialized = false;
    bool m_supported = false;
  };
} // namespace Luminous

#endif // DXINTEROP_HPP
