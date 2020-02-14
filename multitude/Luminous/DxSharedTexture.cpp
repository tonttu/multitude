#include "ContextArray.hpp"
#include "DxInterop.hpp"
#include "DxSharedTexture.hpp"
#include "RenderContext.hpp"
#include "ResourceHandleGL.hpp"
#include "GfxDriver.hpp"

#include <Radiant/Mutex.hpp>
#include <Radiant/StringUtils.hpp>

#include <folly/executors/ManualExecutor.h>

#include <dxgi.h>
#include <d3d11_1.h>

#include <comdef.h>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

bool operator==(const LUID & l, const LUID & r)
{
  return l.LowPart == r.LowPart && l.HighPart == r.HighPart;
}

namespace
{
  using WglCopyImageSubDataNV = BOOL (WINAPI *)(
    HGLRC hSrcRC, GLuint srcName, GLenum srcTarget, GLint srcLevel,
    GLint srcX, GLint srcY, GLint srcZ,
    HGLRC hDstRC, GLuint dstName, GLenum dstTarget, GLint dstLevel,
    GLint dstX, GLint dstY, GLint dstZ,
    GLsizei width, GLsizei height, GLsizei depth);

  QByteArray comErrorStr(HRESULT res)
  {
    return QString::fromStdWString(_com_error(res).ErrorMessage()).toUtf8();
  }

  enum class CopyStatus
  {
    None,
    Copying,
    Finished,
    Failed
  };

  struct Context
  {
    Luminous::DxInterop dxInteropApi;
    HANDLE interopDev = nullptr;
    HANDLE interopTex = nullptr;
    folly::Executor * executor = nullptr;
    WglCopyImageSubDataNV wglCopyImageSubDataNV = nullptr;

    std::atomic<CopyStatus> copyStatus{CopyStatus::None};

    Context() = default;
    Context(Context &&) = default;
    Context & operator=(Context &&) = default;
    Context(const Context & c)
      : dxInteropApi(c.dxInteropApi)
      , interopDev(c.interopDev)
      , interopTex(c.interopTex)
      , executor(c.executor)
      , wglCopyImageSubDataNV(c.wglCopyImageSubDataNV)
      , copyStatus(c.copyStatus.load())
    {}

    void release();
  };

  void Context::release()
  {
    if (interopTex) {
      if (!dxInteropApi.wglDXUnlockObjectsNV(interopDev, 1, &interopTex)) {
        GLERROR("wglDXUnlockObjectsNV");
        Radiant::error("DxSharedTexture # wglDXUnlockObjectsNV failed");
      }

      if (!dxInteropApi.wglDXUnregisterObjectNV(interopDev, interopTex)) {
        GLERROR("wglDXUnregisterObjectNV");
        Radiant::error("DxSharedTexture # wglDXUnregisterObjectNV failed");
      }
      interopTex = nullptr;
    }

    if (interopDev) {
      if (!dxInteropApi.wglDXCloseDeviceNV(interopDev)) {
        GLERROR("wglDXCloseDeviceNV");
        Radiant::error("DxSharedTexture # wglDXCloseDeviceNV failed");
      }
      interopDev = nullptr;
    }
  }

  /// Creates a device from the same adapter that owns sharedHandle
  ComPtr<ID3D11Device1> createDevice(HANDLE sharedHandle, LUID & adapterLuid)
  {
    ComPtr<IDXGIFactory2> dxgiFactory;

    HRESULT res = CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));
    if (FAILED(res)) {
      Radiant::error("DxSharedTexture # CreateDXGIFactory1 failed: %s", comErrorStr(res).data());
      return nullptr;
    }

    res = dxgiFactory->GetSharedResourceAdapterLuid(sharedHandle, &adapterLuid);
    if (FAILED(res)) {
      Radiant::error("DxSharedTexture # GetSharedResourceAdapterLuid failed: %s", comErrorStr(res).data());
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
      Radiant::error("DxSharedTexture # Couldn't find the correct DXGIAdapter for the shared texture");
      return nullptr;
    }

    ComPtr<ID3D11Device> dev;
    D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_11_1 };
    res = D3D11CreateDevice(it, D3D_DRIVER_TYPE_UNKNOWN, nullptr,
                            0, featureLevels, 1,
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
  class DxSharedTexture::D
  {
  public:
    // The device that owns the shared texture
    ComPtr<ID3D11Device1> m_dev;
    LUID m_adapterLuid;
    std::shared_ptr<void> m_sharedHandle;

    ComPtr<ID3D11Texture2D> m_dxTex;
    ComPtr<IDXGIKeyedMutex> m_lock;

    Luminous::Texture m_tex;

    ContextArrayT<Context> m_ctx;

    int m_ownerThreadIndex = -1;

    /// Protects m_refs, m_acquired and m_lastUsed
    Radiant::Mutex m_refsMutex;
    int m_refs = 0;
    bool m_acquired = false;
    Radiant::TimeStamp m_lastUsed = Radiant::TimeStamp::currentTime();
  };

  /////////////////////////////////////////////////////////////////////////////

  DxSharedTexture::DxSharedTexture()
    : m_d(new D())
  {
  }

  bool DxSharedTexture::init(void * sharedHandle)
  {
    assert(sharedHandle);

    LUID adapterLuid{};
    ComPtr<ID3D11Device1> dev = createDevice(sharedHandle, adapterLuid);
    if (!dev) {
      return false;
    }

    HANDLE copy = nullptr;
    HANDLE currentProcessHandle = GetCurrentProcess();
    if (!DuplicateHandle(currentProcessHandle, sharedHandle,
                         currentProcessHandle, &copy,
                         0, FALSE, DUPLICATE_SAME_ACCESS)) {
      Radiant::error("DxSharedTexture # DuplicateHandle failed: %s",
                     Radiant::StringUtils::getLastErrorMessage().toUtf8().data());
      return false;
    }
    std::shared_ptr<void> copyPtr(copy, &CloseHandle);

    ComPtr<ID3D11Texture2D> dxTex;
    HRESULT res = dev->OpenSharedResource1(copy, IID_PPV_ARGS(&dxTex));
    if (FAILED(res)) {
      Radiant::error("DxSharedTexture # OpenSharedResource1 failed: %s",
                     comErrorStr(res).data());
      return false;
    }

    ComPtr<IDXGIKeyedMutex> lock;
    res = dxTex->QueryInterface(IID_PPV_ARGS(&lock));
    if (FAILED(res)) {
      Radiant::error("DxSharedTexture # QueryInterface IDXGIKeyedMutex failed: %s",
                     comErrorStr(res).data());
      return false;
    } else {
      res = lock->AcquireSync(1, INFINITE);
      if (res != 0) {
        Radiant::error("DxSharedTexture # AcquireSync failed: %s [0x%x]",
                       comErrorStr(res).data(), res);
        return false;
      }
    }

    D3D11_TEXTURE2D_DESC desc;
    dxTex->GetDesc(&desc);

    m_d->m_tex.setData(desc.Width, desc.Height, Luminous::PixelFormat::rgbaUByte(),
                       nullptr);
    m_d->m_dxTex = std::move(dxTex);
    m_d->m_dev = std::move(dev);
    m_d->m_lock = std::move(lock);
    m_d->m_sharedHandle = std::move(copyPtr);
    m_d->m_acquired = true;
    m_d->m_adapterLuid = adapterLuid;
    return true;
  }

  bool DxSharedTexture::release(bool force)
  {
    {
      Radiant::Guard g(m_d->m_refsMutex);
      if (!m_d->m_lock || !m_d->m_acquired)
        return true;

      if (!force && m_d->m_refs > 0)
        return false;

      m_d->m_acquired = false;
    }
    HRESULT res = m_d->m_lock->ReleaseSync(0);
    if (res != 0) {
      Radiant::error("DxSharedTexture # ReleaseSync failed: %s [0x%x]",
                     comErrorStr(res).data(), res);
    }
    return true;
  }

  void DxSharedTexture::ref(Radiant::TimeStamp time)
  {
    Radiant::Guard g(m_d->m_refsMutex);
    m_d->m_lastUsed = time;
    ++m_d->m_refs;
  }

  void DxSharedTexture::unref()
  {
    bool doRelease = false;
    {
      Radiant::Guard g(m_d->m_refsMutex);
      doRelease = --m_d->m_refs == 0;
    }
    if (doRelease)
      release(false);
  }

  DxSharedTexture::~DxSharedTexture()
  {
    release(true);
    for (Context & ctx: m_d->m_ctx) {
      if (ctx.executor && ctx.interopTex) {
        try {
        ctx.executor->add([ctx] () mutable {
          ctx.release();
        });
        } catch (std::exception & error) {
          Radiant::error("%s", error.what());
        }
      }
    }
  }

  void DxSharedTexture::acquire()
  {
    assert(!m_d->m_acquired);
    assert(m_d->m_lock);

    HRESULT res = m_d->m_lock->AcquireSync(1, INFINITE);
    if (res != 0) {
      Radiant::error("DxSharedTexture # AcquireSync failed: %s", comErrorStr(res).data());
      return;
    }

    m_d->m_acquired = true;
  }

  void * DxSharedTexture::sharedHandle() const
  {
    return m_d->m_sharedHandle.get();
  }

  Radiant::TimeStamp DxSharedTexture::lastUsed() const
  {
    return m_d->m_lastUsed;
  }

  Nimble::SizeI DxSharedTexture::size() const
  {
    return Nimble::SizeI(m_d->m_tex.width(), m_d->m_tex.height());
  }

  const Texture * DxSharedTexture::texture(RenderContext & r, bool copyIfNeeded, std::weak_ptr<DxSharedTexture> weak)
  {
    {
      Radiant::Guard g(m_d->m_refsMutex);
      m_d->m_lastUsed = r.frameTime();
    }

    Context & ctx = *m_d->m_ctx;
    if (ctx.interopTex || ctx.copyStatus == CopyStatus::Finished)
      return &m_d->m_tex;

    if (!ctx.dxInteropApi.isInitialized()) {
      if (auto api = r.dxInteropApi())
        ctx.dxInteropApi = *api;
      else
        return nullptr;
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

    if (m_d->m_ownerThreadIndex < 0) {
      // This adapter might not support OpenGL affinity extensions, so we just
      // hope for the best.
      m_d->m_ownerThreadIndex = currentThreadIndex;
    }

    if (m_d->m_ownerThreadIndex == currentThreadIndex) {
      if (!ctx.interopDev) {
        ctx.interopDev = ctx.dxInteropApi.wglDXOpenDeviceNV(m_d->m_dev.Get());
        if (ctx.interopDev == nullptr) {
          GLERROR("wglDXOpenDeviceNV");
          Radiant::error("DxSharedTexture # wglDXOpenDeviceNV failed");
          return nullptr;
        }
      }

      ctx.executor = &r.renderDriver().afterFlush();
      Luminous::TextureGL & texGl = r.handle(m_d->m_tex);

      if (!ctx.interopTex) {
        /// @todo this shouldn't be needed anymore according to the spec, but
        /// wglDXRegisterObjectNV will fail without this
        if (!ctx.dxInteropApi.wglDXSetResourceShareHandleNV(m_d->m_dxTex.Get(), m_d->m_sharedHandle.get())) {
          GLERROR("wglDXSetResourceShareHandleNV");
          Radiant::error("DxSharedTexture # wglDXSetResourceShareHandleNV failed");
          return nullptr;
        }

        ctx.interopTex = ctx.dxInteropApi.wglDXRegisterObjectNV(
              ctx.interopDev, m_d->m_dxTex.Get(), texGl.handle(),
              GL_TEXTURE_2D, WGL_ACCESS_READ_ONLY_NV);

        if (ctx.interopTex == nullptr) {
          GLERROR("wglDXRegisterObjectNV");
          Radiant::error("DxSharedTexture # wglDXRegisterObjectNV failed");
          return nullptr;
        }

        if (!ctx.dxInteropApi.wglDXLockObjectsNV(ctx.interopDev, 1, &ctx.interopTex)) {
          GLERROR("wglDXLockObjectsNV");
          Radiant::error("DxSharedTexture # wglDXLockObjectsNV failed");
          return nullptr;
        }
      }
    } else if (!copyIfNeeded) {
      return nullptr;
    } else {
      auto expected = CopyStatus::None;
      if (!ctx.copyStatus.compare_exchange_strong(expected, CopyStatus::Copying))
        return ctx.copyStatus == CopyStatus::Finished ? &m_d->m_tex : nullptr;

      HGLRC targetContext = wglGetCurrentContext();
      GLuint target = r.handle(m_d->m_tex).handle();
      auto & gfx = r.renderDriver().gfxDriver();
      auto * srcRenderContext = &gfx.renderContext(m_d->m_ownerThreadIndex);

      gfx.renderDriver(m_d->m_ownerThreadIndex).afterFlush().add(
            [r = srcRenderContext, target, weak, currentThreadIndex, targetContext] {
        std::shared_ptr<DxSharedTexture> self = weak.lock();
        if (!self)
          return;

        auto & wglCopyImageSubDataNV = self->m_d->m_ctx->wglCopyImageSubDataNV;
        if (!wglCopyImageSubDataNV) {
          PROC proc = wglGetProcAddress("wglCopyImageSubDataNV");
          if (!proc) {
            self->m_d->m_ctx[currentThreadIndex].copyStatus = CopyStatus::Failed;
            return;
          }
          wglCopyImageSubDataNV = reinterpret_cast<WglCopyImageSubDataNV>(proc);
        }

        const Texture * tex = self->texture(*r, false, weak);
        if (!tex || !targetContext) {
          self->m_d->m_ctx[currentThreadIndex].copyStatus = CopyStatus::Failed;
          return;
        }

        Luminous::TextureGL & src = r->handle(*tex);

        self->ref(r->frameTime());
        BOOL ok = wglCopyImageSubDataNV(0, src.handle(), GL_TEXTURE_2D, 0,
                                        0, 0, 0,
                                        targetContext, target, GL_TEXTURE_2D, 0,
                                        0, 0, 0,
                                        self->m_d->m_tex.width(), self->m_d->m_tex.height(), 1);
        self->unref();
        /// @todo sync object?
        GLERROR("wglCopyImageSubDataNV");
        self->m_d->m_ctx[currentThreadIndex].copyStatus = ok ? CopyStatus::Finished : CopyStatus::Failed;
      });
    }

    return &m_d->m_tex;
  }

  /////////////////////////////////////////////////////////////////////////////

  class DxSharedTextureBag::D
  {
  public:
    void cleanOldTextures();

  public:
    Radiant::Mutex m_texturesMutex;
    std::vector<std::shared_ptr<DxSharedTexture>> m_textures;
  };

  /////////////////////////////////////////////////////////////////////////////

  void DxSharedTextureBag::D::cleanOldTextures()
  {
    Radiant::Guard g(m_texturesMutex);
    /// Always keep the latest texture alive
    if (m_textures.size() <= 1)
      return;

    const Nimble::SizeI size = m_textures.back()->size();
    const Radiant::TimeStamp now = Radiant::TimeStamp::currentTime();
    const double timeout = 3.0; // seconds

    /// Delete old textures, if:
    /// * they are not in use, and
    /// * have different size or haven't been used in a while
    for (size_t i = 0, s = m_textures.size() - 1; i < s;) {
      auto & ptr = m_textures[i];
      if (!ptr->release(false)) {
        ++i;
        continue;
      }

      if (ptr->size() != size || (now - ptr->lastUsed()).secondsD() >= timeout) {
        m_textures.erase(m_textures.begin() + i);
        --s;
      } else {
        ++i;
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////

  DxSharedTextureBag::DxSharedTextureBag()
    : m_d(new D())
  {
  }

  DxSharedTextureBag::~DxSharedTextureBag()
  {
  }

  void DxSharedTextureBag::addSharedHandle(void * sharedHandle)
  {
    bool found = false;
    {
      Radiant::Guard g(m_d->m_texturesMutex);
      for (auto it = m_d->m_textures.begin(); it != m_d->m_textures.end(); ++it) {
        if (CompareObjectHandles((*it)->sharedHandle(), sharedHandle)) {
          std::shared_ptr<DxSharedTexture> tex = std::move(*it);
          m_d->m_textures.erase(it);
          tex->acquire();
          found = true;
          m_d->m_textures.push_back(std::move(tex));
          break;
        }
      }
    }

    if (!found) {
      auto tex = std::make_shared<DxSharedTexture>();
      bool ok = tex->init(sharedHandle);
      if (ok) {
        Radiant::Guard g(m_d->m_texturesMutex);
        m_d->m_textures.push_back(std::move(tex));
      }
    }

    m_d->cleanOldTextures();
  }

  const Texture * DxSharedTextureBag::texture(RenderContext & r)
  {
    Radiant::Guard g(m_d->m_texturesMutex);
    if (m_d->m_textures.empty())
      return nullptr;

    bool first = true;
    for (auto it = m_d->m_textures.rbegin(); it != m_d->m_textures.rend(); ++it) {
      std::shared_ptr<DxSharedTexture> & dx = *it;
      const Texture * tex = dx->texture(r, first, dx);
      first = false;
      if (tex) {
        dx->ref(r.frameTime());
        r.renderDriver().afterFlush().add([dx] () mutable {
          dx->unref();
          dx.reset();
        });
        return tex;
      }
    }

    return nullptr;
  }
}
