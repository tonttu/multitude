#include "ContextArray.hpp"
#include "DxInterop.hpp"
#include "DxSharedTexture.hpp"
#include "RenderContext.hpp"
#include "ResourceHandleGL.hpp"
#include "GfxDriver.hpp"

#include <Radiant/Mutex.hpp>
#include <Radiant/Task.hpp>
#include <Radiant/StringUtils.hpp>

#include <folly/futures/Future.h>
#include <folly/executors/ManualExecutor.h>

#include <QReadWriteLock>

#include <dxgi.h>
#include <d3d11_1.h>

#include <comdef.h>
#include <wrl.h>

#include <cuda_runtime_api.h>
#include <cuda_d3d11_interop.h>
#include <cuda_gl_interop.h>

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
    Radiant::debug("Dx acquire stats: %s", stats.join(", ").toUtf8().data());
  }
#else
  void updateAcquireStats(void *, int) {}
#endif

  QByteArray comErrorStr(HRESULT res)
  {
    return QString::fromStdWString(_com_error(res).ErrorMessage()).toUtf8();
  }

  bool cudaCheck(cudaError_t error, const char * cmd, const char * file, int line)
  {
    if (error == cudaSuccess)
      return true;

    Radiant::error("CUDA error %s:%d: %s: %s", file, line, cmd, cudaGetErrorString(error));
    return false;
  }

#define CUDA_CHECK(cmd) cudaCheck(cmd, #cmd, __FILE__, __LINE__)

  /// Creates a device from the same adapter that owns sharedHandle
  ComPtr<ID3D11Device1> createDevice(HANDLE sharedHandle, LUID & adapterLuid)
  {
    ComPtr<IDXGIFactory2> dxgiFactory;

    HRESULT res = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(res)) {
      Radiant::error("DxSharedTexture # CreateDXGIFactory1 failed: %s",
                     comErrorStr(res).data());
      return nullptr;
    }

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

    ComPtr<ID3D11Device1> dev1;
    res = dev->QueryInterface(IID_PPV_ARGS(&dev1));
    if (FAILED(res)) {
      Radiant::error("DxSharedTexture # QueryInterface ID3D11Device1 failed: %s",
                     comErrorStr(res).data());
      return nullptr;
    }

    return dev1;
  }
}

namespace Luminous
{
  /// Status of copying textures between GPUs
  enum CopyStatus
  {
    /// No copying is in progress, or copying already finished.
    COPY_STATUS_NONE,
    /// Copying is in progress and ref() was called, so the original texture
    /// is locked for Cornerstone.
    COPY_STATUS_STARTED,
    /// The original texture has been duplicated and unref() has been called.
    /// Copying to host or to the other GPU is in progress or finished. Call
    /// checkCopy to find out and finalize the copy process.
    COPY_STATUS_DONE,
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

    /// If this GPU doesn't belong to D::m_dev, the texture data for this
    /// GPU is copied with CUDA to this texture.
    cudaGraphicsResource_t cudaTex = nullptr;
    /// Status of ongoing DX / CUDA copy tasks
    /// Protected with D::m_devMutex
    CopyStatus cudaCopying = COPY_STATUS_NONE;
    /// Frame number of cudaTex
    /// Protected with D::m_devMutex
    uint64_t cudaFrameNum = 0;
    /// CUDA stream where copying and OpenGL interop happens.
    cudaStream_t cudaStream = nullptr;
    /// CUDA device that matches this GPU
    int cudaDev = -1;

    /// Set to true if either DX-GL or CUDA-GL interop failed for this GPU.
    /// Invalidates the whole DxSharedTexture object for this context and
    /// there is no recovery. Practically all reasons for any errors are
    /// something that would happen again later, so this practically just
    /// stops flooding the same error message every frame (like CUDA is
    /// not supported).
    std::atomic<bool> failed{false};

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
      std::swap(cudaTex, c.cudaTex);
      std::swap(cudaCopying, c.cudaCopying);
      std::swap(cudaFrameNum, c.cudaFrameNum);
      std::swap(cudaStream, c.cudaStream);
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
    void startCopy(Context & ctx, GLuint handle, bool registerTex);
    bool checkCopy(Context & ctx);

    bool ref(Context & ctx);
    void lockAndUnref(Context & ctx);
    void unref(Context & ctx);

    // Returns false if force was false and the texture is still in use
    bool release(bool force);

  public:
    /// Lock this when using m_dev or any interop functions in GL/CUDA
    /// associated with m_dev.
    Radiant::Mutex m_devMutex;
    /// The device that owns the shared texture.
    ComPtr<ID3D11Device1> m_dev;
    /// LUID of m_dev adapter. Used for identifying the correct render thread
    /// that uses the same GPU as m_dev.
    LUID m_adapterLuid;
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
    /// share it with CUDA.
    ComPtr<ID3D11Texture2D> m_copy;
    /// Frame number of m_copy (compare to m_frameNumber)
    std::atomic<uint64_t> m_dxCopyFrameNum{0};

    /// Pinned memory where we copy data from m_copy
    std::shared_ptr<void> m_copyData;
    /// Frame number of m_copyData (compare to m_frameNumber)
    std::atomic<uint64_t> m_pinnedCopyFrameNum{0};

    /// Texture with the correct size and pixel format, but empty data pointer.
    /// Some of the TextureGL instances might be using DX interop extension and
    /// others CUDA interop extension.
    Luminous::Texture m_tex;

    /// Per-render thread data
    ContextArrayT<Context> m_ctx;

    /// Luminous::RenderThread index of the thread that runs on the same GPU as m_dev
    int m_ownerThreadIndex = -1;
    /// CUDA dev that matches m_dev
    int m_ownerCudaDev = -1;
    /// m_copy in CUDA
    cudaGraphicsResource_t m_cudaTex = nullptr;
    /// Copy stream, used for DtoH copies
    cudaStream_t m_cudaStream = nullptr;
    /// Synchronize DtoH copy event between m_cudaStream and several ctx.cudaStream
    cudaEvent_t m_copyEvent = nullptr;
    /// Number of ongoing copy operations. Protected by m_devMutex. Once this
    /// goes to zero, we can unmap m_cudaTex
    int m_copyRef = 0;

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
    if (glExecutor && cudaTex) {
      glExecutor->add([cudaDev=cudaDev, cudaTex=cudaTex] {
        CUDA_CHECK(cudaSetDevice(cudaDev));
        CUDA_CHECK(cudaGraphicsUnregisterResource(cudaTex));
      });
    }
    if (glExecutor && interopTex) {
      glExecutor->add([interopDev=this->interopDev, interopTex=this->interopTex,
                      dxInteropApi=this->dxInteropApi] {
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

  /// Copy D3D texture 'm_dxTex' to OpenGL texture 'handle' on a different GPU using CUDA.
  ///  * First make a copy of m_dxTex to a temporary m_copy DX texture on the
  ///    source GPU, since CUDA - DX interop code doesn't work with shared textures
  ///  * Map m_copy to CUDA
  ///  * Allocate memory for handle on the destination GPU using OpenGL
  ///  * Map handle to CUDA
  ///  * Perform memory copy between the CUDA objects
  ///  * Finalize everything in checkCopy
  void DxSharedTexture::D::startCopy(Context & ctx, GLuint handle, bool registerTex)
  {
    Radiant::Guard g(m_devMutex);
    if (!m_copy) {
      D3D11_TEXTURE2D_DESC desc;
      m_dxTex->GetDesc(&desc);
      desc.BindFlags = 0;
      desc.MiscFlags = 0;
      if (FAILED(m_dev->CreateTexture2D(&desc, nullptr, &m_copy)))
        abort();
    }

    if (m_dxCopyFrameNum < m_frameNum) {
      ComPtr<ID3D11DeviceContext1> deviceCtx;
      m_dev->GetImmediateContext1(&deviceCtx);
      deviceCtx->CopyResource(m_copy.Get(), m_dxTex.Get());
      m_dxCopyFrameNum = m_frameNum.load();
    }

    // We don't need the original texture anymore
    unref(ctx);

    if (m_ownerCudaDev < 0) {
      ComPtr<IDXGIDevice> dxgiDev;
      if (FAILED(m_dev->QueryInterface(IID_PPV_ARGS(&dxgiDev)))) abort();
      ComPtr<IDXGIAdapter> adapter;
      if (FAILED(dxgiDev->GetAdapter(&adapter))) abort();

      CUDA_CHECK(cudaD3D11GetDevice(&m_ownerCudaDev, adapter.Get()));
    }

    CUDA_CHECK(cudaSetDevice(m_ownerCudaDev));
    if (!m_cudaStream)
      CUDA_CHECK(cudaStreamCreateWithFlags(&m_cudaStream, cudaStreamNonBlocking));
    if (!m_cudaTex)
      CUDA_CHECK(cudaGraphicsD3D11RegisterResource(&m_cudaTex, m_copy.Get(),
                                                   cudaGraphicsRegisterFlagsNone));

    if (++m_copyRef == 1)
      CUDA_CHECK(cudaGraphicsMapResources(1, &m_cudaTex, m_cudaStream));
    m_devMutex.unlock();

    cudaArray_t srcArray;
    CUDA_CHECK(cudaGraphicsSubResourceGetMappedArray(&srcArray, m_cudaTex, 0, 0));

    CUDA_CHECK(cudaSetDevice(ctx.cudaDev));

    if (!ctx.cudaStream)
      CUDA_CHECK(cudaStreamCreateWithFlags(&ctx.cudaStream, cudaStreamNonBlocking));

    if (ctx.cudaTex && registerTex)
      CUDA_CHECK(cudaGraphicsUnregisterResource(ctx.cudaTex));
    if (!ctx.cudaTex || registerTex)
      CUDA_CHECK(cudaGraphicsGLRegisterImage(&ctx.cudaTex, handle, GL_TEXTURE_2D,
                                             cudaGraphicsRegisterFlagsWriteDiscard));

    CUDA_CHECK(cudaGraphicsMapResources(1, &ctx.cudaTex, ctx.cudaStream));

    cudaArray_t targetArray;
    CUDA_CHECK(cudaGraphicsSubResourceGetMappedArray(&targetArray, ctx.cudaTex, 0, 0));

    m_devMutex.lock();
    if (m_pinnedCopyFrameNum < m_frameNum) {
      m_devMutex.unlock();
      CUDA_CHECK(cudaSetDevice(m_ownerCudaDev));

      if (!m_copyData) {
        void * data = nullptr;
        cudaHostAlloc(&data, m_tex.width() * 4 * m_tex.height(), cudaHostAllocDefault);
        std::shared_ptr<void> ptr(data, &cudaFreeHost);
        m_copyData = std::move(ptr);
      }

      if (!m_copyEvent)
        cudaEventCreateWithFlags(&m_copyEvent, cudaEventDisableTiming);

      CUDA_CHECK(cudaMemcpy2DFromArrayAsync(
                   m_copyData.get(), m_tex.width() * 4, srcArray, 0, 0,
                   m_tex.width() * 4, m_tex.height(), cudaMemcpyDeviceToHost, m_cudaStream));
      CUDA_CHECK(cudaEventRecord(m_copyEvent, m_cudaStream));

      CUDA_CHECK(cudaSetDevice(ctx.cudaDev));

      m_devMutex.lock();
      m_pinnedCopyFrameNum = m_frameNum.load();
    }
    m_devMutex.unlock();

    CUDA_CHECK(cudaStreamWaitEvent(ctx.cudaStream, m_copyEvent, 0));
    CUDA_CHECK(cudaMemcpy2DToArrayAsync(
                 targetArray, 0, 0, m_copyData.get(),
                 m_tex.width() * 4, m_tex.width() * 4, m_tex.height(),
                 cudaMemcpyHostToDevice, ctx.cudaStream));

    CUDA_CHECK(cudaGraphicsUnmapResources(1, &ctx.cudaTex, ctx.cudaStream));

    m_devMutex.lock();
    ctx.cudaCopying = COPY_STATUS_DONE;
  }

  bool DxSharedTexture::D::checkCopy(Context & ctx)
  {
    if (ctx.cudaCopying != COPY_STATUS_DONE)
      return false;

    CUDA_CHECK(cudaSetDevice(ctx.cudaDev));
    cudaError_t err = cudaStreamQuery(ctx.cudaStream);
    if (err == cudaSuccess) {
      if (--m_copyRef == 0) {
        CUDA_CHECK(cudaSetDevice(m_ownerCudaDev));
        CUDA_CHECK(cudaGraphicsUnmapResources(1, &m_cudaTex, m_cudaStream));
      }
      ctx.cudaCopying = COPY_STATUS_NONE;
      ctx.cudaFrameNum = m_frameNum;
      return true;
    } else if (err == cudaErrorNotReady) {
      return false;
    } else {
      CUDA_CHECK(err);
      return false;
    }
  }

  bool DxSharedTexture::D::ref(Context & ctx)
  {
    if (!m_acquired)
      return false;
    ++m_refs;

    if (ctx.interopTex && ++ctx.glRefs == 1) {
      if (!ctx.dxInteropApi.wglDXLockObjectsNV(ctx.interopDev, 1, &ctx.interopTex)) {
        GLERROR("wglDXLockObjectsNV");
        Radiant::error("DxSharedTexture # wglDXLockObjectsNV failed");
      }
    }
    return true;
  }

  void DxSharedTexture::D::lockAndUnref(Context & ctx)
  {
    Radiant::Guard g(m_devMutex);
    unref(ctx);
  }

  void DxSharedTexture::D::unref(Context & ctx)
  {
    if (ctx.interopTex && --ctx.glRefs == 0) {
      if (!ctx.dxInteropApi.wglDXUnlockObjectsNV(ctx.interopDev, 1, &ctx.interopTex)) {
        GLERROR("wglDXUnlockObjectsNV");
        Radiant::error("DxSharedTexture # wglDXUnlockObjectsNV failed");
      }
    }
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
    : m_d(new D())
  {
    m_d->m_tex.setExpiration(0);

    /// Initialize all CUDA devices, otherwise the application freezes for
    /// ~200ms when a cuda stream is created on a new device.
    MULTI_ONCE {
      Radiant::SingleShotTask::run([] {
        int count = 0;
        CUDA_CHECK(cudaGetDeviceCount(&count));
        for (int dev = 0; dev < count; ++dev) {
          CUDA_CHECK(cudaSetDevice(dev));
          cudaSetDeviceFlags(cudaDeviceMapHost | cudaDeviceScheduleYield);
          cudaStream_t stream;
          CUDA_CHECK(cudaStreamCreateWithFlags(&stream, cudaStreamNonBlocking));
          CUDA_CHECK(cudaStreamDestroy(stream));
        }
      });
    }
  }

  std::shared_ptr<DxSharedTexture> DxSharedTexture::create(void * sharedHandle)
  {
    assert(sharedHandle);

    LUID adapterLuid{};
    ComPtr<ID3D11Device1> dev = createDevice(sharedHandle, adapterLuid);
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
    self->m_d->m_adapterLuid = adapterLuid;
    ++self->m_d->m_frameNum;
    return self;
  }

  DxSharedTexture::~DxSharedTexture()
  {
    /// @todo destroy D in bg thread, for instance cudaFreeHost might be slow

    for (Context & ctx: m_d->m_ctx) {
      if (ctx.cudaStream) {
        /// @todo should this be done in ~Context?
        CUDA_CHECK(cudaSetDevice(ctx.cudaDev));
        CUDA_CHECK(cudaStreamDestroy(ctx.cudaStream));
        ctx.cudaStream = nullptr;
      }

      if (ctx.cudaCopying != COPY_STATUS_NONE && --m_d->m_copyRef == 0) {
        CUDA_CHECK(cudaSetDevice(m_d->m_ownerCudaDev));
        CUDA_CHECK(cudaGraphicsUnmapResources(1, &m_d->m_cudaTex, m_d->m_cudaStream));
      }
    }

    if (m_d->m_cudaTex) {
      CUDA_CHECK(cudaSetDevice(m_d->m_ownerCudaDev));
      CUDA_CHECK(cudaGraphicsUnregisterResource(m_d->m_cudaTex));
      CUDA_CHECK(cudaStreamDestroy(m_d->m_cudaStream));
    }

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
    Radiant::Guard g(m_d->m_devMutex);
    for (Context & ctx: m_d->m_ctx)
      m_d->checkCopy(ctx);
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
      return false;

    Radiant::Guard g(m_d->m_devMutex);
    if (ctx.interopTex) {
      if (!m_d->m_acquired)
        return false;
      return true;
    }

    if (m_d->checkCopy(ctx))
      return true;

    return ctx.cudaFrameNum == m_d->m_frameNum;
  }

  const Texture * DxSharedTexture::texture(RenderContext & r, bool copyIfNeeded)
  {
    Context & ctx = *m_d->m_ctx;
    if (ctx.failed)
      return nullptr;

    if (ctx.interopTex) {
      Radiant::Guard g(m_d->m_devMutex);
      m_d->m_lastUsed = r.frameTime().value();
      // This shared texture can only be used when we reserve the texture to
      // this process. If we have already released this texture, it can't be used.
      if (m_d->ref(ctx)) {
        // We can release the texture after the rendering is done. This is done
        // with afterFlush executor.
        ctx.glExecutor->add([dx = shared_from_this(), &ctx] () mutable {
          dx->m_d->lockAndUnref(ctx);
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

    if (m_d->m_ownerThreadIndex < 0) {
      auto & gfx = r.renderDriver().gfxDriver();
      for (unsigned int i = 0, s = gfx.renderThreadCount(); i < s; ++i) {
        if (gfx.renderDriver(i).gpuInfo().dxgiAdapterLuid == m_d->m_adapterLuid) {
          m_d->m_ownerThreadIndex = i;
          break;
        }
      }
    }

    unsigned int currentThreadIndex = r.renderDriver().threadIndex();

    // If we don't have GPU affinities set, gpuInfo().dxgiAdapterLuid is going
    // to be empty and m_ownerThreadIndex is -1, so we attempt to use the
    // shared texture directly, that should work, even though it might be slow.
    if (m_d->m_ownerThreadIndex < 0 || m_d->m_ownerThreadIndex == currentThreadIndex) {
      if (!ctx.dxInteropApi.isInitialized()) {
        if (auto api = r.dxInteropApi()) {
          ctx.dxInteropApi = *api;
        } else {
          Radiant::error("DxSharedTexture # WGL_NV_DX_interop is not supported");
          ctx.failed = true;
          return nullptr;
        }
      }

      Radiant::Guard g(m_d->m_devMutex);
      m_d->m_lastUsed = r.frameTime().value();
      if (!ctx.interopDev) {
        ctx.interopDev = ctx.dxInteropApi.wglDXOpenDeviceNV(m_d->m_dev.Get());
        if (ctx.interopDev == nullptr) {
          GLERROR("wglDXOpenDeviceNV");
          Radiant::error("DxSharedTexture # wglDXOpenDeviceNV failed");
          ctx.failed = true;
          return nullptr;
        }
      }

      ctx.glExecutor = &r.renderDriver().afterFlush();
      Luminous::TextureGL & texGl = r.handle(m_d->m_tex);

      /// Make sure TextureGL::upload() will be no-op and won't mess with
      /// the interop texture.
      texGl.setGeneration(m_d->m_tex.generation());
      texGl.setParamsGeneration(m_d->m_tex.paramsGeneration());
      texGl.setTarget(GL_TEXTURE_2D);

      /// This shouldn't be needed anymore according to the spec, but
      /// wglDXRegisterObjectNV will fail without this
      if (!m_d->m_shareHandleSet) {
        if (!ctx.dxInteropApi.wglDXSetResourceShareHandleNV(m_d->m_dxTex.Get(), m_d->m_sharedHandle.get())) {
          GLERROR("wglDXSetResourceShareHandleNV");
          Radiant::error("DxSharedTexture # wglDXSetResourceShareHandleNV failed");
          ctx.failed = true;
          return nullptr;
        }
        m_d->m_shareHandleSet = true;
      }

      ctx.interopTex = ctx.dxInteropApi.wglDXRegisterObjectNV(
            ctx.interopDev, m_d->m_dxTex.Get(), texGl.handle(),
            GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV);

      if (ctx.interopTex == nullptr) {
        GLERROR("wglDXRegisterObjectNV");
        Radiant::error("DxSharedTexture # wglDXRegisterObjectNV failed");
        ctx.failed = true;
        return nullptr;
      }

      if (m_d->ref(ctx)) {
        ctx.glExecutor->add([dx = shared_from_this(), &ctx] () mutable {
          dx->m_d->lockAndUnref(ctx);
          dx.reset();
        });
        return &m_d->m_tex;
      } else {
        return nullptr;
      }
    }

    // The shared texture is on wrong GPU. Check if we have a copy or if we
    // should start making one.

    if (ctx.cudaDev < 0)
      ctx.cudaDev = r.renderDriver().gpuInfo().cudaDev;

    if (ctx.cudaDev < 0) {
      Radiant::error("DxSharedTexture # Failed to find correct CUDA device");
      ctx.failed = true;
      return false;
    }

    Radiant::Guard g(m_d->m_devMutex);
    m_d->m_lastUsed = r.frameTime().value();

    if (ctx.cudaCopying != COPY_STATUS_NONE)
      return m_d->checkCopy(ctx) ? &m_d->m_tex : nullptr;

    if (ctx.cudaFrameNum == m_d->m_frameNum)
      return &m_d->m_tex;

    if (!copyIfNeeded)
      return nullptr;

    // Now we know that we don't have a copy of the texture, it's not being copied,
    // and we want to start a copy. Only thing remaining is to reserve the original
    // texture for us while we do the copy.
    if (!m_d->ref(ctx))
      return nullptr;

    ctx.glExecutor = &r.renderDriver().afterFlush();

    Luminous::TextureGL & texgl = r.handle(m_d->m_tex);
    bool registerTex = texgl.generation() == 0;
    texgl.upload(m_d->m_tex, 0);

    ctx.cudaCopying = COPY_STATUS_STARTED;

    r.stateGl().addTask([weak = std::weak_ptr<DxSharedTexture>(shared_from_this()), &ctx, handle = texgl.handle(), registerTex] {
      if (auto self = weak.lock())
        self->m_d->startCopy(ctx, handle, registerTex);
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
