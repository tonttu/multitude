#include "ScreenDetectorNV.hpp"

#include <QStringList>
#include <QX11Info>
#include <QRect>
#include <QDesktopWidget>

#include <X11/Xlib.h>
#include <NVCtrl/NVCtrlLib.h>

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
}

namespace Luminous
{
  ScreenDetectorNV::ScreenDetectorNV()
  {
  }

  bool ScreenDetectorNV::detect(int screen, QList<ScreenInfo> & results)
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

    int mask = 0;
    if(XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_X_SCREEN, screen, 0,
                                   //NV_CTRL_ASSOCIATED_DISPLAY_DEVICES
                                   NV_CTRL_ENABLED_DISPLAYS, &mask) == False)
      return false;

    int gpuCount = 0;
    if(XNVCTRLQueryTargetCount(display, NV_CTRL_TARGET_TYPE_GPU, &gpuCount) == False)
      return false;

    char * name = 0;

    // screen number => gpu mask
    std::map<int, int> screen_gpus;

    // gpu id => gpu name
    std::map<int, QString> gpu_names;

    for(int gpu = 0; gpu < gpuCount; ++gpu) {
      if(XNVCTRLQueryTargetStringAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu, 0,
                                     NV_CTRL_STRING_PRODUCT_NAME, &name)) {
        gpu_names[gpu] = name;
        XFree(name);
        name = 0;
      }

      int mask_gpu = 0;
      if(XNVCTRLQueryTargetAttribute(display, NV_CTRL_TARGET_TYPE_GPU, gpu, 0,
                                     //NV_CTRL_ASSOCIATED_DISPLAY_DEVICES
                                     NV_CTRL_ENABLED_DISPLAYS, &mask_gpu) == False)
        continue;

      mask_gpu &= mask;
      if(!mask_gpu) continue;
      for(int i = 0; i < 24; ++i) {
        int d = 1 << i;
        if((mask_gpu & d) == 0) continue;
        screen_gpus[i] |= (1 << gpu);
      }
    }

    std::vector<int> xinerama_order;
    if(XNVCTRLQueryStringAttribute(display, screen, 0,
                                   NV_CTRL_STRING_TWINVIEW_XINERAMA_INFO_ORDER, &name)) {
      std::map<QString, int> all;
      for(int port = 0; port < 24; ++port) {
        int d = 1 << port;
        if((mask & d) == 0) continue;
        all[connectionName(port)] = port;
      }
      foreach(const QString& c, QString(name).split(", ")) {
        if(all.count(c) == 0) continue;
        xinerama_order.push_back(all[c]);
      }

      XFree(name);
      name = 0;
    }

    QDesktopWidget desktopWidget;

    for(int port = 0; port < 24; ++port) {
      int d = 1 << port;
      if((mask & d) == 0) continue;

      results.push_back(ScreenInfo());
      ScreenInfo& info = results.back();

      if(XNVCTRLQueryStringAttribute(display, screen, d,
                                     NV_CTRL_STRING_DISPLAY_DEVICE_NAME, &name)) {
        info.setName(name);
        XFree(name);
        name = 0;
      }

      QStringList lst, lst2;
      for(int gpu = 0; gpu < 32; ++gpu) {
        int gpu_mask = 1 << gpu;
        if(screen_gpus[port] & gpu_mask) {
          lst << QString("GPU-%1").arg(gpu);
          lst2 << gpu_names[gpu];
        }
      }
      info.setGpu(lst.join(","));
      info.setGpuName(lst2.join(","));
      info.setConnection(connectionName(port));
      info.setLogicalScreen(screen);

      for(int di = 0, xinerama_idx = 0; di < desktopWidget.screenCount(); ++di) {
        if(desktopWidget.screen(di)->x11Info().screen() != screen) continue;
        if(xinerama_order.size() <= xinerama_idx) break;
        if(xinerama_order[xinerama_idx++] != port)
          continue;

        QRect r = desktopWidget.screenGeometry(di);
        info.setGeometry(Nimble::Recti(r.left(), r.top(), r.left() + r.width(), r.top() + r.height()));
        break;
      }
    }

    return true;
  }
}
