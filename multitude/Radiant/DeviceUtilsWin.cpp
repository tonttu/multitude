#include "DeviceUtilsWin.hpp"
#include "StringUtils.hpp"
#include "Trace.hpp"

#define NOMINMAX
#include <Windows.h>
#include <devguid.h>
#include <initguid.h>
#include <Setupapi.h>
#include <Devpkey.h>
#include <objbase.h>
#include <pciprop.h>

#include <QStringList>
#include <QSet>

// Checks if the given 'data' has any of the properties we are looking for,
// fills DeviceInfo, and if there is still something missing, looks at the
// parent node recursively
static void readProperties(Radiant::DeviceUtils::DeviceInfo & out, HDEVINFO & devinfo, SP_DEVINFO_DATA & data)
{
  DEVPROPTYPE type;
  if (out.bus == -1) {
    int32_t bus;
    if (SetupDiGetDeviceProperty(devinfo, &data, &DEVPKEY_Device_BusNumber, &type,
                                 (PBYTE)&bus, sizeof(bus), nullptr, 0)) {
      out.bus = bus;
    }
  }

  if (out.link == -1) {
    int32_t link;
    if (SetupDiGetDeviceProperty(devinfo, &data, &DEVPKEY_PciDevice_MaxLinkWidth, &type,
                                 (PBYTE)&link, sizeof(link), nullptr, 0)) {
      out.link = link;
    }
  }

  if (out.speed == -1) {
    int32_t speed;
    if (SetupDiGetDeviceProperty(devinfo, &data, &DEVPKEY_PciDevice_CurrentLinkSpeed, &type,
                                 (PBYTE)&speed, sizeof(speed), nullptr, 0)) {
      if (speed == DevProp_PciExpressDevice_LinkSpeed_TwoAndHalf_Gbps) {
        out.speed = 2500;
      } else if (speed == DevProp_PciExpressDevice_LinkSpeed_Five_Gbps) {
        out.speed = 5000;
      } else if (speed == 3) {
        out.speed = 8000;
      } else if (speed == 4) {
        out.speed = 16000;
      } else if (speed != 0) {
        Radiant::warning("DeviceUtils # Unknown PCIe speed %d", speed);
      }
    }
  }

  if (out.numaNode == -1) {
    int32_t numaProximityDomain;
    if (SetupDiGetDeviceProperty(devinfo, &data, &DEVPKEY_Numa_Proximity_Domain, &type,
                                 (PBYTE)&numaProximityDomain, sizeof(numaProximityDomain), nullptr, 0)) {
      unsigned char node;
      if (GetNumaProximityNode(numaProximityDomain, &node)) {
        out.numaNode = node;
      } else {
        Radiant::error("GetNumaProximityNode: %s", Radiant::StringUtils::getLastErrorMessage().toUtf8().data());
      }
    }
  }

  if (out.bus != -1 && out.link != -1 && out.numaNode != -1 && out.speed != -1)
    return;

  wchar_t parent[1024];
  if (SetupDiGetDeviceProperty(devinfo, &data, &DEVPKEY_Device_Parent, &type,
                               (PBYTE)&parent, sizeof(parent), nullptr, 0)) {
    /// @todo not entirely sure how device info lists work, would it be ok
    ///       just to reuse the existing devinfo?
    HDEVINFO devinfo2 = SetupDiCreateDeviceInfoList(nullptr, 0);

    SP_DEVINFO_DATA data2;
    data2.cbSize = sizeof(data2);
    if (SetupDiOpenDeviceInfo(devinfo2, parent, 0, 0, &data2)) {
      readProperties(out, devinfo2, data2);
    }

    SetupDiDestroyDeviceInfoList(devinfo2);
  }
}

namespace Radiant
{
  namespace DeviceUtils
  {
    DeviceInfo deviceInfo(const QString & deviceInstanceId,
                          const GUID * deviceClassGuid)
    {
      DeviceInfo info;
      HDEVINFO devinfo = SetupDiGetClassDevs(deviceClassGuid, nullptr, nullptr, DIGCF_PRESENT);

      SP_DEVINFO_DATA data;
      data.cbSize = sizeof(SP_DEVINFO_DATA);

      for (int i = 0;; ++i) {
        if (SetupDiEnumDeviceInfo(devinfo, i, &data)) {
          DEVPROPTYPE type;
          wchar_t buffer[1024];
          if (SetupDiGetDeviceProperty(devinfo, &data, &DEVPKEY_Device_InstanceId, &type,
                                       (PBYTE)&buffer, sizeof(buffer), nullptr, 0)) {
            if (deviceInstanceId != QString::fromWCharArray(buffer))
              continue;
          }

          readProperties(info, devinfo, data);
          break;
        } else break;
      }

      SetupDiDestroyDeviceInfoList(devinfo);
      return info;
    }

    DeviceInfo displayDeviceInfo(const QString & deviceInstanceId)
    {
      return deviceInfo(deviceInstanceId, &GUID_DEVCLASS_DISPLAY);
    }

    std::vector<int> DeviceUtils::cpuList(int numaNode)
    {
      std::vector<int> cpus;
      ULONGLONG mask = 0;
      if (GetNumaNodeProcessorMask(numaNode, &mask) == 0) {
        Radiant::error("NumaUtils::cpuList # %s", Radiant::StringUtils::getLastErrorMessage().toUtf8().data());
        return cpus;
      }
      for (unsigned int i = 0; i < sizeof(mask)*8; ++i) {
        if (mask & (ULONGLONG(1) << i)) {
          cpus.push_back(i);
        }
      }
      return cpus;
    }
  }

} // namespace Radiant
