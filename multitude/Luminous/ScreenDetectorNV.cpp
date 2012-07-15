#include "ScreenDetectorNV.hpp"

#include <Radiant/Platform.hpp>
#include <Radiant/Trace.hpp>

#include <QStringList>
#include <QX11Info>
#include <QRect>
#include <QDesktopWidget>

#if defined (RADIANT_LINUX)
#  include <X11/Xlib.h>
#  include <NVCtrl/NVCtrlLib.h>
#elif defined (RADIANT_WINDOWS)
#  include <NVAPI/nvapi.h>
#  include <windows.h>
#endif

#include <map>
#include <vector>

namespace
{
  QString connectionName(int id)
  {
    if(id >= 0 && id <= 7) return QString("CRT-%1").arg(id);
    if(id >= 8 && id <= 15) return QString("TV-%1").arg(id-8);
    if(id >= 16 && id <= 23) return QString("DFP-%1").arg(id-16);
    return "Unknown";
  }

#if defined (RADIANT_LINUX)

  bool detectLinuxInternal(int screen, int logical_screen, Display * display, QList<Luminous::ScreenInfo> & results)
  {
    int* bdata;
    int len;
    QStringList gpu_ids;
    QStringList gpu_names;

    char * name = 0;
    QString display_port_device_name;
    bool xinerama_on = false;
    bool twinview_on;
    int query;
    int display_mask = 0;

    if (XNVCTRLQueryAttribute(display, screen, 0,
                                    NV_CTRL_XINERAMA, &query))
    {
      xinerama_on = query == NV_CTRL_XINERAMA_ON;
      Radiant::debug("screen %d xinerama is %d\n",screen, xinerama_on);
    }
    else
    {
      Radiant::error("couldn't query xinerama\n");
      return false;
    }

    if (XNVCTRLQueryAttribute(display, screen, 0, NV_CTRL_TWINVIEW, &query))
    {
      twinview_on = query == NV_CTRL_TWINVIEW_ENABLED;
      Radiant::debug("screen %d twinview is %d\n", screen, twinview_on);
    }
    else
    {
      Radiant::debug("couldnt query twinview\n");
    }

    /*
      which display ports are enabled for this XScreen, usually just 1 but could be
      2 in the case of TwinView.
      You can think of a display port as one of the connectors to the GPU
    */

    if(XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_X_SCREEN, screen, 0,
      NV_CTRL_ENABLED_DISPLAYS, &display_mask) == False)
      return false;
    Radiant::debug("display/port mask for screen %d = 0X%x\n", screen, display_mask);

    if(XNVCTRLQueryBinaryData(display, screen,0, NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN, (unsigned char **)&bdata, &len))
    {
      int num = bdata[0];
      Radiant::debug("NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN for screen %d :  #gpus=%d:\n", screen, num);

      for(int i=1; i<=num; i++)
      {
        Radiant::debug("\tgpu=%d\n", bdata[i]);
        gpu_ids<<QString("GPU-%1").arg(bdata[i]);
        if(XNVCTRLQueryTargetStringAttribute(display, NV_CTRL_TARGET_TYPE_GPU, bdata[i], 0,
          NV_CTRL_STRING_PRODUCT_NAME, &name)) {
            gpu_names<<QString(name);
            XFree(name);
            name = 0;
        }
        else
        {
          gpu_names<<"";
        }
      }
      XFree(bdata);
    }
    else
    {
        Radiant::debug("XNVCTRLQueryBinaryData(display, screen,0, NV_CTRL_BINARY_DATA_GPUS_USED_BY_XSCREEN, &bdata, &len)) failed\n");
    }

    //for all enabled display ports in screen
    for(int port = 0; port < 24; ++port)
    {
      int d = 1 << port;
      if((display_mask & d) == 0) continue;

      int posx = 0;
      int posy = 0;
      int dwidth = 0;
      int dheight = 0;

       //one screenInfo per enabled display port
      results.push_back(Luminous::ScreenInfo());
      Luminous::ScreenInfo& info = results.back();


      /*
        nvidia-setting allows the creation of a Xinerama setup that has inside
        a number of Twinview XScreens along with a number of separate regular
        XScreens all packed together into one Xinerama logical XScreen.
        In this case both xinerama and twinview flags will be 1 for the Twinview
        screens.
        NV_CTRL_STRING_XINERAMA_SCREEN_INFO property only works for normal
        Separate XScreens hidden by Xinerama not for TwinView screens hidden by
        Xinerama.
       */
      if(xinerama_on && !twinview_on)
      {
        char* ptr;
        if(XNVCTRLQueryStringAttribute(display, screen, d,
                                    NV_CTRL_STRING_XINERAMA_SCREEN_INFO, &ptr))
        {
          QString sinfo(ptr);
          QRegExp x_re("x=(\\d+)");
          QRegExp y_re("y=(\\d+)");
          QRegExp w_re("width=(\\d+)");
          QRegExp h_re("height=(\\d+)");

          if(x_re.indexIn(sinfo)!=-1)
          {
            posx = x_re.cap(1).toInt();
          }

          if(y_re.indexIn(sinfo)!=-1)
          {
            posy = y_re.cap(1).toInt();
          }

          if(w_re.indexIn(sinfo)!=-1)
          {
            dwidth = w_re.cap(1).toInt();
          }


          if(h_re.indexIn(sinfo)!=-1)
          {
            dheight = h_re.cap(1).toInt();
          }

          Radiant::debug("xinerama screen info, for screen %d display=0X%x: x=%d y=%d w=%d h=%d\n", screen, d, posx, posy, dwidth, dheight);
        }
        else
          Radiant::debug("couldn't query xinerama screen info");
      }
      /*
        This applies to TwinView Screens (under xinerama or not) and regular separate XScreens (not Xinerama)
        */
      else
      {
        if(XNVCTRLQueryBinaryData(display, screen,d, NV_CTRL_BINARY_DATA_DISPLAY_VIEWPORT, (unsigned char **)&bdata, &len))
        {
          posx = bdata[0];
          posy = bdata[1];
          dwidth = bdata[2];
          dheight = bdata[3];

          Radiant::debug("NV_CTRL_BINARY_DATA_DISPLAY_VIEWPORT for screen %d and port 0X%x:  x=%d y=%d w=%d h=%d\n", screen, d, posx, posy, dwidth, dheight);
          XFree(bdata);
        }
        else
        {
            Radiant::debug("XNVCTRLQueryBinaryData(... NV_CTRL_BINARY_DATA_DISPLAY_VIEWPORT...) failed\n");
            return false;
        }
      }

      //name of device (e.g. monitor, cell) connected to this display port (e.g. ViewSonic VX2260WM)
      if(XNVCTRLQueryStringAttribute(display, screen, d,
        NV_CTRL_STRING_DISPLAY_DEVICE_NAME, &name))
      {

        Radiant::debug("monitor name attached to screen %d display/port 0X%x = %s\n",screen, d, name);
        display_port_device_name = QString(name);
          XFree(name);
          name = 0;
      }

      info.setGeometry(Nimble::Recti(posx, posy, posx + dwidth, posy + dheight));
      info.setGpu(gpu_ids.join(","));
      info.setGpuName(gpu_names.join(","));
      info.setConnection(connectionName(port));
      if(logical_screen != -1)
        info.setLogicalScreen(logical_screen);
      else
        info.setLogicalScreen(screen);
      info.setName(display_port_device_name);
    }
    return true;
  }


  bool detectLinux(int screen, QList<Luminous::ScreenInfo> & results)
  {
    Display * display = QX11Info::display();

    int event_base, error_base;
    if(XNVCTRLQueryExtension(display, &event_base, &error_base) == False)
      return false;

    int major, minor;
    if(XNVCTRLQueryVersion(display, &major, &minor) == False)
      return false;

    if(XNVCTRLIsNvScreen(display, screen) == False)
      return false;

    bool xinerama_on;
    int query;
    if (XNVCTRLQueryAttribute(display, screen, 0,
                                    NV_CTRL_XINERAMA, &query))
    {
      xinerama_on = query == NV_CTRL_XINERAMA_ON;
      Radiant::debug("screen %d xinerama is %d\n",screen, xinerama_on);
    }
    else
    {
      Radiant::error("couldn't query xinerama\n");
      return false;
    }

    if(xinerama_on)
    {
      /*
        When xinerama is enabled there are actually N xscreens that are hidden
        behind the "logical XScreen" that XLib's XScreenCount() reports and that
        cornerstone will eventually use.
        Querying a Xinerama setup with NVCtrl API  sucks because it requires you
        to refer to each of the hidden XScreens, that otherwise should be
        invisible to the user...
        This is not the case with TwinView where there aren't any hidden
         XScreens, just one that contains two displays.
      */
      int screenCount = 0;
      /*This property seems to enumerate all the screens including the ones
         Xinerama might be hiding
       */
      if(XNVCTRLQueryTargetCount(display, NV_CTRL_TARGET_TYPE_X_SCREEN, &screenCount) == False)
        return false;
      Radiant::debug("number of xscreens according to *TargetCount(...NV_CTRL_TARGET_TYPE_X_SCREEN) %d\n", screenCount);

      /*
        All these screens are hidden behind a logical XScreen 0
      */
      for(int i = 0; i< screenCount; i++)
      {
          //i is the hidden XScreen, screen is our logical XScreen
          if(!detectLinuxInternal(i, screen, display, results))
            return false;
      }
    }
    else
    {
      return detectLinuxInternal(screen, -1, display, results);
    }
    return true;
  }
#elif defined (RADIANT_WINDOWS)
  bool detectWindows(QList<Luminous::ScreenInfo> & results)
  {
    NvAPI_Status err;

    // Load API library. On failure, we're done
    if (NvAPI_Initialize() != NVAPI_OK)
      return false;

    // Enumerate all GPUs
    NvU32 gpuCount = 0;
    NvPhysicalGpuHandle gpu[NVAPI_MAX_PHYSICAL_GPUS];
    NvAPI_EnumPhysicalGPUs(gpu, &gpuCount);

    // Enumerate attached displays
    NvU32 displayIndex = 0;
    NvDisplayHandle displayHandle;
    while (NvAPI_EnumNvidiaDisplayHandle(displayIndex++, &displayHandle) == NVAPI_OK)
    {
      // Find out which GPU(s) belong to this display
      NvPhysicalGpuHandle displayGpu[NVAPI_MAX_PHYSICAL_GPUS];
      NvU32 displayGpuCount = 0;
      /// @todo errorcheck
      err = NvAPI_GetPhysicalGPUsFromDisplay(displayHandle, displayGpu, &displayGpuCount);
      assert(err == NVAPI_OK);

      // Create the GPU specifier string
      QString gpuInfo;
      for (NvU32 i = 0; i < displayGpuCount; ++i) {
        for (NvU32 j = 0; j < gpuCount; ++j) {
          if (displayGpu[i] == gpu[j]) {
            if (!gpuInfo.isEmpty()) gpuInfo += ",";
            gpuInfo += QString("GPU-%1").arg(i);
            break;
          }
        }
      }

      // Note: display is physically attached to the first GPU
      NvPhysicalGpuHandle gpuHandle = gpu[0];
      NvAPI_ShortString gpuName;
      /// @todo errorcheck
      NvAPI_GPU_GetFullName(gpuHandle, gpuName);

      NvAPI_ShortString displayName;
      /// @todo errorcheck
      err = NvAPI_GetAssociatedNvidiaDisplayName(displayHandle, displayName);
      assert(err == NVAPI_OK);

      DEVMODEA devMode;
      memset(&devMode, 0, sizeof(devMode));
      devMode.dmSize = sizeof(devMode);
      /// @todo errorcheck
      EnumDisplaySettingsExA(displayName, ENUM_CURRENT_SETTINGS, &devMode, 0);

      DISPLAY_DEVICEA dd;
      memset(&dd, 0, sizeof(dd));
      dd.cb = sizeof(dd);
      EnumDisplayDevicesA(displayName, 0, &dd, 0);

      // Write the screen information
      Luminous::ScreenInfo info;
      info.setLogicalScreen(0);

      // Screen device name
      info.setName(dd.DeviceString);

      // GPU id and description
      info.setGpu(gpuInfo);
      info.setGpuName(gpuName);

      // Displayport ID
      NvU32 outputId;
      err = NvAPI_GetAssociatedDisplayOutputId (displayHandle, &outputId);
      int outputNumber = 0;
      for (int i = 0; i < 32; ++i)
        if (outputId & (1<<i)) { outputNumber = i; break; }

        info.setConnection(QString("DFP-%1").arg(outputNumber));

        // Geometry
        Nimble::Recti rect;
        rect.setLow( Nimble::Vector2i(devMode.dmPosition.x, devMode.dmPosition.y) );
        rect.setHigh( Nimble::Vector2i(devMode.dmPosition.x + devMode.dmPelsWidth, devMode.dmPosition.y + devMode.dmPelsHeight) );
        info.setGeometry(rect);

        results.push_back(info);
    }

    /// @todo Unloading may fail if resources are locked. Do more error checking
    err = NvAPI_Unload();
    return true;
  }
#endif
}

namespace Luminous
{
  bool ScreenDetectorNV::detect(int screen, QList<ScreenInfo> & results)
  {
#if defined (RADIANT_LINUX)
    return detectLinux(screen, results);
#elif defined (RADIANT_WINDOWS)
    (void)screen;
    return detectWindows(results);
#else
#   error "ScreenDetectorNV not implemented on this platform";
#endif
  }
}
