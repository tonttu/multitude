#include "DisplayConfigWin.hpp"
#include "GPUAffinity.hpp"

#include <comdef.h>
#include <dxgi1_2.h>
#include <wrl.h>

#include <Radiant/Trace.hpp>

#include <QSettings>

namespace Luminous
{
  // https://www.khronos.org/opengl/wiki/Load_OpenGL_Functions#Windows
  static bool isValidProcAddress(void * addr)
  {
    if (addr == nullptr || (addr == (void*)0x1) || (addr == (void*)0x2) ||
        (addr == (void*)0x3) || (addr == (void*)-1)) {
      return false;
    }
    return true;
  }

  class GPUAffinity::D
  {
  public:
    DECLARE_HANDLE(HGPUNV);

    typedef struct _GPU_DEVICE {
      DWORD  cb;
      CHAR   DeviceName[32];
      CHAR   DeviceString[128];
      DWORD  Flags;
      RECT   rcVirtualScreen;
    } GPU_DEVICE, *PGPU_DEVICE;

    HDC (APIENTRY * wglCreateAffinityDCNV)(const HGPUNV *phGpuList) = nullptr;
    BOOL (APIENTRY * wglDeleteDCNV)(HDC hdc) = nullptr;
    BOOL (APIENTRY * wglEnumGpusNV)(uint iGpuIndex, HGPUNV *phGpu) = nullptr;
    BOOL (APIENTRY * wglEnumGpuDevicesNV)(HGPUNV hGpu, UINT iDeviceIndex, PGPU_DEVICE lpGpuDevice) = nullptr;

    struct Gpu
    {
      uint32_t index;
      HGPUNV handle;
      std::vector<GPU_DEVICE> devices;
    };

    std::vector<Gpu> gpus;
  };

  GPUAffinity::GPUAffinity()
    : m_d(new D())
  {
    m_d->wglCreateAffinityDCNV = reinterpret_cast<decltype(D::wglCreateAffinityDCNV)>(wglGetProcAddress("wglCreateAffinityDCNV"));
    m_d->wglDeleteDCNV = reinterpret_cast<decltype(D::wglDeleteDCNV)>(wglGetProcAddress("wglDeleteDCNV"));
    m_d->wglEnumGpusNV = reinterpret_cast<decltype(D::wglEnumGpusNV)>(wglGetProcAddress("wglEnumGpusNV"));
    m_d->wglEnumGpuDevicesNV = reinterpret_cast<decltype(D::wglEnumGpuDevicesNV)>(wglGetProcAddress("wglEnumGpuDevicesNV"));

    if (isSupported()) {
      for (unsigned int gpuIndex = 0;; ++gpuIndex) {
        D::Gpu gpu;
        gpu.index = gpuIndex;
        if (!m_d->wglEnumGpusNV(gpuIndex, &gpu.handle))
          break;

        for (unsigned int deviceIndex = 0;; ++deviceIndex) {
          D::GPU_DEVICE device;
          device.cb = sizeof(device);
          if (!m_d->wglEnumGpuDevicesNV(gpu.handle, deviceIndex, &device))
            break;
          gpu.devices.push_back(device);
        }
        m_d->gpus.push_back(std::move(gpu));
      }
    }
  }

  GPUAffinity::~GPUAffinity()
  {
  }

  bool GPUAffinity::isSupported() const
  {
    return isValidProcAddress(m_d->wglCreateAffinityDCNV) &&
        isValidProcAddress(m_d->wglDeleteDCNV) &&
        isValidProcAddress(m_d->wglEnumGpusNV) &&
        isValidProcAddress(m_d->wglEnumGpuDevicesNV);
  }

  std::vector<uint32_t> GPUAffinity::gpusForDesktopArea(QRect desktop) const
  {
    std::vector<uint32_t> out;
    std::vector<QRegion> areas = gpuDesktopAreas();
    for (uint32_t i = 0; i < areas.size(); ++i) {
      if (areas[i].intersects(desktop)) {
        out.push_back(i);
      }
    }

    return out;
  }

  std::vector<QRegion> GPUAffinity::gpuDesktopAreas() const
  {
    std::vector<QRegion> out(m_d->gpus.size());
    for (size_t i = 0; i < out.size(); ++i) {
      const D::Gpu & gpu = m_d->gpus[i];
      for (const D::GPU_DEVICE & device: gpu.devices) {
        if (device.Flags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
          QRect deviceRect(device.rcVirtualScreen.left, device.rcVirtualScreen.top,
                           device.rcVirtualScreen.right - device.rcVirtualScreen.left,
                           device.rcVirtualScreen.bottom - device.rcVirtualScreen.top);
          out[i] |= deviceRect;
        }
      }
    }
    return out;
  }

  QString GPUAffinity::gpuName(uint32_t index) const
  {
    if (index < m_d->gpus.size()) {
      const D::Gpu & gpu = m_d->gpus[index];
      for (const D::GPU_DEVICE & device: gpu.devices) {
        if (device.DeviceString[0])
          return device.DeviceString;
      }
    }
    return QString();
  }

  QStringList GPUAffinity::displayGdiDeviceNames(uint32_t gpuIndex) const
  {
    QStringList ret;
    if (gpuIndex < m_d->gpus.size()) {
      const D::Gpu & gpu = m_d->gpus[gpuIndex];
      for (const D::GPU_DEVICE & device: gpu.devices) {
        if (device.DeviceName[0])
          ret << device.DeviceName;
      }
    }
    return ret;
  }

  QStringList GPUAffinity::adapterInstanceIds(uint32_t gpuIndex) const
  {
    QStringList ret;
    QStringList gdiNames = displayGdiDeviceNames(gpuIndex);
    if (gdiNames.isEmpty())
      return ret;

    try {
      DisplayConfigWin cfg = DisplayConfigWin::currentConfig();

      for (const DisplayConfigWin::Output & output: cfg.outputs) {
        if (gdiNames.contains(output.sourceGdiDeviceName)) {
          QString instanceId = cfg.cleanInstanceId(output.adapterDevicePath);
          if (!ret.contains(instanceId))
            ret << instanceId;
        }
      }
    } catch (std::exception & error) {
      Radiant::error("Failed to read display config: %s", error.what());
    }
    return ret;
  }

  std::vector<LUID> GPUAffinity::dxgiAdapterLuids(uint32_t gpuIndex) const
  {
    std::vector<LUID> ret;
    for (const QString & instanceId: adapterInstanceIds(gpuIndex)) {
      QSettings enumSettings("HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Enum\\" + instanceId + "\\Device Parameters", QSettings::NativeFormat);
      QString videoId = enumSettings.value("VideoID").toString();
      if (videoId.isEmpty())
        continue;

      QSettings dxSettings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\DirectX\\" + videoId, QSettings::NativeFormat);
      qulonglong luid = dxSettings.value("AdapterLuid").toULongLong();
      if (luid) {
        LUID l{luid & 0xFFFFFFFF, (luid >> 32) & 0xFFFFFFFF};
        ret.push_back(l);
      }
    }
    return ret;
  }
} // namespace Luminous
