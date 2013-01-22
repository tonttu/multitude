#ifndef ADL_FUNCTIONS_H_
#define ADL_FUNCTIONS_H_

#include <Radiant/Platform.hpp>

#include <QLibrary>

/// ADL uses this
#if defined (RADIANT_LINUX)
#  define LINUX
#  include <Luminous/XRandR.hpp>
#endif // RADIANT_LINUX

#include <adl_sdk.h>

/// Function to initialize the ADL interface. This function should be called first.
/// [in] callback
///      The memory allocation function for memory buffer allocation.
///      This must be provided by the user.
/// [in] iEnumConnectedAdapters
///      Specify a value of 0 to retrieve adapter information for all adapters
///      that have ever been present in the system. Specify a value of 1 to
///      retrieve adapter information only for adapters that are physically
///      present and enabled in the system.
typedef int (*ADL_MAIN_CONTROL_CREATE)(ADL_MAIN_MALLOC_CALLBACK callback, int iEnumConnectedAdapters);

/// Function to destroy ADL global pointers. This function should be called last.
typedef int (*ADL_MAIN_CONTROL_DESTROY)();

/// Function to refresh adapter information. This function generates an
/// adapter index value for all logical adapters that have ever been
/// present in the system.
typedef int (*ADL_MAIN_CONTROL_REFRESH)();

/// Function to retrieve the number of OS-known adapters.
/// [out] lpNumAdapters
///       The pointer to the number of OS-known adapters.
typedef int (*ADL_ADAPTER_NUMBEROFADAPTERS_GET)(int *lpNumAdapters);

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
typedef int (*ADL_ADAPTER_ADAPTERINFO_GET)(LPAdapterInfo lpInfo, int iInputSize);

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
typedef int (*ADL_DISPLAY_DISPLAYINFO_GET)(int iAdapterIndex, int *lpNumDisplays, ADLDisplayInfo **lppInfo, int iForceDetect);

/// Function to get Device Display Position.
/// [in]  iAdapterIndex
///       The ADL index handle of the desired adapter.
/// [in]  iDisplayIndex
///       The desired display index. It can be retrieved from the ADLDisplayInfo data structure.
/// [out] lpX
///       The pointer to the current X coordinate display position.
/// [out] lpY
///       The pointer to the current Y coordinate display position.
/// [out] lpXDefault
///       The pointer to the default X coordinate display position.
/// [out] lpYDefault
///       The pointer to the default Y coordinate display position.
/// [out] lpMinX
///       The pointer to the minimum X display size.
/// [out] lpMinY
///       The pointer to the minimum Y display size.
/// [out] lpMaxX
///       The pointer to the maximum X display size.
/// [out] lpMaxY
///       The pointer to the maximum Y display size.
/// [out] lpStepX
///       The pointer to the step size along the X axis.
/// [out] lpStepY
///       The pointer to the step size along the Y axis.
typedef int (*ADL_DISPLAY_POSITION_GET)(int iAdapterIndex, int iDisplayIndex, int *lpX, int *lpY, int *lpXDefault, int *lpYDefault, int *lpMinX,
  int *lpMinY, int *lpMaxX, int *lpMaxY, int *lpStepX, int *lpStepY);

/// Function to retrieve the display mode information
/// [in]  iAdapterIndex
///       The ADL index handle of the desired adapter. A value of -1 retrieves all modes for the system across multiple GPUs.
/// [in]  iDisplayIndex
///       The desired display index. If the index is -1, this field is ignored
/// [out] lpNumModes
///       The pointer to the number of modes retrieved.
/// [out] lppModes
///       The pointer to the pointer to the retrieved display modes. Refer to the Display ADLMode structure for more information
typedef int (*ADL_DISPLAY_MODES_GET)(int iAdapterIndex, int iDisplayIndex, int *lpNumModes, ADLMode **lppModes);

/// Function to get the Device Display Size.
/// [in]  iAdapterIndex
///       The ADL index handle of the desired adapter.
/// [in]  iDisplayIndex
///       The desired display index. It can be retrieved from the ADLDisplayInfo data structure.
/// [out] lpWidth
///       The pointer to the current display width.
/// [out] lpHeight
///       The pointer to the current display height.
/// [out] lpDefaultWidth
///       The pointer to the default display width.
/// [out] lpDefaultHeight
///       The pointer to the default display height.
/// [out] lpMinWidth
///       The pointer to the minimum display width.
/// [out] lpMinHeight
///       The pointer to the minimum display height.
/// [out] lpMaxWidth
///       The pointer to the maximum display width.
/// [out] lpMaxHeight
///       The pointer to the maximum display height.
/// [out] lpStepWidth
///       The pointer to the step width.
/// [out] lpStepHeight
///       The pointer to the step height.
typedef int (*ADL_DISPLAY_SIZE_GET)(int iAdapterIndex, int iDisplayIndex, int *lpWidth, int *lpHeight, int *lpDefaultWidth, int *lpDefaultHeight,
  int *lpMinWidth, int *lpMinHeight, int *lpMaxWidth, int *lpMaxHeight, int *lpStepWidth, int *lpStepHeight);

#ifdef RADIANT_LINUX
/// Function to retrieve all X Screen information for all OS-known adapters.
/// [out] lpXScreenInfo
///       The pointer to the buffer storing the retrieved X Screen information.
/// [in]  iInputSize
///       The size of lpInfo buffer.
typedef int (*ADL_ADAPTER_XSCREENINFO_GET)(LPXScreenInfo lpXScreenInfo, int iInputSize);

/// Function to get the Desktop Configuration.
/// @see ADL_DESKTOPCONFIG_UNKNOWN etc
/// [in]	iAdapterIndex
///       The ADL index handle of the desired adapter.
/// [out]	lpDesktopConfig
///       The pointer to the retrieved desktop configuration information.
typedef int (*ADL_DESKTOPCONFIG_GET)(int iAdapterIndex, int *lpDesktopConfig);

/// Function to retrieve the name of the Xrandr display.
/// [in]  iAdapterIndex
///       The ADL index handle of the desired adapter.
/// [in]	iDisplayIndex
///       The ADL index handle of the desired display.
/// [out]	lpXrandrDisplayName
///       The pointer to the buffer storing the retrieved Xrandr display name.
/// [in]	iBuffSize
///       The size of the lpXrandrDisplayName buffer.
typedef int (*ADL_DISPLAY_XRANDRDISPLAYNAME_GET)(int iAdapterIndex, int iDisplayIndex, char *lpXrandrDisplayName, int iBuffSize);
#endif

/// Function to get the SLS map index for a given adapter and a given display device.
/// [in] 	iAdapterIndex
///       The ADL index of the desired adapter. This function does not support -1.
/// [in] 	iADLNumDisplayTarget
///       Specifies the number of input displays.
/// [in] 	lpDisplayTarget
///       Array of displays that are used to do the query. The array type is ADLDisplayTarget.
/// [out]	lpSLSMapIndex
///       Pointer to a variable that will receive the SLS map index. If the displays in an SLS map match the input displays, a valid SLS map index will be assigned to this parameter. Otherwise, -1 will be assigned to it.
typedef int (*ADL_DISPLAY_SLSMAPINDEX_GET)(int iAdapterIndex, int iADLNumDisplayTarget, ADLDisplayTarget *lpDisplayTarget, int *lpSLSMapIndex);


/// Function to retrieve an SLS configuration.
/// [in] 	iAdapterIndex
///       Specifies the adapter to be queried. This function does not support -1.
/// [in] 	iSLSMapIndex
///       Specifies the SLS map index to be queried.
/// [out] lpSLSMap
///       Pointer to a variable that contains the SLS map data. The data type is ADLSLSMap.
/// [out] lpNumSLSTarget
///       Pointer to variable that will receive the number of targets in the SLS map.
/// [out] lppSLSTarget
///       Pointer of a pointer to a variable that conations targets in the SLS map. the data type is ADLDisplayTarget Application does not need to allocate memory but it should free the pointer since the memory is allocated by ADL call back function.
/// [out] lpNumSLSNativeMode
///       Pointer to a variable that will receive the number of native SLS modes supported by the SLS configuration.
/// [out] lppSLSNativeMode
///       Pointer of a pointer to a variable that contains the native SLS modes. the data type is ADLSLSMode. Application does not need to allocate memory but it should free the pointer since the memory is allocated by ADL call back function.
/// [out] lpNumSLSBezelMode
///       Pointer to a variable that will receive the number of the bezel modes supported by the SLS configuration.
/// [out] lppSLSbezelMode
///       Pointer of a pointer to a variable that contains the bezel SLS modes. the data type is ADLSLSMode. Application does not need to allocate memory but it should free the pointer since the memory is allocated by ADL call back function.
/// [out] lpNumSLSTransientMode
///       Pointer to a variable that will receive the number of the transient modes supported by the SLS configuration.
/// [out] lppTransientMode
///       Pointer of a pointer to a variable that contains the transient SLS modes. the data type is ADLSLSMode. Application does not need to allocate memory but it should free the pointer since the memory is allocated by ADL call back function.
/// [out] lpNumSLSOffset
///       Pointer to a variable that will receive the number of the SLS offset supported by the SLS configuration.
/// [out] lppSLSOffset
///       Pointer of a pointer to a variable that contains the SLS offsets. the data type is ADLSLSOffset. Application does not need to allocate memory but it should free the pointer since the memory is allocated by ADL call back function.
/// [in] 	iOption
///       Specifies the layout type of SLS grid data. It is bit vector. There are two types of SLS layout:s, relative to landscape (ref ) and relative to current angle (ref ).
typedef int (*ADL_DISPLAY_SLSMAPCONFIG_GET)(int iAdapterIndex, int iSLSMapIndex, ADLSLSMap * lpSLSMap, int *lpNumSLSTarget, ADLSLSTarget **lppSLSTarget, int *lpNumSLSNativeMode, ADLSLSMode **lppSLSNativeMode, int *lpNumSLSBezelMode, ADLBezelTransientMode **lppSLSbezelMode, int *lpNumSLSTransientMode, ADLBezelTransientMode **lppTransientMode, int *lpNumSLSOffset, ADLSLSOffset **lppSLSOffset, int iOption);

/// Function to retrieve current display map configurations.
/// [in] 	iAdapterIndex
///       The ADL index handle of the desired adapter. A value of -1 returns all display configurations for the system across multiple GPUs.
/// [out]	lpNumDisplayMap
///       The pointer to the number of retrieved display maps.
/// [out]	lppDisplayMap
///       The pointer to the pointer to the display manner information. Refer to the ADLDisplayMap structure for more information.
/// [out]	lpNumDisplayTarget
///       The pointer to the display target sets retrieved.
/// [out]	lppDisplayTarget
///       The pointer to the pointer to the display target buffer. Refer to the ADLDisplayTarget structure for more information.
/// [in] 	iOptions
///       The function option. ADL_DISPLAY_DISPLAYMAP_OPTION_GPUINFO.
typedef int (*ADL_DISPLAY_DISPLAYMAPCONFIG_GET)(int iAdapterIndex, int * lpNumDisplayMap, ADLDisplayMap ** lppDisplayMap, int * lpNumDisplayTarget, ADLDisplayTarget ** lppDisplayTarget, int iOptions);


/// Function to indicate whether displays are physically connected to an adapter.
/// [in] 	iAdapterIndex
///       The ADL index handle of the desired adapter.
/// [out] lpConnections
///       The pointer to the bit field indicating whether the output connectors on the specified adapter have devices physically attached to them.
typedef int (*ADL_DISPLAY_CONNECTEDDISPLAYS_GET)(int iAdapterIndex, int * lpConnections);

/// Function to get the unique identifier of an adapter.
/// [in] 	iAdapterIndex
///       The ADL index handle of the desired adapter.
/// [out]	lpAdapterID
///       The pointer to the adapter identifier. Zero means: The adapter is not AMD.
typedef int (*ADL_ADAPTER_ID_GET)(int iAdapterIndex, int * lpAdapterID);

ADL_MAIN_CONTROL_CREATE ADL_Main_Control_Create = 0;
ADL_MAIN_CONTROL_DESTROY ADL_Main_Control_Destroy = 0;
ADL_ADAPTER_NUMBEROFADAPTERS_GET ADL_Adapter_NumberOfAdapters_Get = 0;
ADL_MAIN_CONTROL_REFRESH ADL_Main_Control_Refresh = 0;
ADL_ADAPTER_ACTIVE_GET ADL_Adapter_Active_Get = 0;
ADL_DISPLAY_DISPLAYINFO_GET ADL_Display_DisplayInfo_Get = 0;
ADL_ADAPTER_ADAPTERINFO_GET ADL_Adapter_AdapterInfo_Get = 0;
ADL_DISPLAY_POSITION_GET ADL_Display_Position_Get = 0;
ADL_DISPLAY_SIZE_GET ADL_Display_Size_Get = 0;
ADL_DISPLAY_CONNECTEDDISPLAYS_GET ADL_Display_ConnectedDisplays_Get = 0;
ADL_ADAPTER_ID_GET ADL_Adapter_ID_Get = 0;
#ifdef RADIANT_LINUX
ADL_ADAPTER_XSCREENINFO_GET ADL_Adapter_XScreenInfo_Get = 0;
ADL_DESKTOPCONFIG_GET ADL_DesktopConfig_Get = 0;
ADL_DISPLAY_XRANDRDISPLAYNAME_GET ADL_Display_XrandrDisplayName_Get = 0;
#endif
#ifdef RADIANT_WINDOWS
ADL_DISPLAY_MODES_GET ADL_Display_Modes_Get = 0;
ADL_DISPLAY_DISPLAYMAPCONFIG_GET ADL_Display_DisplayMapConfig_Get = 0;
ADL_DISPLAY_SLSMAPINDEX_GET ADL_Display_SLSMapIndex_Get = 0;
ADL_DISPLAY_SLSMAPCONFIG_GET ADL_Display_SLSMapConfig_Get = 0;
#endif

namespace
{
  bool initADL()
  {
    QLibrary lib("atiadlxx");
    // Try to load the 64-bit version of the library
    if(!lib.load()) {
      lib.setFileName("atiadlxy");
      // Try to load the 32-bit version (needed if running 32-bits app on 64-bits platform for instance)
      if (!lib.load())
        return false;
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
    ADL_Display_ConnectedDisplays_Get = (ADL_DISPLAY_CONNECTEDDISPLAYS_GET) lib.resolve("ADL_Display_ConnectedDisplays_Get");
    ADL_Adapter_ID_Get = (ADL_ADAPTER_ID_GET) lib.resolve("ADL_Adapter_ID_Get");
#ifdef RADIANT_WINDOWS
    ADL_Display_Modes_Get = (ADL_DISPLAY_MODES_GET) lib.resolve("ADL_Display_Modes_Get");
    ADL_Display_DisplayMapConfig_Get = (ADL_DISPLAY_DISPLAYMAPCONFIG_GET) lib.resolve("ADL_Display_DisplayMapConfig_Get");
    ADL_Display_SLSMapIndex_Get = (ADL_DISPLAY_SLSMAPINDEX_GET) lib.resolve("ADL_Display_SLSMapIndex_Get");
    ADL_Display_SLSMapConfig_Get = (ADL_DISPLAY_SLSMAPCONFIG_GET) lib.resolve("ADL_Display_SLSMapConfig_Get");
#endif
#ifdef RADIANT_LINUX
    ADL_Adapter_XScreenInfo_Get = (ADL_ADAPTER_XSCREENINFO_GET) lib.resolve("ADL_Adapter_XScreenInfo_Get");
    ADL_DesktopConfig_Get = (ADL_DESKTOPCONFIG_GET) lib.resolve("ADL_DesktopConfig_Get");
    ADL_Display_XrandrDisplayName_Get = (ADL_DISPLAY_XRANDRDISPLAYNAME_GET) lib.resolve("ADL_Display_XrandrDisplayName_Get");
#endif

    return (ADL_Main_Control_Create && ADL_Main_Control_Destroy
            && ADL_Adapter_NumberOfAdapters_Get && ADL_Main_Control_Refresh
            && ADL_Adapter_Active_Get && ADL_Display_DisplayInfo_Get
            && ADL_Adapter_AdapterInfo_Get && ADL_Display_Position_Get
            && ADL_Display_Size_Get && ADL_Display_ConnectedDisplays_Get
            && ADL_Adapter_ID_Get
#ifdef RADIANT_WINDOWS
            && ADL_Display_SLSMapIndex_Get && ADL_Display_SLSMapConfig_Get
            && ADL_Display_DisplayMapConfig_Get && ADL_Display_Modes_Get
#endif
#ifdef RADIANT_LINUX
            && ADL_Adapter_XScreenInfo_Get && ADL_DesktopConfig_Get
            && ADL_Display_XrandrDisplayName_Get
#endif
      );
  }
  
  void * __stdcall adlAlloc(int size)
  {
    return operator new(size);
  }

  void adlFree(void * ptr)
  {
    operator delete(ptr);
  }
}

#endif // ADL_FUNCTIONS_H_
