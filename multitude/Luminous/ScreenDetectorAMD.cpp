#include "ScreenDetectorAMD.hpp"

#include <Radiant/Platform.hpp>
#include <Radiant/Mutex.hpp>
#include <Radiant/Trace.hpp>

#if defined (RADIANT_WINDOWS)
#  include <windows.h>
#elif defined (RADIANT_WINDOWS)
#  include "XRandR.hpp"
#elif defined (RADIONT_OSX)
#  error "Not supported on OSX"
#endif

#include <adl_functions.h>

#include <vector>
#include <algorithm>
#include <set>
#include <map>

// Compare display modes
bool operator==(const ADLMode & lhs, const ADLMode & rhs) { return (lhs.iXRes == rhs.iXRes && lhs.iYRes == rhs.iYRes); }
template <typename T> bool operator==(const ADLMode & lhs, const T & rhs) { return lhs == rhs.displayMode; }
template <typename T> bool operator==(const T & lhs, const ADLMode & rhs) { return lhs.displayMode == rhs; }

// Compare display targets
bool operator==(const ADLDisplayTarget & lhs, const ADLDisplayTarget & rhs) { return ( lhs.displayID.iDisplayLogicalIndex == rhs.displayID.iDisplayLogicalIndex ); }
template <typename T> bool operator==(const ADLDisplayTarget & lhs, const T & rhs) { return lhs == rhs.displayTarget; }
template <typename T> bool operator==(const T & lhs, const ADLDisplayTarget & rhs) { return lhs.displayTarget == rhs; }

// Compare display IDs
bool operator<(const ADLDisplayID & a, const ADLDisplayID & b) { return memcmp(&a, &b, sizeof(ADLDisplayID)) < 0; }

namespace {
  // Error checking for ADL functions
  bool checkADL(const std::string & msg, int err)
  {
    switch (err)
    {
    case ADL_ERR:                         Radiant::error("ScreenDetectorAMD::detect # %s: Generic error (%d)", msg.c_str(), err); break;
    case ADL_ERR_NOT_INIT:                Radiant::error("ScreenDetectorAMD::detect # %s: ADL not initialized (%d)", msg.c_str(), err); break;
    case ADL_ERR_INVALID_PARAM:           Radiant::error("ScreenDetectorAMD::detect # %s: Invalid parameter (%d)", msg.c_str(), err); break;
    case ADL_ERR_INVALID_PARAM_SIZE:      Radiant::error("ScreenDetectorAMD::detect # %s: Invalid parameter size (%d)", msg.c_str(), err); break;
    case ADL_ERR_INVALID_ADL_IDX:         Radiant::error("ScreenDetectorAMD::detect # %s: Invalid ADL index (%d)", msg.c_str(), err); break;
    case ADL_ERR_INVALID_CONTROLLER_IDX:  Radiant::error("ScreenDetectorAMD::detect # %s: Invalid controller index (%d)", msg.c_str(), err); break;
    case ADL_ERR_INVALID_DIPLAY_IDX:      Radiant::error("ScreenDetectorAMD::detect # %s: Invalid display index (%d)", msg.c_str(), err); break;
    case ADL_ERR_NOT_SUPPORTED:           Radiant::error("ScreenDetectorAMD::detect # %s: Function not supported (%d)", msg.c_str(), err); break;
    case ADL_ERR_NULL_POINTER:            Radiant::error("ScreenDetectorAMD::detect # %s: Null pointer error (%d)", msg.c_str(), err); break;
    case ADL_ERR_DISABLED_ADAPTER:        Radiant::error("ScreenDetectorAMD::detect # %s: Disabled adapter (%d)", msg.c_str(), err); break;
    case ADL_ERR_INVALID_CALLBACK:        Radiant::error("ScreenDetectorAMD::detect # %s: Invalid callback (%d)", msg.c_str(), err); break;
    case ADL_ERR_RESOURCE_CONFLICT:       Radiant::error("ScreenDetectorAMD::detect # %s: Resource conflict (%d)", msg.c_str(), err); break;
    case ADL_OK:                          return true; // All ok
    default:
      Radiant::error("ScreenDetectorAMD::detect # Error %d", err);
    }
    return false;
  }

  // Retrieve adapter information
  bool getAdapterInformation( std::vector<AdapterInfo> & adapterInfo )
  {
    int adapterCount = 0;
    bool success = checkADL("ADL_Adapter_NumberOfAdapters_Get", ADL_Adapter_NumberOfAdapters_Get(&adapterCount));
    if (success) {
      adapterInfo.resize(adapterCount);
      memset(adapterInfo.data(), 0, adapterCount * sizeof(AdapterInfo));
      success = checkADL("ADL_Adapter_AdapterInfo_Get", ADL_Adapter_AdapterInfo_Get( adapterInfo.data(), adapterCount * sizeof(AdapterInfo)));
    }
    return success;
  }

#if defined (RADIANT_WINDOWS)
  // Retrieve display configuration
  bool getDisplayMapConfig(int adapterIndex, std::vector<ADLDisplayMap> & displayMap, std::vector<ADLDisplayTarget> & displayTarget)
  {
    ADLDisplayMap * maps = NULL;
    ADLDisplayTarget * targets = NULL;
    int numDisplayMaps = 0;
    int numDisplayTargets = 0;

    bool success = checkADL("ADL_Display_DisplayMapConfig_Get", ADL_Display_DisplayMapConfig_Get(adapterIndex, &numDisplayMaps, &maps, &numDisplayTargets, &targets, ADL_DISPLAY_DISPLAYMAP_OPTION_GPUINFO));
    if (success) {
      // Copy into target containers
      displayMap.resize(numDisplayMaps); memcpy(displayMap.data(), maps, numDisplayMaps * sizeof(ADLDisplayMap));
      displayTarget.resize(numDisplayTargets); memcpy(displayTarget.data(), targets, numDisplayTargets * sizeof(ADLDisplayTarget));

      // Clean-up
      adlFree(maps);
      adlFree(targets);
    }
    return success;
  }

  // Retrieve SLS (EyeFinity) configuration
  bool getSLSMapConfig(int adapterIndex, std::vector<ADLDisplayTarget> displayTargets, ADLSLSMap & slsMap, std::vector<ADLSLSTarget> & targets, std::vector<ADLSLSMode> & modes, std::vector<ADLBezelTransientMode> & bezels, std::vector<ADLBezelTransientMode> & transients, std::vector<ADLSLSOffset> & offsets)
  {
    int slsIndex = 0;
    int numDisplayTarget = (int)displayTargets.size();
    ADLDisplayTarget * displayTarget = displayTargets.data();

    bool success = false;
    // NOTE: we don't use checkADL here since this is allowed to fail on non-SLS targets
    if (ADL_Display_SLSMapIndex_Get(adapterIndex, numDisplayTarget, displayTarget, &slsIndex) == ADL_OK) {
      int numSLSTargets = 0;
      int numNativeModes = 0;
      int numBezelModes = 0;
      int numTransientModes = 0;
      int numSLSOffsets = 0;

      ADLSLSTarget * slsTargets = NULL;
      ADLSLSMode * nativeModes = NULL;
      ADLBezelTransientMode * bezelModes = NULL;
      ADLBezelTransientMode * transientModes = NULL;
      ADLSLSOffset * slsOffsets = NULL;

      if (checkADL("ADL_Display_SLSMapConfig_Get", ADL_Display_SLSMapConfig_Get(adapterIndex, slsIndex, &slsMap,
        &numSLSTargets, &slsTargets,
        &numNativeModes, &nativeModes,
        &numBezelModes, &bezelModes,
        &numTransientModes, &transientModes,
        &numSLSOffsets, &slsOffsets,
        ADL_DISPLAY_SLSGRID_CAP_OPTION_RELATIVETO_CURRENTANGLE)))
      {
        if ( slsMap.grid.iSLSGridColumn * slsMap.grid.iSLSGridRow == numDisplayTarget ) {
          success = true;
          targets.resize(numSLSTargets); memcpy(targets.data(), slsTargets, numSLSTargets * sizeof(ADLSLSTarget));
          modes.resize(numNativeModes); memcpy(modes.data(), nativeModes, numNativeModes * sizeof(ADLSLSMode));
          bezels.resize(numBezelModes); memcpy(bezels.data(), bezelModes, numBezelModes * sizeof(ADLBezelTransientMode));
          transients.resize(numTransientModes); memcpy(transients.data(), transientModes, numTransientModes * sizeof(ADLBezelTransientMode));
          offsets.resize(numSLSOffsets); memcpy(offsets.data(), slsOffsets, numSLSOffsets * sizeof(ADLSLSOffset));
        }
        else
          Radiant::error("Number of display targets returned is not equal to the SLS grid size");

        // Free resources
        adlFree(slsTargets);
        adlFree(nativeModes);
        adlFree(bezelModes);
        adlFree(transientModes);
        adlFree(slsOffsets);
      }
    }
    return success;
  }

  // Checks validity of a display mode
  bool isValidMode(const ADLMode & mode)
  {
    // Check if we have a valid orientation
    return
      mode.iModeValue != 0 &&
      mode.iOrientation != -1;
  }

  // Retrieve mode for a single displaytarget
  bool getDisplayTargetMode(int adapterIndex, int displayTargetIndex, std::vector<ADLMode> & mode)
  {
    int numModes = 0;
    ADLMode *modes = NULL;

    // Retrieve mode for this target
    bool success = checkADL("ADL_Display_Modes_Get", ADL_Display_Modes_Get( adapterIndex, displayTargetIndex, &numModes, &modes));
    if (success) {
      mode.clear();
      // Filter out invalid modes
      for (int i = 0; i < numModes; ++i) {
        if (isValidMode(modes[i]))
          mode.push_back(modes[i]);
      }
      adlFree(modes);
    }

    return success;
  }
#endif

  // Get display information for all displays on the specified adapter
  bool getDisplayInfo(int adapterIndex, std::vector<ADLDisplayInfo> & displayInfo)
  {
    int displayCount = 0;
    ADLDisplayInfo * lst = 0;
    bool success = checkADL("ADL_Display_DisplayInfo_Get", ADL_Display_DisplayInfo_Get(adapterIndex, &displayCount, &lst, 1));
    if (success) {
      displayInfo.resize(displayCount); memcpy(displayInfo.data(), lst, displayCount * sizeof(ADLDisplayInfo));
      adlFree(lst);
    }
    return success;
  }

  #if defined (RADIANT_LINUX)
  bool detectLinux(int screen, QList<Luminous::ScreenInfo> & results)
  {
    std::vector<AdapterInfo> adapterInfo;
    getAdapterInformation(adapterInfo);
    if (adapterInfo.empty())
      return false;

    // Fetch the XScreen information
    std::vector<XScreenInfo> xscreens(adapterInfo.size());
    memset(xscreens.data(), 0, sizeof(XScreenInfo) * xscreens.size());
    if (!checkADL("ADL_Adapter_XScreenInfo_Get", ADL_Adapter_XScreenInfo_Get(xscreens.data(), sizeof(XScreenInfo) * xscreens.size())))
      return false;

    std::set<ADLDisplayID> uniqueDisplays;
    bool ok = false;
    for(size_t adapterIdx = 0; adapterIdx < adapterInfo.size(); ++adapterIdx) {

      // XScreen doesn't match the requested screen: skip
      if(xscreens[adapterInfo[adapterIdx].iAdapterIndex].iXScreenNum != screen) continue;

      const AdapterInfo & currentAdapter = adapterInfo[adapterIdx];

      std::vector<ADLDisplayInfo> displayInfos;
      if (!getDisplayInfo(currentAdapter.iAdapterIndex, displayInfos))
        continue;

      for(size_t displayIdx = 0; displayIdx < displayInfos.size(); ++displayIdx) {
        const ADLDisplayInfo & currentDisplay = displayInfos[displayIdx];

        if (uniqueDisplays.find(currentDisplay.displayID) != uniqueDisplays.end()                   // Already seen this display
            ||(currentDisplay.iDisplayInfoValue & ADL_DISPLAY_DISPLAYINFO_DISPLAYCONNECTED ) == 0)  // Don't care about disconnected outputs
          continue;

        uniqueDisplays.insert(currentDisplay.displayID);

        Luminous::ScreenInfo screenInfo;
        screenInfo.setName(currentDisplay.strDisplayName);
        screenInfo.setGpuName(currentAdapter.strAdapterName);
        screenInfo.setNumId(displayIdx);

        int gpuID = 0;
        ADL_Adapter_ID_Get(adapterInfo[adapterIdx].iAdapterIndex, &gpuID);
        screenInfo.setGpu(QString("GPU-0x") + QString::number(gpuID,16));

        QString type;
        switch (currentDisplay.iDisplayType)
        {
        case ADL_DT_MONITOR: type = "Monitor-"; break;
        case ADL_DT_TELEVISION: type = "TV-"; break;
        case ADL_DT_LCD_PANEL: type = "LCD-"; break;
        case ADL_DT_DIGITAL_FLAT_PANEL: type = "DFP-"; break;
        case ADL_DT_COMPONENT_VIDEO: type = "Component-"; break;
        case ADL_DT_PROJECTOR: type = "Projector-"; break;
        default:
          type = "Unknown-";
        }

        screenInfo.setConnection(type + QString::number(currentDisplay.displayID.iDisplayLogicalIndex));
        screenInfo.setLogicalScreen(screen);

        // Get geometry from XRandr
        char name[256];
        bool err = ADL_Display_XrandrDisplayName_Get(currentAdapter.iAdapterIndex,
          currentDisplay.displayID.iDisplayLogicalIndex,
          name, sizeof(name));
        if(err != ADL_OK) {
          Radiant::error("ScreenDetectorAMD::detect # ADL_Display_XrandrDisplayName_Get: %d", err);
          continue;
        }
        Nimble::Recti rect;
        Luminous::XRandR xrandr;
        if(xrandr.getGeometry(screen, name, rect)) {
          screenInfo.setGeometry(rect);
        } else {
          continue;
        }

        ok = true;
        results.push_back(screenInfo);
      }
    }
    return ok;
  }

  #elif defined (RADIANT_WINDOWS)
  bool detectWindows(QList< Luminous::ScreenInfo> & results)
  {
    std::vector<AdapterInfo> adapterInfo;
    getAdapterInformation(adapterInfo);
    if (adapterInfo.empty())
      return false;

    Luminous::ScreenInfo screeninfo;
    screeninfo.setLogicalScreen(0); // Windows doesn't have a logical screen

    for (int adapterIdx = 0; adapterIdx < adapterInfo.size(); ++adapterIdx) {
      int active;
      checkADL("ADL_Adapter_Active_Get", ADL_Adapter_Active_Get(adapterInfo[adapterIdx].iAdapterIndex, & active));
      // Get rid of invalid adapters
      if (active == ADL_FALSE
        || adapterInfo[adapterIdx].iPresent == 0
        || adapterInfo[adapterIdx].iExist == 0
        || adapterInfo[adapterIdx].iAdapterIndex == -1)
      {
        continue;
      }

      // Set GPU information
      int gpuID = 0;
      ADL_Adapter_ID_Get(adapterInfo[adapterIdx].iAdapterIndex, &gpuID);
      screeninfo.setGpu(QString("GPU-0x") + QString::number(gpuID,16));
      screeninfo.setGpuName(adapterInfo[adapterIdx].strAdapterName);

      // Retrieve display device name
      DISPLAY_DEVICEA dd;
      memset(&dd, 0, sizeof(dd));
      dd.cb = sizeof(dd);
      EnumDisplayDevicesA(adapterInfo[adapterIdx].strDisplayName, 0, &dd, 0);
      screeninfo.setName(dd.DeviceString);

      //
      std::vector<ADLDisplayMap> displayMaps;
      std::vector<ADLDisplayTarget> displayTargets;

      if (!getDisplayMapConfig( adapterInfo[adapterIdx].iAdapterIndex, displayMaps, displayTargets))
        continue;

      ADLSLSMap slsMap = { 0 };

      std::vector<ADLSLSTarget> slsTargets;
      std::vector<ADLSLSMode> slsModes;
      std::vector<ADLBezelTransientMode> bezelModes;
      std::vector<ADLBezelTransientMode> transientModes;
      std::vector<ADLSLSOffset> slsOffsets;

      if (!getSLSMapConfig(adapterInfo[adapterIdx].iAdapterIndex, displayTargets, slsMap, slsTargets, slsModes, bezelModes, transientModes, slsOffsets))
        continue;

      // Get the display mode for each target
      for ( int displayTargetIdx = 0; displayTargetIdx < displayTargets.size(); ++displayTargetIdx )
      {
        std::vector<ADLMode> targetMode;
        if (!getDisplayTargetMode(adapterInfo[adapterIdx].iAdapterIndex, displayTargets[displayTargetIdx].displayID.iDisplayLogicalIndex, targetMode))
          continue;

        // Search the 'normal' modes
        std::vector<ADLSLSMode>::const_iterator slsIter = std::find(slsModes.begin(), slsModes.end(), targetMode[0]);
        std::vector<ADLBezelTransientMode>::const_iterator bezelIter;
        if (slsIter != slsModes.end()) {
          /// Found a 'normal' mode (without bezel correction)
          //////////////////////////////////////////////////////////////////////////
          // Find the SLSTarget that maps to the current display target
          std::vector<ADLSLSTarget>::const_iterator slsTargetIter = std::find(slsTargets.begin(), slsTargets.end(), displayTargets[displayTargetIdx]);

          // Determine if we're in portrait or landscape
          bool portrait = (targetMode[0].iOrientation != 0 && targetMode[0].iOrientation != 180);
          // Set geometry
          int width = (portrait ? targetMode[0].iYRes : targetMode[0].iXRes) / slsMap.grid.iSLSGridColumn;
          int height = (portrait ? targetMode[0].iXRes : targetMode[0].iYRes) / slsMap.grid.iSLSGridRow;
          int posX = width * slsTargetIter->iSLSGridPositionX;
          int posY = height * slsTargetIter->iSLSGridPositionY;

          screeninfo.setGeometry(Nimble::Recti(posX, posY, posX + width, posY + height));
          screeninfo.setConnection(QString("DFP-%1").arg(slsTargetIter->displayTarget.displayID.iDisplayLogicalIndex));
          results.push_back(screeninfo);
        } else
        {
          // Search bezel-corrected modes
          bezelIter = std::find(bezelModes.begin(), bezelModes.end(), targetMode[0]);

          if (bezelIter != bezelModes.end()) {
            /// Found a bezel corrected mode
            //////////////////////////////////////////////////////////////////////////
            // Find the current SLS Offset
            int currentSLSOffset = -1;
            for ( int slsOffsetIdx = 0; slsOffsetIdx< slsOffsets.size(); slsOffsetIdx++) {
              if ( bezelIter->iSLSModeIndex == slsOffsets[slsOffsetIdx].iBezelModeIndex &&
                displayTargets[displayTargetIdx].displayID.iDisplayLogicalIndex == slsOffsets[slsOffsetIdx].displayID.iDisplayLogicalIndex )
              {
                currentSLSOffset = slsOffsetIdx;
                break;
              }
            }

            // Set geometry
            int left = slsOffsets[currentSLSOffset].iBezelOffsetX;
            int top = slsOffsets[currentSLSOffset].iBezelOffsetY;
            int right = left + slsOffsets[currentSLSOffset].iDisplayWidth;
            int bottom = top + slsOffsets[currentSLSOffset].iDisplayHeight;

            screeninfo.setGeometry(Nimble::Recti(left, top, right, bottom));
            screeninfo.setConnection(QString("DFP-%1").arg(slsOffsets[currentSLSOffset].displayID.iDisplayLogicalIndex));
            results.push_back(screeninfo);
          }
          else
          {
            // Single display
            //////////////////////////////////////////////////////////////////////////
            // Determine if we're in portrait or landscape
            bool portrait = (targetMode[0].iOrientation != 0 && targetMode[0].iOrientation != 180);
            // Set geometry
            int left = targetMode[0].iXPos;
            int top = targetMode[0].iYPos;
            int right = left + (portrait ? targetMode[0].iYRes : targetMode[0].iXRes);
            int bottom = top + (portrait ? targetMode[0].iXRes : targetMode[0].iYRes);

            screeninfo.setGeometry(Nimble::Recti(left, top, right, bottom));
            screeninfo.setConnection(QString("DFP-%1").arg(targetMode[0].displayID.iDisplayLogicalIndex));
            results.push_back(screeninfo);
          }
        }
      }
    }
    return true;
  }
  #endif // PLATFORM
}

namespace Luminous
{
  static bool adlAvailable = false;

  bool ScreenDetectorAMD::detect(int screen, QList<ScreenInfo> & results)
  {
    MULTI_ONCE { adlAvailable = initADL(); }
    if (!adlAvailable)
      return false;

    checkADL("ADL_Main_Control_Create", ADL_Main_Control_Create(adlAlloc, 1));

#if defined (RADIANT_LINUX)
    bool success = detectLinux(screen, results);
#elif defined (RADIANT_WINDOWS)
    (void)screen;
    bool success = detectWindows(results);
#else
#  error "ScreenDetectorAMD Not implemented on this platform"
#endif

    ADL_Main_Control_Destroy();
    return success;
  }
}
