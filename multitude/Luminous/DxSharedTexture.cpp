#include "ContextArray.hpp"
#include "DxInterop.hpp"
#include "DxSharedTexture.hpp"
#include "RenderContext.hpp"
#include "ResourceHandleGL.hpp"
#include "GfxDriver.hpp"
#include "RenderDriverGL.hpp"

#include <Radiant/Mutex.hpp>
#include <Radiant/Task.hpp>
#include <Radiant/StringUtils.hpp>

#include <folly/futures/Future.h>
#include <folly/executors/ManualExecutor.h>

#include <QReadWriteLock>

#include <dxgi.h>
#include <d3d11_3.h>

#include <comdef.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

bool operator==(const LUID & l, const LUID & r)
{
  return l.LowPart == r.LowPart && l.HighPart == r.HighPart;
}

namespace
{
  Radiant::Mutex s_allDxTextureBagsMutex;
  std::unordered_set<Luminous::DxSharedTextureBag*> s_allDxTextureBags;

// This is for debugging synchronization with CEF
#if 0
  Radiant::Mutex s_acquireStatsMutex;
  std::map<void*, int> s_acquireStats;

  void updateAcquireStats(void * ptr, int diff)
  {
    Radiant::Guard g(s_acquireStatsMutex);
    s_acquireStats[ptr] += diff;
    QStringList stats;
    for (auto & p: s_acquireStats)
      if (p.second)
        stats << QString("%1: %2").arg((qlonglong)p.first, 0, 16).arg(p.second);
    Radiant::info("Dx acquire stats: %s", stats.join(", ").toUtf8().data());
  }
#else
  void updateAcquireStats(void *, int) {}
#endif

  QByteArray comErrorStr(HRESULT res)
  {
    return QString::fromStdWString(_com_error(res).ErrorMessage()).toUtf8();
  }

  /// Creates a device from the same adapter that owns sharedHandle
  ComPtr<ID3D11Device3> createDevice(HANDLE sharedHandle)
  {
    ComPtr<IDXGIFactory2> dxgiFactory;

    HRESULT res = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(res)) {
      Radiant::error("DxSharedTexture # CreateDXGIFactory1 failed: %s",
                     comErrorStr(res).data());
      return nullptr;
    }

    LUID adapterLuid{};
    res = dxgiFactory->GetSharedResourceAdapterLuid(sharedHandle, &adapterLuid);
    if (FAILED(res)) {
      Radiant::error("DxSharedTexture # GetSharedResourceAdapterLuid failed: %s",
                     comErrorStr(res).data());
      return nullptr;
    }

    IDXGIAdapter * it = nullptr;
    for (UINT i = 0; dxgiFactory->EnumAdapters(i, &it) != DXGI_ERROR_NOT_FOUND; ++i) {
      DXGI_ADAPTER_DESC desc;
      if (SUCCEEDED(it->GetDesc(&desc))) {
        if (desc.AdapterLuid == adapterLuid)
          break;
      }
      it->Release();
      it = nullptr;
    }

    if (!it) {
      Radiant::error("DxSharedTexture # Couldn't find the correct DXGIAdapter "
                     "for the shared texture");
      return nullptr;
    }

    ComPtr<ID3D11Device> dev;
    UINT flags = 0;
#ifdef RADIANT_DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    /// CEF and DxSharedTexture use D3D11_RESOURCE_MISC_SHARED_NTHANDLE type
    /// shared textures that require D3D 11.1
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1 };
    res = D3D11CreateDevice(it, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                            flags, featureLevels, 1,
                            D3D11_SDK_VERSION, &dev, nullptr, nullptr);
    it->Release();
    if (FAILED(res)) {
      Radiant::error("DxSharedTexture # D3D11CreateDevice failed: %s",
                     comErrorStr(res).data());
      return nullptr;
    }

    ComPtr<ID3D11Device3> dev3;
    res = dev->QueryInterface(IID_PPV_ARGS(&dev3));
    if (FAILED(res)) {
      Radiant::error("DxSharedTexture # QueryInterface ID3D11Device3 failed: %s",
                     comErrorStr(res).data());
      return nullptr;
    }

    return dev3;
  }
}

namespace Luminous
{
  /// Defines how this thread can access the shared texture
  enum ThreadAccess
  {
    /// Initial value, we don't know yet
    ACCESS_UNKNOWN,
    /// We can access the texture directly using DX-OGL interop API, either
    /// the D3D texture is on the same GPU as the OpenGL context, or there
    /// is Mosaic or similar system copying data.
    ACCESS_DX,
    /// This thread can't access the shared texture, we need to copy the data
    ACCESS_COPY,
  };

  /// Per-render context data
  struct Context
  {
    /// DX-GL interop stuff. These can be valid if this GPU is the same as the
    /// adapter associated with D::m_dev.
    Luminous::DxInterop dxInteropApi;
    HANDLE interopDev = nullptr;
    HANDLE interopTex = nullptr;
    /// GL stuff needs to run in a specific thread. This is an executor for
    /// the correct render thread. Used to delete GL resources in ~Context()
    folly::Executor * glExecutor = nullptr;
    /// Number of users rendering interopTex at the moment. If this is greater
    /// than zero, the texture is locked with wglDXLockObjectsNV
    int glRefs = 0;

    RenderDriverGL * renderDriver = nullptr;
    /// Matching GL texture for D::m_tex. If this is non-null, we have called
    /// glTex->ref() to keep it alive.
    TextureGL * glTex = nullptr;
    /// Status of ongoing DX / OGL copy tasks
    /// Protected with D::m_refMutex
    bool copying = false;
    /// Frame number of glTex
    /// Protected with D::m_refMutex
    uint64_t copyFrameNum = 0;
    /// OpenGL sync object for glTex after copying
    GLsync copyFence = nullptr;

    /// Set to true if DX-GL interop or something else failed for this GPU.
    /// Invalidates the whole DxSharedTexture object for this context and
    /// there is no recovery. Practically all reasons for any errors are
    /// something that would happen again later, so this practically just
    /// stops flooding the same error message every frame.
    std::atomic<bool> failed{false};

    ThreadAccess access = ACCESS_UNKNOWN;

    Context() = default;
    Context(Context && c)
    {
      *this = std::move(c);
    }

    Context & operator=(Context && c)
    {
      std::swap(dxInteropApi, c.dxInteropApi);
      std::swap(interopDev, c.interopDev);
      std::swap(interopTex, c.interopTex);
      std::swap(glExecutor, c.glExecutor);
      std::swap(glRefs, c.glRefs);
      std::swap(glTex, c.glTex);
      std::swap(copying, c.copying);
      std::swap(copyFrameNum, c.copyFrameNum);
      std::swap(copyFence, c.copyFence);
      std::swap(access, c.access);
      bool f = failed;
      failed = c.failed.load();
      c.failed = f;
      return *this;
    }

    Context(const Context &) = delete;
    Context & operator=(const Context &) = delete;

    ~Context();
  };

  class DxSharedTexture::D
  {
  public:
    D(DxSharedTexture & host)
      : m_host(host)
    {}

    void startCopy(Context & ctx);
    void finishCopy(Context & ctx, UploadBufferRef * buffer);

    bool ref(Context & ctx);
    void unref(Context & ctx);

    // Returns false if force was false and the texture is still in use
    // m_refMutex needs to be locked while calling this function
    bool release(bool force);

  public:
    DxSharedTexture & m_host;

    /// Lock this when using m_dev or any interop functions in GL
    /// associated with m_dev.
    Radiant::Mutex m_devMutex;
    /// Protects ctx.copying, copyFrameNum, m_acquired, m_release, m_refs
    Radiant::Mutex m_refMutex;
    /// The device that owns the shared texture.
    ComPtr<ID3D11Device3> m_dev;
    /// Copy of the shared handle received from DX application
    std::shared_ptr<void> m_sharedHandle;
    /// wglDXSetResourceShareHandleNV needs to be called exactly once, but
    /// from a correct OpenGL context. This makes sure we don't call it twice.
    bool m_shareHandleSet = false;
    /// Texture from m_sharedHandle
    ComPtr<ID3D11Texture2D> m_dxTex;
    /// Lock for m_dxTex. We need to do AcquireSync(1) when we want to use the
    /// texture, and ReleaseSync(0) when we are done with it.
    ComPtr<IDXGIKeyedMutex> m_lock;
    /// Set to true if we have called AcquireSync(1).
    bool m_acquired = false;

    /// First frame is 1, increased every time acquire is called.
    std::atomic<uint64_t> m_frameNum{0};

    /// We need to make a copy of m_dxTex on the same GPU before being able to
    /// map it.
    ComPtr<ID3D11Texture2D> m_copy;
    /// Frame number of m_copy (compare to m_frameNumber)
    std::atomic<uint64_t> m_dxCopyFrameNum{0};

    /// Texture with the correct size and pixel format, but empty data pointer.
    /// Some of the TextureGL instances might be using DX interop extension and
    /// others just standalone OpenGL texture.
    Luminous::Texture m_tex;

    /// Per-render thread data
    ContextArrayT<Context> m_ctx;

    /// Nonzero if m_copy is mapped. Protected by m_devMutex.
    int m_copyRef = 0;
    /// m_copy mapped, null if m_copyRef is zero.
    D3D11_MAPPED_SUBRESOURCE m_copyMapped{};
    /// Used to synchronize m_dxTex -> m_copy task so that we don't need to
    /// wait with m_devMutex locked.
    std::shared_ptr<void> m_copyEvent;

    /// Number of m_dxTex users (render operations in the same context or copy
    /// operation to m_copy). When this goes back to zero, and m_release is true,
    /// release() and eventually ReleaseSync(0) is called.
    int m_refs = 0;
    /// This is set to true by DxSharedTexture::release, meaning that we are
    /// ready to call ReleaseSync(0) once nobody is using the texture anymore.
    bool m_release = false;

    /// Updated every time texture() is called
    std::atomic<Radiant::TimeStamp::type> m_lastUsed{
      Radiant::TimeStamp::currentTime().value()};
  };

  /////////////////////////////////////////////////////////////////////////////


  Context::~Context()
  {
    if (glExecutor && (glTex || (renderDriver && copyFence))) {
      glExecutor->add([glTex=glTex, copyFence=copyFence, renderDriver=renderDriver] {
        if (copyFence && renderDriver)
          renderDriver->opengl().glDeleteSync(copyFence);
        if (glTex)
          glTex->unref();
      });
    }
    if (glExecutor && interopTex) {
      glExecutor->add([interopDev=interopDev, interopTex=interopTex,
                      dxInteropApi=dxInteropApi] {
        if (!dxInteropApi.wglDXUnregisterObjectNV(interopDev, interopTex)) {
          GLERROR("wglDXUnregisterObjectNV");
          Radiant::error("DxSharedTexture # wglDXUnregisterObjectNV failed");
        }

        if (!dxInteropApi.wglDXCloseDeviceNV(interopDev)) {
          GLERROR("wglDXCloseDeviceNV");
          Radiant::error("DxSharedTexture # wglDXCloseDeviceNV failed");
        }
      });
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  /// Copy D3D texture 'm_dxTex' to OpenGL texture 'handle' on a different GPU.
  ///  * First make a copy of m_dxTex to a temporary m_copy DX texture on the
  ///    source GPU since the DX interop texture can't be mapped.
  ///  * Map m_copy
  ///  * Allocate memory for handle on the destination GPU using OpenGL
  ///  * Upload mapped m_copy to the target texture using a temporary upload buffer

  /// This is executed in a random bg thread
  void DxSharedTexture::D::startCopy(Context & ctx)
  {
    ComPtr<ID3D11DeviceContext3> deviceCtx;

    {
      Radiant::Guard g(m_devMutex);
      m_dev->GetImmediateContext3(&deviceCtx);

      if (!m_copy) {
        D3D11_TEXTURE2D_DESC desc;
        m_dxTex->GetDesc(&desc);
        desc.BindFlags = 0;
        desc.MiscFlags = 0;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        HRESULT res = m_dev->CreateTexture2D(&desc, nullptr, &m_copy);
        if (FAILED(res)) {
          Radiant::error("DxSharedTexture # CreateTexture2D failed: %s", comErrorStr(res).data());
          ctx.copying = false;
          unref(ctx);
          return;
        }
      }

      if (!m_copyEvent) {
        std::shared_ptr<void> copyEvent(CreateEventA(nullptr, true, false, nullptr), &CloseHandle);
        m_copyEvent = std::move(copyEvent);
      }

      if (m_dxCopyFrameNum < m_frameNum) {
        deviceCtx->CopyResource(m_copy.Get(), m_dxTex.Get());
        m_dxCopyFrameNum = m_frameNum.load();

        ResetEvent(m_copyEvent.get());
        deviceCtx->Flush1(D3D11_CONTEXT_TYPE_COPY, m_copyEvent.get());
      }
      ctx.copyFrameNum = m_frameNum.load();
    }

    // We don't need the original texture anymore
    unref(ctx);

    WaitForSingleObject(m_copyEvent.get(), INFINITE);

    {
      Radiant::Guard g(m_devMutex);
      if (++m_copyRef == 1) {
        HRESULT res = deviceCtx->Map(m_copy.Get(), 0, D3D11_MAP_READ, 0, &m_copyMapped);
        if (FAILED(res)) {
          Radiant::error("DxSharedTexture # Map failed: %s", comErrorStr(res).data());
          --m_copyRef;
          ctx.failed = true;
          return;
        }
      }
    }

    auto self = m_host.shared_from_this();
    ctx.renderDriver->worker().add([self, this, &ctx, deviceCtx] {
      UploadBufferRef * buffer = new UploadBufferRef(ctx.renderDriver->uploadBuffer(
                                                       m_copyMapped.RowPitch * m_tex.height()));
      Radiant::SingleShotTask::run([self, this, &ctx, buffer, deviceCtx] {
        memcpy(buffer->persistentMapping(), m_copyMapped.pData, m_copyMapped.RowPitch * m_tex.height());
        {
          Radiant::Guard g(m_devMutex);
          if (--m_copyRef == 0)
            deviceCtx->Unmap(m_copy.Get(), 0);
        }
        ctx.renderDriver->worker().add([self, this, &ctx, buffer] () mutable {
          finishCopy(ctx, buffer);
        });
      });
    });
  }

  void DxSharedTexture::D::finishCopy(Context & ctx, UploadBufferRef * buffer)
  {
    // Set proper alignment
    int alignment = 8;
    while (m_copyMapped.RowPitch % alignment)
      alignment >>= 1;

    OpenGLAPI & gl = ctx.renderDriver->opengl();
    gl.glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    gl.glPixelStorei(GL_UNPACK_ROW_LENGTH, m_copyMapped.RowPitch / 4);

    (*buffer)->bind(Buffer::UNPACK);

    gl.glBindTexture(GL_TEXTURE_2D, ctx.glTex->handle());
    gl.glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_tex.width(), m_tex.height(),
                       m_tex.dataFormat().layout(), m_tex.dataFormat().type(),
                       nullptr);

    delete buffer;

    gl.glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    auto fence = gl.glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);

    Radiant::Guard g(m_refMutex);
    ctx.copyFence = fence;
  }

  bool DxSharedTexture::D::ref(Context & ctx)
  {
    {
      Radiant::Guard g(m_refMutex);
      if (!m_acquired)
        return false;
      ++m_refs;
    }

    if (ctx.interopTex && ++ctx.glRefs == 1) {
      Radiant::Guard g(m_devMutex);
      if (!ctx.dxInteropApi.wglDXLockObjectsNV(ctx.interopDev, 1, &ctx.interopTex)) {
        GLERROR("wglDXLockObjectsNV");
        Radiant::error("DxSharedTexture # wglDXLockObjectsNV failed");
      }
    }
    return true;
  }

  void DxSharedTexture::D::unref(Context & ctx)
  {
    if (ctx.interopTex && --ctx.glRefs == 0) {
      Radiant::Guard g(m_devMutex);
      if (!ctx.dxInteropApi.wglDXUnlockObjectsNV(ctx.interopDev, 1, &ctx.interopTex)) {
        GLERROR("wglDXUnlockObjectsNV");
        Radiant::error("DxSharedTexture # wglDXUnlockObjectsNV failed");
      }
    }

    Radiant::Guard g(m_refMutex);
    if (--m_refs > 0 || !m_release)
      return;

    release(false);
  }

  bool DxSharedTexture::D::release(bool force)
  {
    if (!m_lock || !m_acquired)
      return true;

    if (!force && m_refs > 0) {
      // The texture is still used by someone (rendering or copying in progress).
      // Once that finishes, release the texture automatically.
      m_release = true;
      return false;
    }

    m_acquired = false;
    updateAcquireStats(m_sharedHandle.get(), -1);
    HRESULT res = m_lock->ReleaseSync(0);
    if (res != 0) {
      Radiant::error("DxSharedTexture # ReleaseSync failed: %s [0x%x]",
                     comErrorStr(res).data(), res);
    }
    m_release = false;
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////

  DxSharedTexture::DxSharedTexture()
    : m_d(new D(*this))
  {
    m_d->m_tex.setExpiration(0);
  }

  std::shared_ptr<DxSharedTexture> DxSharedTexture::create(void * sharedHandle)
  {
    assert(sharedHandle);

    ComPtr<ID3D11Device3> dev = createDevice(sharedHandle);
    if (!dev)
      return nullptr;

    // We can't keep a copy of sharedHandle without duplicating it
    HANDLE copy = nullptr;
    HANDLE currentProcessHandle = GetCurrentProcess();
    if (!DuplicateHandle(currentProcessHandle, sharedHandle,
                         currentProcessHandle, &copy,
                         0, FALSE, DUPLICATE_SAME_ACCESS)) {
      Radiant::error("DxSharedTexture # DuplicateHandle failed: %s",
                     Radiant::StringUtils::getLastErrorMessage().toUtf8().data());
      return nullptr;
    }
    std::shared_ptr<void> copyPtr(copy, &CloseHandle);

    ComPtr<ID3D11Texture2D> dxTex;
    HRESULT res = dev->OpenSharedResource1(copy, IID_PPV_ARGS(&dxTex));
    if (FAILED(res)) {
      Radiant::error("DxSharedTexture # OpenSharedResource1 failed: %s",
                     comErrorStr(res).data());
      return nullptr;
    }

    ComPtr<IDXGIKeyedMutex> lock;
    res = dxTex->QueryInterface(IID_PPV_ARGS(&lock));
    if (FAILED(res)) {
      Radiant::error("DxSharedTexture # QueryInterface IDXGIKeyedMutex failed: %s",
                     comErrorStr(res).data());
      return nullptr;
    } else {
      updateAcquireStats(copyPtr.get(), 1);
      res = lock->AcquireSync(1, INFINITE);
      if (res != 0) {
        Radiant::error("DxSharedTexture # AcquireSync failed: %s [0x%x]",
                       comErrorStr(res).data(), res);
        return nullptr;
      }
    }

    D3D11_TEXTURE2D_DESC desc;
    dxTex->GetDesc(&desc);

    std::shared_ptr<DxSharedTexture> self(new DxSharedTexture());
    self->m_d->m_tex.setData(desc.Width, desc.Height, Luminous::PixelFormat::rgbaUByte(), nullptr);
    self->m_d->m_dxTex = std::move(dxTex);
    self->m_d->m_dev = std::move(dev);
    self->m_d->m_lock = std::move(lock);
    self->m_d->m_sharedHandle = std::move(copyPtr);
    self->m_d->m_acquired = true;
    ++self->m_d->m_frameNum;
    return self;
  }

  DxSharedTexture::~DxSharedTexture()
  {
    m_d->release(true);
  }

  void DxSharedTexture::acquire()
  {
    assert(!m_d->m_acquired);
    assert(m_d->m_lock);

    updateAcquireStats(m_d->m_sharedHandle.get(), 1);
    HRESULT res = m_d->m_lock->AcquireSync(1, INFINITE);
    if (res != 0) {
      Radiant::error("DxSharedTexture # AcquireSync failed: %s", comErrorStr(res).data());
      return;
    }

    m_d->m_acquired = true;
    ++m_d->m_frameNum;

    /// @todo copy automatically to all active GPUs
  }

  bool DxSharedTexture::release()
  {
    Radiant::Guard g(m_d->m_refMutex);
    return m_d->release(false);
  }

  void * DxSharedTexture::sharedHandle() const
  {
    return m_d->m_sharedHandle.get();
  }

  Radiant::TimeStamp DxSharedTexture::lastUsed() const
  {
    return Radiant::TimeStamp(m_d->m_lastUsed);
  }

  Nimble::SizeI DxSharedTexture::size() const
  {
    return Nimble::SizeI(m_d->m_tex.width(), m_d->m_tex.height());
  }

  bool DxSharedTexture::checkStatus(unsigned int renderThreadIndex)
  {
    Context & ctx = m_d->m_ctx[renderThreadIndex];
    if (ctx.failed)
      return true;

    if (ctx.interopTex) {
      if (!m_d->m_acquired)
        return false;
      return true;
    }

    Radiant::Guard g(m_d->m_refMutex);
    return !ctx.copying && ctx.copyFrameNum == m_d->m_frameNum;
  }

  const Texture * DxSharedTexture::texture(RenderContext & r, bool copyIfNeeded)
  {
    Context & ctx = *m_d->m_ctx;
    if (ctx.failed)
      return nullptr;

    if (ctx.interopTex) {
      m_d->m_lastUsed = r.frameTime().value();
      // This shared texture can only be used when we reserve the texture to
      // this process. If we have already released this texture, it can't be used.
      if (m_d->ref(ctx)) {
        // We can release the texture after the rendering is done. This is done
        // with afterFlush executor.
        ctx.glExecutor->add([dx = shared_from_this(), &ctx] () mutable {
          dx->m_d->unref(ctx);
          // ~Context might add tasks to this same executor, and it seems that
          // folly::ManualExecutor holds a mutex to its own queue while destroying
          // this task, but not when running it. We need to destroy dx here
          // explicitly, in case dx is the last reference.
          dx.reset();
        });
        return &m_d->m_tex;
      } else {
        return nullptr;
      }
    }

    if (ctx.access == ACCESS_UNKNOWN) {
      if (auto api = r.dxInteropApi()) {
        ctx.dxInteropApi = *api;
      } else {
        Radiant::error("DxSharedTexture # WGL_NV_DX_interop is not supported");
        ctx.failed = true;
        return nullptr;
      }

      Radiant::Guard g(m_d->m_devMutex);
      ctx.interopDev = ctx.dxInteropApi.wglDXOpenDeviceNV(m_d->m_dev.Get());
      if (ctx.interopDev == nullptr) {
        /// Most likely this failed because m_dev and current OpenGL contexts
        /// are on a different devices and there is no Mosaic or similar set
        /// up. We need to copy the texture manually.
        ctx.access = ACCESS_COPY;
      } else {
        /// This shouldn't be needed anymore according to the spec, but
        /// wglDXRegisterObjectNV will fail without this
        if (!m_d->m_shareHandleSet) {
          if (ctx.dxInteropApi.wglDXSetResourceShareHandleNV(m_d->m_dxTex.Get(), m_d->m_sharedHandle.get())) {
            m_d->m_shareHandleSet = true;
          } else {
            ctx.access = ACCESS_COPY;
          }
        }
      }

      if (ctx.access == ACCESS_UNKNOWN) {
        Luminous::TextureGL & texGl = r.handle(m_d->m_tex);

        ctx.interopTex = ctx.dxInteropApi.wglDXRegisterObjectNV(
              ctx.interopDev, m_d->m_dxTex.Get(), texGl.handle(),
              GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV);

        if (ctx.interopTex) {
          /// Make sure TextureGL::upload() will be no-op and won't mess with
          /// the interop texture.
          texGl.setGeneration(m_d->m_tex.generation());
          texGl.setParamsGeneration(m_d->m_tex.paramsGeneration());
          texGl.setTarget(GL_TEXTURE_2D);

          ctx.glExecutor = &r.renderDriver().afterFlush();

          ctx.access = ACCESS_DX;
        } else {
          ctx.access = ACCESS_COPY;
        }
      }
    }

    if (ctx.access == ACCESS_DX) {
      m_d->m_lastUsed = r.frameTime().value();
      if (m_d->ref(ctx)) {
        ctx.glExecutor->add([dx = shared_from_this(), &ctx] () mutable {
          dx->m_d->unref(ctx);
          dx.reset();
        });
        return &m_d->m_tex;
      } else {
        return nullptr;
      }
    }

    // The shared texture is on wrong GPU. Check if we have a copy or if we
    // should start making one.

    if (!ctx.renderDriver) {
      ctx.renderDriver = dynamic_cast<RenderDriverGL*>(&r.renderDriver());
      if (!ctx.renderDriver) {
        ctx.failed = true;
        return nullptr;
      }
    }

    m_d->m_lastUsed = r.frameTime().value();

    {
      Radiant::Guard g(m_d->m_refMutex);

      if (ctx.copying) {
        if (ctx.copyFence) {
          GLenum e = r.glClientWaitSync(ctx.copyFence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
          if (e == GL_ALREADY_SIGNALED || e == GL_CONDITION_SATISFIED) {
            r.glDeleteSync(ctx.copyFence);
            ctx.copyFence = nullptr;
            ctx.copying = false;
            return &m_d->m_tex;
          }
        }
        return nullptr;
      }

      if (ctx.copyFrameNum == m_d->m_frameNum)
        return &m_d->m_tex;

      if (!copyIfNeeded)
        return nullptr;

      ctx.copying = true;
    }

    // Now we know that we don't have a copy of the texture, it's not being copied,
    // and we want to start a copy. Only thing remaining is to reserve the original
    // texture for us while we do the copy.
    if (!m_d->ref(ctx)) {
      Radiant::Guard g(m_d->m_refMutex);
      ctx.copying = false;
      return nullptr;
    }

    ctx.glExecutor = &r.renderDriver().afterFlush();

    if (!ctx.glTex) {
      ctx.glTex = &r.handle(m_d->m_tex);
      ctx.glTex->upload(m_d->m_tex, 0, TextureGL::UPLOAD_SYNC);
      ctx.glTex->ref();
    }

    Radiant::SingleShotTask::run([self=shared_from_this(), &ctx] {
      self->m_d->startCopy(ctx);
    });

    return nullptr;
  }

  /////////////////////////////////////////////////////////////////////////////

  class DxSharedTextureBag::D
  {
  public:
    void cleanOldTextures();

  public:
    QReadWriteLock m_texturesLock;
    std::vector<std::shared_ptr<DxSharedTexture>> m_textures;
    Luminous::ContextArrayT<int> m_rendered;
  };

  /////////////////////////////////////////////////////////////////////////////

  void DxSharedTextureBag::D::cleanOldTextures()
  {
    m_texturesLock.lockForWrite();

    /// Always keep the latest texture alive
    if (m_textures.size() <= 1) {
      m_texturesLock.unlock();
      return;
    }

    const Nimble::SizeI size = m_textures.back()->size();
    const Radiant::TimeStamp now = Radiant::TimeStamp::currentTime();
    const double timeout = 3.0; // seconds

    std::vector<unsigned int> activeThreads;
    for (unsigned int idx = 0; idx < m_rendered.size(); ++idx) {
      if (m_rendered[idx]) {
        activeThreads.push_back(idx);
        m_rendered[idx] -= 1;
      }
    }

    for (size_t i = m_textures.size() - 1;;) {
      const bool canRelease = activeThreads.empty() && i != m_textures.size() - 1;
      DxSharedTexture & tex = *m_textures[i];

      for (auto it2 = activeThreads.begin(); it2 != activeThreads.end();) {
        if (tex.checkStatus(*it2)) {
          it2 = activeThreads.erase(it2);
        } else {
          ++it2;
        }
      }

      if (canRelease && tex.release()) {
        if (tex.size() != size || (now - tex.lastUsed()).secondsD() >= timeout) {
          m_textures.erase(m_textures.begin() + i);
        }
      }

      if (i == 0)
        break;
      --i;
    }

    m_texturesLock.unlock();
  }

  /////////////////////////////////////////////////////////////////////////////

  DxSharedTextureBag::DxSharedTextureBag()
    : m_d(new D())
  {
    Radiant::Guard g(s_allDxTextureBagsMutex);
    s_allDxTextureBags.insert(this);
  }

  DxSharedTextureBag::~DxSharedTextureBag()
  {
    Radiant::Guard g(s_allDxTextureBagsMutex);
    s_allDxTextureBags.erase(this);
  }

  bool DxSharedTextureBag::addSharedHandle(void * sharedHandle)
  {
    bool found = false;
    m_d->m_texturesLock.lockForWrite();
    for (auto it = m_d->m_textures.begin(); it != m_d->m_textures.end(); ++it) {
      if (CompareObjectHandles((*it)->sharedHandle(), sharedHandle)) {
        std::shared_ptr<DxSharedTexture> tex = std::move(*it);
        m_d->m_textures.erase(it);
        tex->acquire();
        found = true;
        // Put the latest texture to back
        m_d->m_textures.push_back(std::move(tex));
        break;
      }
    }
    m_d->m_texturesLock.unlock();

    if (!found) {
      if (auto tex = DxSharedTexture::create(sharedHandle)) {
        m_d->m_texturesLock.lockForWrite();
        m_d->m_textures.push_back(std::move(tex));
        m_d->m_texturesLock.unlock();
        found = true;
      }
    }

    return found;
  }

  const Texture * DxSharedTextureBag::texture(RenderContext & r)
  {
    // m_rendered is used to keep track of which render threads are interested
    // in this texture. For those render threads we maintain at least one good
    // texture in the texture pool, so there's always something to render.
    // Ideally this could just be a boolean (keepAliveFrames = 1), but there are
    // some componenta like CachingViewWidget that don't call render every
    // frame.
    //
    // The bigger number we have here, the longer we keep textures alive for
    // those components, which avoids blinking. However, too large number here
    // might prevent us from releasing the source texture and block the other
    // application that is feeding these textures.
    const int keepAliveFrames = 3;
    *m_d->m_rendered = keepAliveFrames;

    m_d->m_texturesLock.lockForRead();
    if (m_d->m_textures.empty()) {
      m_d->m_texturesLock.unlock();
      return nullptr;
    }

    bool first = true;
    for (auto it = m_d->m_textures.rbegin(); it != m_d->m_textures.rend(); ++it) {
      const Texture * tex = (*it)->texture(r, first);
      // Start copy operation only on the first texture
      first = false;
      if (tex) {
        m_d->m_texturesLock.unlock();
        return tex;
      }
    }

    m_d->m_texturesLock.unlock();
    return nullptr;
  }

  void DxSharedTextureBag::clean()
  {
    Radiant::Guard g(s_allDxTextureBagsMutex);
    for (DxSharedTextureBag * t: s_allDxTextureBags)
      t->m_d->cleanOldTextures();
  }
}
