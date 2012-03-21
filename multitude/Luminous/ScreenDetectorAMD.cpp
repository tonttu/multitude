#include "ScreenDetectorAMD.hpp"

#include <Radiant/Mutex.hpp>
#include <Radiant/Trace.hpp>

#ifdef RADIANT_LINUX
/// ADL uses this
#define LINUX

#include "XRandR.hpp"
#endif

#include <adl_sdk.h>

#include <QLibrary>

#include <set>
#include <map>

namespace
{
  /// Function to initialize the ADL interface. This function should be called first.
  /// [in] callback
  ///      The memory allocation function for memory buffer allocation.
  ///      This must be provided by the user.
  /// [in] iEnumConnectedAdapters
  ///      Specify a value of 0 to retrieve adapter information for all adapters
  ///      that have ever been present in the system. Specify a value of 1 to
  ///      retrieve adapter information only for adapters that are physically
  ///      present and enabled in the system.
  typedef int (*ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK, int);

  /// Function to destroy ADL global pointers. This function should be called last.
  typedef int (*ADL_MAIN_CONTROL_DESTROY)();

  /// Function to refresh adapter information. This function generates an
  /// adapter index value for all logical adapters that have ever been
  /// present in the system.
  typedef int (*ADL_MAIN_CONTROL_REFRESH)();

  /// Function to retrieve the number of OS-known adapters.
  /// [out] lpNumAdapters The pointer to the number of OS-known adapters.
  typedef int (*ADL_ADAPTER_NUMBEROFADAPTERS_GET)(int *);

  /// Function to determine if the adapter is active or not.
  /// [in]  iAdapterIndex
  ///       The ADL index handle of the desired adapter.
  /// [out] lpStatus
  ///       The pointer to the retrieved status.
  ///       ADL_TRUE : Active; ADL_FALSE : Disabled.
  typedef int (*ADL_ADAPTER_ACTIVE_GET)(int, int *);

  /// Retrieves all OS-known adapter information.
  /// [in]  iInputSize
  ///       The size of the lpInfo buffer.
  /// [out] lpInfo
  ///       The pointer to the buffer containing the retrieved adapter information.
  typedef int (*ADL_ADAPTER_ADAPTERINFO_GET)(LPAdapterInfo, int);

  /// Function to retrieve the adapter display information.
  /// [in]  iAdapterIndex
  ///       The ADL index handle of the desired adapter. A value of -1 returns
  ///       all displays in the system across multiple GPUs.
  /// [out] lpNumDisplays
  ///       The pointer to the number of displays detected.
  /// [out] lppInfo
  ///       The pointer to the pointer to the retrieved display information
  ///       array. Initialize to NULL before calling this API. Refer to the
  ///       ADLDisplayInfo structure for more information.
  /// [in]  iForceDetect
  ///       0: Do not force detection of the adapters in the system
  ///       1 : Force detection
  typedef int (*ADL_DISPLAY_DISPLAYINFO_GET)(int, int *, ADLDisplayInfo **, int);

  /// Function to get Device Display Position.
  /// [in]  iAdapterIndex, The ADL index handle of the desired adapter.
  /// [in]  iDisplayIndex, The desired display index. It can be retrieved from the ADLDisplayInfo data structure.
  /// [out] lpX, The pointer to the current X coordinate display position.
  /// [out] lpY, The pointer to the current Y coordinate display position.
  /// [out] lpXDefault, The pointer to the default X coordinate display position.
  /// [out] lpYDefault, The pointer to the default Y coordinate display position.
  /// [out] lpMinX, The pointer to the minimum X display size.
  /// [out] lpMinY, The pointer to the minimum Y display size.
  /// [out] lpMaxX, The pointer to the maximum X display size.
  /// [out] lpMaxY, The pointer to the maximum Y display size.
  /// [out] lpStepX, The pointer to the step size along the X axis.
  /// [out] lpStepY, The pointer to the step size along the Y axis.
  typedef int (*ADL_DISPLAY_POSITION_GET)(int, int, int *, int *, int *, int *, int *,
                                          int *, int *, int *, int *, int *);

  /// Function to get the Device Display Size.
  /// [in]  iAdapterIndex, The ADL index handle of the desired adapter.
  /// [in]  iDisplayIndex, The desired display index. It can be retrieved from the ADLDisplayInfo data structure.
  /// [out] lpWidth, The pointer to the current display width.
  /// [out] lpHeight, The pointer to the current display height.
  /// [out] lpDefaultWidth, The pointer to the default display width.
  /// [out] lpDefaultHeight, The pointer to the default display height.
  /// [out] lpMinWidth, The pointer to the minimum display width.
  /// [out] lpMinHeight, The pointer to the minimum display height.
  /// [out] lpMaxWidth, The pointer to the maximum display width.
  /// [out] lpMaxHeight, The pointer to the maximum display height.
  /// [out] lpStepWidth, The pointer to the step width.
  /// [out] lpStepHeight, The pointer to the step height.
  typedef int (*ADL_DISPLAY_SIZE_GET)(int, int, int *, int *, int *, int *,
                                       int *, int *, int *, int *, int *, int *);

#ifdef RADIANT_LINUX
  /// Function to retrieve all X Screen information for all OS-known adapters.
  /// [in]  iInputSize
  ///       The size of lpInfo buffer.
  /// [out] lpXScreenInfo
  ///       The pointer to the buffer storing the retrieved X Screen information.
  typedef int (*ADL_ADAPTER_XSCREENINFO_GET)(LPXScreenInfo, int);

  /// Function to get the Desktop Configuration.
  /// @see ADL_DESKTOPCONFIG_UNKNOWN etc
  /// [in]	iAdapterIndex, The ADL index handle of the desired adapter.
  /// [out]	lpDesktopConfig, The pointer to the retrieved desktop configuration information.
  typedef int (*ADL_DESKTOPCONFIG_GET)(int, int *);

  /// Function to retrieve the name of the Xrandr display.
  /// [in]  iAdapterIndex, The ADL index handle of the desired adapter.
  /// [in]	iDisplayIndex, The ADL index handle of the desired display.
  /// [out]	lpXrandrDisplayName, The pointer to the buffer storing the retrieved Xrandr display name.
  /// [in]	iBuffSize, The size of the lpXrandrDisplayName buffer.
  typedef int (*ADL_DISPLAY_XRANDRDISPLAYNAME_GET)(int, int, char *, int);
#endif

  ADL_MAIN_CONTROL_CREATE ADL_Main_Control_Create = 0;
  ADL_MAIN_CONTROL_DESTROY ADL_Main_Control_Destroy = 0;
  ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get = 0;
  ADL_MAIN_CONTROL_REFRESH ADL_Main_Control_Refresh = 0;
  ADL_ADAPTER_ACTIVE_GET ADL_Adapter_Active_Get = 0;
  ADL_DISPLAY_DISPLAYINFO_GET ADL_Display_DisplayInfo_Get = 0;
  ADL_ADAPTER_ADAPTERINFO_GET ADL_Adapter_AdapterInfo_Get = 0;
  ADL_DISPLAY_POSITION_GET ADL_Display_Position_Get = 0;
  ADL_DISPLAY_SIZE_GET ADL_Display_Size_Get = 0;

#ifdef RADIANT_LINUX
  ADL_ADAPTER_XSCREENINFO_GET ADL_Adapter_XScreenInfo_Get = 0;
  ADL_DESKTOPCONFIG_GET ADL_DesktopConfig_Get = 0;
  ADL_DISPLAY_XRANDRDISPLAYNAME_GET ADL_Display_XrandrDisplayName_Get = 0;
#endif

  bool s_ok = false;
}

bool operator<(const ADLDisplayID & a, const ADLDisplayID & b)
{
  return memcmp(&a, &b, sizeof(ADLDisplayID)) < 0;
}

namespace
{
  void initADL()
  {
    QLibrary lib("atiadlxx");
    if(!lib.load()) {
      Radiant::info("AMD screen detector is disabled: %s", lib.errorString().toUtf8().data());
      return;
    }

    ADL_Main_Control_Create = (ADL_MAIN_CONTROL_CREATE) lib.resolve("ADL_Main_Control_Create");
    ADL_Main_Control_Destroy = (ADL_MAIN_CONTROL_DESTROY) lib.resolve("ADL_Main_Control_Destroy");
    ADL_Adapter_NumberOfAdapters_Get = (ADL_ADAPTER_NUMBEROFADAPTERS_GET) lib.resolve("ADL_Adapter_NumberOfAdapters_Get");
    ADL_Main_Control_Refresh = (ADL_MAIN_CONTROL_REFRESH) lib.resolve("ADL_Main_Control_Refresh");
    ADL_Adapter_Active_Get = (ADL_ADAPTER_ACTIVE_GET) lib.resolve("ADL_Adapter_Active_Get");
    ADL_Display_DisplayInfo_Get = (ADL_DISPLAY_DISPLAYINFO_GET) lib.resolve("ADL_Display_DisplayInfo_Get");
    ADL_Adapter_AdapterInfo_Get = (ADL_ADAPTER_ADAPTERINFO_GET) lib.resolve("ADL_Adapter_AdapterInfo_Get");
    ADL_Display_Position_Get = (ADL_DISPLAY_POSITION_GET) lib.resolve("ADL_Display_Position_Get");
    ADL_Display_Size_Get = (ADL_DISPLAY_SIZE_GET) lib.resolve("ADL_Display_Size_Get");

#ifdef RADIANT_LINUX
    ADL_Adapter_XScreenInfo_Get = (ADL_ADAPTER_XSCREENINFO_GET) lib.resolve("ADL_Adapter_XScreenInfo_Get");
    ADL_DesktopConfig_Get = (ADL_DESKTOPCONFIG_GET) lib.resolve("ADL_DesktopConfig_Get");
    ADL_Display_XrandrDisplayName_Get = (ADL_DISPLAY_XRANDRDISPLAYNAME_GET) lib.resolve("ADL_Display_XrandrDisplayName_Get");
#endif

    if(ADL_Main_Control_Create && ADL_Main_Control_Destroy
       && ADL_Adapter_NumberOfAdapters_Get && ADL_Main_Control_Refresh
       && ADL_Adapter_Active_Get && ADL_Display_DisplayInfo_Get
       && ADL_Adapter_AdapterInfo_Get && ADL_Display_Position_Get
       && ADL_Display_Size_Get
 #ifdef RADIANT_LINUX
       && ADL_Adapter_XScreenInfo_Get && ADL_DesktopConfig_Get
       && ADL_Display_XrandrDisplayName_Get
 #endif
      ) s_ok = true;
  }

  void * adlAlloc(int size)
  {
    return operator new(size);
  }

  void adlFree(void * ptr)
  {
    operator delete(ptr);
  }
}

namespace Luminous
{
  ScreenDetectorAMD::ScreenDetectorAMD()
  {
    MULTI_ONCE(initADL();)
  }

  bool ScreenDetectorAMD::detect(int screen, QList<ScreenInfo> & results)
  {
    if(!s_ok) return false;

    if(ADL_Main_Control_Create(adlAlloc, 1) != ADL_OK) {
      Radiant::error("ADL Initialization error");
      return false;
    }

    /// @todo What does this actually do?
    ADL_Main_Control_Refresh();

    // At least on Linux, this isn't the number of adapters, it's the number of
    // total outputs in the system. For example this is 12 if you have two
    // Eyefinity cards connected.
    int adapterCount = 0;
    if(ADL_Adapter_NumberOfAdapters_Get(&adapterCount) != ADL_OK || adapterCount <= 0) {
      ADL_Main_Control_Destroy();
      return false;
    }

    // Because we can't trust the adapter count or adapter indices, we try to
    // determine the actual physical GPUs in the system with UDIDs (unique device id)
    std::map<std::string, int> udidToIndex;
    std::vector<int> activeAdapters;
    std::vector<AdapterInfo> adapterInfoList(adapterCount);
    memset(&adapterInfoList[0], 0, sizeof(AdapterInfo) * adapterInfoList.size());

    /// @todo It is unknown if iSize is required to be set before calling (maybe it should be zero?)
    for(std::size_t adapter = 0; adapter < adapterInfoList.size(); ++adapter)
      adapterInfoList[adapter].iSize = sizeof(AdapterInfo);

    int err = ADL_Adapter_AdapterInfo_Get(&adapterInfoList[0], int(adapterInfoList.size() * sizeof(AdapterInfo)));
    if(err != ADL_OK) {
      Radiant::error("ScreenDetectorAMD::detect # ADL_Adapter_AdapterInfo_Get: %d", err);
      ADL_Main_Control_Destroy();
      return false;
    }

#ifdef RADIANT_LINUX
    std::vector<XScreenInfo> xscreens(adapterInfoList.size());
    memset(&xscreens[0], 0, sizeof(XScreenInfo) * xscreens.size());

    err = ADL_Adapter_XScreenInfo_Get(&xscreens[0], sizeof(XScreenInfo) * xscreens.size());
    if(err != ADL_OK) {
      Radiant::error("ScreenDetectorAMD::detect # ADL_Adapter_XScreenInfo_Get: %d", err);
      ADL_Main_Control_Destroy();
      return false;
    }
#endif

    for(std::size_t i = 0; i < adapterInfoList.size(); ++i) {
      AdapterInfo & adapterInfo = adapterInfoList[i];

      if(udidToIndex.count(adapterInfo.strUDID) == 0) {
        int adapter = (int)udidToIndex.size();
        udidToIndex[adapterInfo.strUDID] = adapter;
      }

#ifdef RADIANT_LINUX
      if(xscreens[adapterInfo.iAdapterIndex].iXScreenNum != screen) continue;
#endif

      int status = ADL_FALSE;
      if(ADL_Adapter_Active_Get(adapterInfo.iAdapterIndex, &status) == ADL_OK &&
         status == ADL_TRUE)
        activeAdapters.push_back(adapterInfo.iAdapterIndex);
    }

    if(activeAdapters.empty()) {
      ADL_Main_Control_Destroy();
      return false;
    }

    // Since we get lots of duplicate displays, we need to filter them with this
    std::set<ADLDisplayID> seenDisplays;

    for(std::size_t i = 0; i < activeAdapters.size(); ++i) {
      AdapterInfo & adapterInfo = adapterInfoList[activeAdapters[i]];

      int displayCount = 0;
      ADLDisplayInfo * lst = 0;
      err = ADL_Display_DisplayInfo_Get(adapterInfo.iAdapterIndex, &displayCount, &lst, 1);
      if(err != ADL_OK) {
        Radiant::error("ScreenDetectorAMD::detect # ADL_Display_DisplayInfo_Get: %d", err);
        continue;
      }

      int desktopConfig = 0;
#ifdef RADIANT_LINUX
      err = ADL_DesktopConfig_Get(adapterInfo.iAdapterIndex, &desktopConfig);
      if(err != ADL_OK) {
        adlFree(lst);
        Radiant::error("ScreenDetectorAMD::detect # ADL_DesktopConfig_Get: %d", err);
        continue;
      }
#endif

      for(int j = 0; j < displayCount; ++j) {
        ADLDisplayInfo & displayInfo = lst[j];

        if(seenDisplays.count(displayInfo.displayID) > 0) continue;
        seenDisplays.insert(displayInfo.displayID);

        // Don't care about disconnected outputs
        if((ADL_DISPLAY_DISPLAYINFO_DISPLAYCONNECTED & displayInfo.iDisplayInfoValue) == 0) continue;

        ScreenInfo screenInfo;
        screenInfo.setName(displayInfo.strDisplayName);
        screenInfo.setGpu("GPU-"+QString::number(udidToIndex[adapterInfo.strUDID]));
        screenInfo.setGpuName(adapterInfo.strAdapterName);

        QString type;
        if(displayInfo.iDisplayType == ADL_DT_MONITOR)
          type = "Monitor-";
        else if(displayInfo.iDisplayType == ADL_DT_TELEVISION)
          type = "TV-";
        else if(displayInfo.iDisplayType == ADL_DT_LCD_PANEL)
          type = "LCD-";
        else if(displayInfo.iDisplayType == ADL_DT_DIGITAL_FLAT_PANEL)
          type = "DFP-";
        else if(displayInfo.iDisplayType == ADL_DT_COMPONENT_VIDEO)
          type = "Component-";
        else if(displayInfo.iDisplayType == ADL_DT_PROJECTOR)
          type = "Projector-";

        screenInfo.setConnection(type + QString::number(displayInfo.displayID.iDisplayLogicalIndex));
        screenInfo.setLogicalScreen(screen);

        if(desktopConfig != ADL_DESKTOPCONFIG_RANDR12) {

        int lpX = 0, lpY = 0, lpXDefault = 0, lpYDefault = 0, lpMinX = 0,
            lpMinY = 0, lpMaxX = 0, lpMaxY = 0, lpStepX = 0, lpStepY = 0;
        err = ADL_Display_Position_Get(adapterInfo.iAdapterIndex,
                                       displayInfo.displayID.iDisplayLogicalIndex,
                                       &lpX, &lpY, &lpXDefault, &lpYDefault,
                                       &lpMinX, &lpMinY, &lpMaxX, &lpMaxY,
                                       &lpStepX, &lpStepY);
        if(err != ADL_OK) {
          Radiant::error("ScreenDetectorAMD::detect # ADL_Display_Position_Get: %d", err);
          continue;
        }

        int lpWidth = 0, lpHeight = 0, lpDefaultWidth = 0, lpDefaultHeight = 0,
            lpMinWidth = 0, lpMinHeight = 0, lpMaxWidth = 0, lpMaxHeight = 0,
            lpStepWidth = 0, lpStepHeight = 0;
        err = ADL_Display_Size_Get(adapterInfo.iAdapterIndex,
                                   displayInfo.displayID.iDisplayLogicalIndex,
                                   &lpWidth, &lpHeight, &lpDefaultWidth,
                                   &lpDefaultHeight, &lpMinWidth, &lpMinHeight,
                                   &lpMaxWidth, &lpMaxHeight, &lpStepWidth, &lpStepHeight);
        if(err != ADL_OK) {
          Radiant::error("ScreenDetectorAMD::detect # ADL_Display_Size_Get: %d", err);
          continue;
        }

        screenInfo.setGeometry(Nimble::Recti(lpX, lpY, lpX+lpWidth, lpY+lpHeight));
        } else {
#ifdef RADIANT_LINUX
          char name[256];
          err = ADL_Display_XrandrDisplayName_Get(adapterInfo.iAdapterIndex,
                                                  displayInfo.displayID.iDisplayLogicalIndex,
                                                  name, sizeof(name));
          if(err != ADL_OK) {
            Radiant::error("ScreenDetectorAMD::detect # ADL_Display_XrandrDisplayName_Get: %d", err);
            continue;
          }
          Nimble::Recti rect;
          XRandR xrandr;
          if(xrandr.getGeometry(screen, name, rect)) {
            screenInfo.setGeometry(rect);
          } else {
            continue;
          }
#endif
        }
        results.push_back(screenInfo);
      }
      adlFree(lst);
    }

    ADL_Main_Control_Destroy();
    return true;
  }
}
