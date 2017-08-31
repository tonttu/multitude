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

#include <QMap>

#include <array>

static QByteArray keyName(DEVPROPKEY key)
{
#define DEVPKEY_TEST(name) if (key == name) return #name

  DEVPKEY_TEST(DEVPKEY_NAME);
  DEVPKEY_TEST(DEVPKEY_Device_DeviceDesc);
  DEVPKEY_TEST(DEVPKEY_Device_HardwareIds);
  DEVPKEY_TEST(DEVPKEY_Device_CompatibleIds);
  DEVPKEY_TEST(DEVPKEY_Device_Service);
  DEVPKEY_TEST(DEVPKEY_Device_Class);
  DEVPKEY_TEST(DEVPKEY_Device_ClassGuid);
  DEVPKEY_TEST(DEVPKEY_Device_Driver);
  DEVPKEY_TEST(DEVPKEY_Device_ConfigFlags);
  DEVPKEY_TEST(DEVPKEY_Device_Manufacturer);
  DEVPKEY_TEST(DEVPKEY_Device_FriendlyName);
  DEVPKEY_TEST(DEVPKEY_Device_LocationInfo);
  DEVPKEY_TEST(DEVPKEY_Device_PDOName);
  DEVPKEY_TEST(DEVPKEY_Device_Capabilities);
  DEVPKEY_TEST(DEVPKEY_Device_UINumber);
  DEVPKEY_TEST(DEVPKEY_Device_UpperFilters);
  DEVPKEY_TEST(DEVPKEY_Device_LowerFilters);
  DEVPKEY_TEST(DEVPKEY_Device_BusTypeGuid);
  DEVPKEY_TEST(DEVPKEY_Device_LegacyBusType);
  DEVPKEY_TEST(DEVPKEY_Device_BusNumber);
  DEVPKEY_TEST(DEVPKEY_Device_EnumeratorName);
  DEVPKEY_TEST(DEVPKEY_Device_Security);
  DEVPKEY_TEST(DEVPKEY_Device_SecuritySDS);
  DEVPKEY_TEST(DEVPKEY_Device_DevType);
  DEVPKEY_TEST(DEVPKEY_Device_Exclusive);
  DEVPKEY_TEST(DEVPKEY_Device_Characteristics);
  DEVPKEY_TEST(DEVPKEY_Device_Address);
  DEVPKEY_TEST(DEVPKEY_Device_UINumberDescFormat);
  DEVPKEY_TEST(DEVPKEY_Device_PowerData);
  DEVPKEY_TEST(DEVPKEY_Device_RemovalPolicy);
  DEVPKEY_TEST(DEVPKEY_Device_RemovalPolicyDefault);
  DEVPKEY_TEST(DEVPKEY_Device_RemovalPolicyOverride);
  DEVPKEY_TEST(DEVPKEY_Device_InstallState);
  DEVPKEY_TEST(DEVPKEY_Device_LocationPaths);
  DEVPKEY_TEST(DEVPKEY_Device_BaseContainerId);
  DEVPKEY_TEST(DEVPKEY_Device_InstanceId);
  DEVPKEY_TEST(DEVPKEY_Device_DevNodeStatus);
  DEVPKEY_TEST(DEVPKEY_Device_ProblemCode);
  DEVPKEY_TEST(DEVPKEY_Device_EjectionRelations);
  DEVPKEY_TEST(DEVPKEY_Device_RemovalRelations);
  DEVPKEY_TEST(DEVPKEY_Device_PowerRelations);
  DEVPKEY_TEST(DEVPKEY_Device_BusRelations);
  DEVPKEY_TEST(DEVPKEY_Device_Parent);
  DEVPKEY_TEST(DEVPKEY_Device_Children);
  DEVPKEY_TEST(DEVPKEY_Device_Siblings);
  DEVPKEY_TEST(DEVPKEY_Device_TransportRelations);
  DEVPKEY_TEST(DEVPKEY_Device_ProblemStatus);
  DEVPKEY_TEST(DEVPKEY_Device_Reported);
  DEVPKEY_TEST(DEVPKEY_Device_Legacy);
  DEVPKEY_TEST(DEVPKEY_Device_ContainerId);
  DEVPKEY_TEST(DEVPKEY_Device_InLocalMachineContainer);
  DEVPKEY_TEST(DEVPKEY_Device_Model);
  DEVPKEY_TEST(DEVPKEY_Device_ModelId);
  DEVPKEY_TEST(DEVPKEY_Device_FriendlyNameAttributes);
  DEVPKEY_TEST(DEVPKEY_Device_ManufacturerAttributes);
  DEVPKEY_TEST(DEVPKEY_Device_PresenceNotForDevice);
  DEVPKEY_TEST(DEVPKEY_Device_SignalStrength);
  DEVPKEY_TEST(DEVPKEY_Device_IsAssociateableByUserAction);
  DEVPKEY_TEST(DEVPKEY_Device_ShowInUninstallUI);
  DEVPKEY_TEST(DEVPKEY_Device_Numa_Proximity_Domain);
  DEVPKEY_TEST(DEVPKEY_Device_DHP_Rebalance_Policy);
  DEVPKEY_TEST(DEVPKEY_Device_Numa_Node);
  DEVPKEY_TEST(DEVPKEY_Device_BusReportedDeviceDesc);
  DEVPKEY_TEST(DEVPKEY_Device_IsPresent);
  DEVPKEY_TEST(DEVPKEY_Device_HasProblem);
  DEVPKEY_TEST(DEVPKEY_Device_ConfigurationId);
  DEVPKEY_TEST(DEVPKEY_Device_ReportedDeviceIdsHash);
  DEVPKEY_TEST(DEVPKEY_Device_PhysicalDeviceLocation);
  DEVPKEY_TEST(DEVPKEY_Device_BiosDeviceName);
  DEVPKEY_TEST(DEVPKEY_Device_DriverProblemDesc);
  DEVPKEY_TEST(DEVPKEY_Device_DebuggerSafe);
  DEVPKEY_TEST(DEVPKEY_Device_PostInstallInProgress);
  DEVPKEY_TEST(DEVPKEY_Device_Stack);
  DEVPKEY_TEST(DEVPKEY_Device_ExtendedConfigurationIds);
  DEVPKEY_TEST(DEVPKEY_Device_IsRebootRequired);
  DEVPKEY_TEST(DEVPKEY_Device_FirmwareDate);
  DEVPKEY_TEST(DEVPKEY_Device_FirmwareVersion);
  DEVPKEY_TEST(DEVPKEY_Device_FirmwareRevision);
  DEVPKEY_TEST(DEVPKEY_Device_DependencyProviders);
  DEVPKEY_TEST(DEVPKEY_Device_DependencyDependents);
  DEVPKEY_TEST(DEVPKEY_Device_SoftRestartSupported);
  DEVPKEY_TEST(DEVPKEY_Device_ExtendedAddress);
  DEVPKEY_TEST(DEVPKEY_Device_SessionId);
  DEVPKEY_TEST(DEVPKEY_Device_InstallDate);
  DEVPKEY_TEST(DEVPKEY_Device_FirstInstallDate);
  DEVPKEY_TEST(DEVPKEY_Device_LastArrivalDate);
  DEVPKEY_TEST(DEVPKEY_Device_LastRemovalDate);
  DEVPKEY_TEST(DEVPKEY_Device_DriverDate);
  DEVPKEY_TEST(DEVPKEY_Device_DriverVersion);
  DEVPKEY_TEST(DEVPKEY_Device_DriverDesc);
  DEVPKEY_TEST(DEVPKEY_Device_DriverInfPath);
  DEVPKEY_TEST(DEVPKEY_Device_DriverInfSection);
  DEVPKEY_TEST(DEVPKEY_Device_DriverInfSectionExt);
  DEVPKEY_TEST(DEVPKEY_Device_MatchingDeviceId);
  DEVPKEY_TEST(DEVPKEY_Device_DriverProvider);
  DEVPKEY_TEST(DEVPKEY_Device_DriverPropPageProvider);
  DEVPKEY_TEST(DEVPKEY_Device_DriverCoInstallers);
  DEVPKEY_TEST(DEVPKEY_Device_ResourcePickerTags);
  DEVPKEY_TEST(DEVPKEY_Device_ResourcePickerExceptions);
  DEVPKEY_TEST(DEVPKEY_Device_DriverRank);
  DEVPKEY_TEST(DEVPKEY_Device_DriverLogoLevel);
  DEVPKEY_TEST(DEVPKEY_Device_NoConnectSound);
  DEVPKEY_TEST(DEVPKEY_Device_GenericDriverInstalled);
  DEVPKEY_TEST(DEVPKEY_Device_AdditionalSoftwareRequested);
  DEVPKEY_TEST(DEVPKEY_Device_SafeRemovalRequired);
  DEVPKEY_TEST(DEVPKEY_Device_SafeRemovalRequiredOverride);
  DEVPKEY_TEST(DEVPKEY_DrvPkg_Model);
  DEVPKEY_TEST(DEVPKEY_DrvPkg_VendorWebSite);
  DEVPKEY_TEST(DEVPKEY_DrvPkg_DetailedDescription);
  DEVPKEY_TEST(DEVPKEY_DrvPkg_DocumentationLink);
  DEVPKEY_TEST(DEVPKEY_DrvPkg_Icon);
  DEVPKEY_TEST(DEVPKEY_DrvPkg_BrandingIcon);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_UpperFilters);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_LowerFilters);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_Security);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_SecuritySDS);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_DevType);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_Exclusive);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_Characteristics);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_Name);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_ClassName);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_Icon);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_ClassInstaller);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_PropPageProvider);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_NoInstallClass);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_NoDisplayClass);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_SilentInstall);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_NoUseClass);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_DefaultService);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_IconPath);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_DHPRebalanceOptOut);
  DEVPKEY_TEST(DEVPKEY_DeviceClass_ClassCoInstallers);
  DEVPKEY_TEST(DEVPKEY_DeviceInterface_FriendlyName);
  DEVPKEY_TEST(DEVPKEY_DeviceInterface_Enabled);
  DEVPKEY_TEST(DEVPKEY_DeviceInterface_ClassGuid);
  DEVPKEY_TEST(DEVPKEY_DeviceInterface_ReferenceString);
  DEVPKEY_TEST(DEVPKEY_DeviceInterface_Restricted);
  DEVPKEY_TEST(DEVPKEY_DeviceInterface_UnrestrictedAppCapabilities);
  DEVPKEY_TEST(DEVPKEY_DeviceInterfaceClass_DefaultInterface);
  DEVPKEY_TEST(DEVPKEY_DeviceInterfaceClass_Name);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_Address);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_DiscoveryMethod);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_IsEncrypted);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_IsAuthenticated);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_IsConnected);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_IsPaired);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_Icon);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_Version);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_Last_Seen);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_Last_Connected);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_IsShowInDisconnectedState);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_IsLocalMachine);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_MetadataPath);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_IsMetadataSearchInProgress);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_MetadataChecksum);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_IsNotInterestingForDisplay);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_LaunchDeviceStageOnDeviceConnect);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_LaunchDeviceStageFromExplorer);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_BaselineExperienceId);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_IsDeviceUniquelyIdentifiable);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_AssociationArray);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_DeviceDescription1);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_DeviceDescription2);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_HasProblem);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_IsSharedDevice);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_IsNetworkDevice);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_IsDefaultDevice);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_MetadataCabinet);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_RequiresPairingElevation);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_ExperienceId);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_Category);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_Category_Desc_Singular);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_Category_Desc_Plural);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_Category_Icon);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_CategoryGroup_Desc);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_CategoryGroup_Icon);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_PrimaryCategory);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_UnpairUninstall);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_RequiresUninstallElevation);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_DeviceFunctionSubRank);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_AlwaysShowDeviceAsConnected);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_ConfigFlags);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_PrivilegedPackageFamilyNames);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_CustomPrivilegedPackageFamilyNames);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_IsRebootRequired);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_FriendlyName);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_Manufacturer);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_ModelName);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_ModelNumber);
  DEVPKEY_TEST(DEVPKEY_DeviceContainer_InstallInProgress);
  DEVPKEY_TEST(DEVPKEY_DevQuery_ObjectType);
  return "Unknown DEVPKEY";
}

struct DeviceTmp
{
  QMap<QByteArray, QString> keys;
  std::vector<DeviceTmp*> children;
};

void build(std::vector<Radiant::DeviceUtils::Device> & list, DeviceTmp & tmp)
{
  list.emplace_back();
  Radiant::DeviceUtils::Device & dev = list.back();
  dev.keys = std::move(tmp.keys);

  for (DeviceTmp * child: tmp.children)
    build(dev.children, *child);
}

void dumpDev(const Radiant::DeviceUtils::Device & dev, QByteArray indent = "")
{
  Radiant::info("%s", indent.data());
  for (auto it = dev.keys.begin(), end = dev.keys.end(); it != end; ++it)
    Radiant::info("%s'%s': '%s'", indent.data(), it.key().data(), it.value().toUtf8().data());
  for (auto & child: dev.children)
    dumpDev(child, indent + "  ");
}

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
    if (SetupDiGetDeviceProperty(devinfo, &data, &DEVPKEY_PciDevice_MaxLinkSpeed, &type,
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

    void dump()
    {
      for (Device & dev: allDevices())
        dumpDev(dev);
    }

    std::vector<Device> allDevices()
    {
      QMap<QString, DeviceTmp> devices;

      HDEVINFO devinfo = SetupDiGetClassDevs(nullptr, nullptr, nullptr, DIGCF_ALLCLASSES | DIGCF_PRESENT);

      SP_DEVINFO_DATA data;
      data.cbSize = sizeof(SP_DEVINFO_DATA);

      for (int i = 0;; ++i) {
        if (SetupDiEnumDeviceInfo(devinfo, i, &data)) {
          QMap<QByteArray, QString> devKeys;
          std::vector<DEVPROPKEY> keys(1024);
          DWORD count = 0;
          if (SetupDiGetDevicePropertyKeys(devinfo, &data, keys.data(), keys.size(), &count, 0)) {
            for (DWORD j = 0; j < count; ++j) {
              DEVPROPKEY key = keys[j];
              DEVPROPTYPE type;
              std::array<uint8_t, 2048> buffer;

              if (SetupDiGetDeviceProperty(devinfo, &data, &key, &type, buffer.data(),
                                           buffer.size(), nullptr, 0)) {
                uint8_t* d = buffer.data();
                int t = DEVPROP_MASK_TYPE & type;
                QString value;
                if (t == DEVPROP_TYPE_EMPTY) value = "(empty)";
                else if (t == DEVPROP_TYPE_NULL) value = "(null)";
                else if (t == DEVPROP_TYPE_SBYTE) value = QString::number(*(int8_t*)d);
                else if (t == DEVPROP_TYPE_BYTE) value = QString::number(*(uint8_t*)d);
                else if (t == DEVPROP_TYPE_INT16) value = QString::number(*(int16_t*)d);
                else if (t == DEVPROP_TYPE_UINT16) value = QString::number(*(uint16_t*)d);
                else if (t == DEVPROP_TYPE_INT32) value = QString::number(*(int32_t*)d);
                else if (t == DEVPROP_TYPE_UINT32) value = QString::number(*(int64_t*)d);
                else if (t == DEVPROP_TYPE_INT64) value = QString::number(*(int64_t*)d);
                else if (t == DEVPROP_TYPE_UINT64) value = QString::number(*(uint64_t*)d);
                else if (t == DEVPROP_TYPE_FLOAT) value = QString::number(*(float*)d);
                else if (t == DEVPROP_TYPE_DOUBLE) value = QString::number(*(double*)d);
                else if (t == DEVPROP_TYPE_GUID) {
                  GUID * guid = (GUID*)d;
                  wchar_t str[128];
                  StringFromGUID2(*guid, str, 128);
                  value = QString::fromWCharArray(str);
                }
                else if (t == DEVPROP_TYPE_STRING) value = QString::fromWCharArray((wchar_t*)d);
                else if (t == DEVPROP_TYPE_BOOLEAN) value = *(bool*)d ? "true" : "false;";
                else if (t == DEVPROP_TYPE_FILETIME) {
                  FILETIME* time = (FILETIME*)d;
                  SYSTEMTIME s;
                  FileTimeToSystemTime(time, &s);
                  value = QString("%1-%2-%3 %4:%5:%6.%7").arg(s.wYear).arg(s.wMonth).arg(s.wDay).
                      arg(s.wHour).arg(s.wMinute).arg(s.wSecond).arg(s.wMilliseconds);
                }
                else if (t == DEVPROP_TYPE_DECIMAL) value = "<DEVPROP_TYPE_DECIMAL>";
                else if (t == DEVPROP_TYPE_CURRENCY) value = "<DEVPROP_TYPE_CURRENCY>";
                else if (t == DEVPROP_TYPE_DATE) value = "<DEVPROP_TYPE_DATE>";
                else if (t == DEVPROP_TYPE_STRING_LIST) value = "<DEVPROP_TYPE_STRING_LIST>";
                else if (t == DEVPROP_TYPE_SECURITY_DESCRIPTOR) value = "<DEVPROP_TYPE_SECURITY_DESCRIPTOR>";
                else if (t == DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING) value = "<DEVPROP_TYPE_SECURITY_DESCRIPTOR_STRING>";
                else if (t == DEVPROP_TYPE_DEVPROPKEY) value = "<DEVPROP_TYPE_DEVPROPKEY>";
                else if (t == DEVPROP_TYPE_DEVPROPTYPE) value = "<DEVPROP_TYPE_DEVPROPTYPE>";
                else if (t == DEVPROP_TYPE_BINARY) value = "<DEVPROP_TYPE_BINARY>";
                else if (t == DEVPROP_TYPE_ERROR) value = "<DEVPROP_TYPE_ERROR>";
                else if (t == DEVPROP_TYPE_NTSTATUS) value = "<DEVPROP_TYPE_NTSTATUS>";
                else if (t == DEVPROP_TYPE_STRING_INDIRECT) value = "<DEVPROP_TYPE_STRING_INDIRECT>";
                devKeys[keyName(key)] = value;
              } else {
                devKeys[keyName(key)] = QString();
              }
            }
          } else {
            Radiant::error("DeviceUtils::allDevices # SetupDiGetDevicePropertyKeys failed");
          }

          if (!devKeys.isEmpty()) {
            DeviceTmp & dev = devices[devKeys["DEVPKEY_Device_InstanceId"]];
            dev.keys = std::move(devKeys);
          }
        } else {
          break;
        }
      }

      SetupDiDestroyDeviceInfoList(devinfo);

      for (DeviceTmp & dev: devices) {
        DeviceTmp & parent = devices[dev.keys["DEVPKEY_Device_Parent"]];
        if (&parent == &dev) continue;

        parent.children.push_back(&dev);
      }

      std::vector<Device> ret;
      DeviceTmp & root = devices[QByteArray()];
      for (DeviceTmp * tmp: root.children) {
        build(ret, *tmp);
      }

      return ret;
    }

  }

} // namespace Radiant
