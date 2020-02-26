#pragma once

#include "Texture.hpp"

#include <Nimble/Size.hpp>

namespace Luminous
{
  class RenderContext;

  /// D3D11.1 shared NT HANDLE texture. It is used to share a texture data with
  /// another process, like CEF. Synchronization is done with IDXGIKeyedMutex
  /// interface. init/acquire will call AcquireSync(1) and release will eventually
  /// call ReleaseSync(0).
  ///
  /// Both acquire and release need to be called every time after the other process
  /// does matching calls, otherwise the other process will end up in deadlock.
  ///
  /// You most likely want to use DxSharedTextureBag instead of DxSharedTexture
  /// directly.
  ///
  /// Requires:
  ///  * Windows 10 (for CompareObjectHandles)
  ///  * DirectX 11.1
  ///  * CUDA 9.0 (nvidia driver 384.81 or newer) for copying data between GPUs
  ///  * WGL_NV_DX_interop OpenGL extension for using the texture in OpenGL
  class LUMINOUS_API DxSharedTexture : public std::enable_shared_from_this<DxSharedTexture>
  {
  public:
    /// Creates a new DxSharedTexture that wraps the given D3D NT HANDLE
    /// shared texture. Returns nullptr if something fails. Calls AcquireSync(1).
    static std::shared_ptr<DxSharedTexture> create(void * sharedHandle);

    /// Destroys the texture and schedules all reserved OpenGL resources to be
    /// deleted in their own respective rendering threads.
    ~DxSharedTexture();

    /// The original texture has been updated. Calls AcquireSync(1).
    void acquire();

    /// Schedules ReleaseSync(0) to be done immediately after the texture is
    /// not being used by anyone. If the texture is not in use, will call
    /// ReleaseSync(0) immediately.
    bool release();

    /// Copy of the original handle given in init()
    void * sharedHandle() const;

    /// Timestamp when texture() was last called
    Radiant::TimeStamp lastUsed() const;

    /// Size of the shared texture, can't change during the lifetime of this object.
    Nimble::SizeI size() const;

    /// Returns true if the texture is ready on the given render thread. Also
    /// finalizes finished copy operations.
    bool checkStatus(unsigned int renderThreadIndex);

    /// @see DxSharedTextureBag::texture
    /// @param copyIfNeeded if the texture is not available on this GPU, and
    ///        it's not already being copied, a new copy operation is started
    ///        if this is true.
    const Luminous::Texture * texture(RenderContext & r, bool copyIfNeeded);

  private:
    DxSharedTexture();

  private:
    class D;
    std::unique_ptr<D> m_d;
  };

  /// Handles a pool of DxSharedTexture objects. Meant for doing multiple
  /// buffering with the other application. For instance CEF uses something
  /// between 3-5 textures per browser to avoid stalling the rendering pipeline.
  ///
  /// In order to properly release resources, clean needs to be called when
  /// nobody is using the textures. Now it's called automatically from
  /// ThreadedGfxDriver::render.
  class LUMINOUS_API DxSharedTextureBag
  {
  public:
    DxSharedTextureBag();
    ~DxSharedTextureBag();

    /// Add a new shared handle to the container, or inform that existing
    /// resource is updated and can be acquired.
    bool addSharedHandle(void * sharedHandle);

    /// Returns the latest texture for this rendering thread. Might start
    /// asynchronous copy operation between GPUs, if the latest texture
    /// is not available on this GPU.
    const Luminous::Texture * texture(RenderContext & r);

    /// Finishes pending asynchronous copy operations and deletes unused textures.
    static void clean();

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}
