/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "CamView.hpp"
#include "MainWindow.hpp"

#include <Nimble/Vector4.hpp>

#include <Radiant/ConfigReader.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/VideoCamera.hpp>
#include <Radiant/CameraDriver.hpp>

#include <QtGui/QApplication>

#ifndef WIN32
#include <Radiant/VideoCamera1394.hpp>
#endif

enum Task {
  TASK_SHOW_CAMERAS,
  TASK_SCAN_BUS
};

void helper(const char * app)
{
  printf("USAGE:\n %s [options]\n\n", app);
  printf
    ("OPTIONS:\n"
     " --binning [ansi|cree|taction7|taction7ab] - select color binning mode for color calibration\n"
     " --debayer - Enable de-Bayer filter\n"
     " --colorbal - Show color balance of color camera\n"     " --format7 +int - Uses Format 7 mode in the argument\n"
     " --format7area +rect - Select Format 7 capture area, for example \"0 0 200 100\"\n"
     " --fps  +float  - Sets arbitrary capture rate for the cameras, with SW trigger\n"
     " --help         - This help\n"
     " --listformat7modes    - List available format 7 modes\n"
     " --rate +int    - Selects one of the standard frame rates (15, 30, 60...)\n"
     " --scanbus      - Scans and reports all available cameras\n"
     " --triggermode   +int - Selects the trigger mode, range: 0-%d\n"
     " --triggerpolarity   +up/down - Selects the trigger polarity, either "
          "\"up\" or \"down\"\n"
     " --triggersource +int - Selects the trigger source, range: 0-%d\n"
     " --wb +coeffs - Color balance coefficients, for example \"1.0 1.1 1.2\"\n"
#ifndef WIN32
     " --busreset - Resets the firewire bus\n"
#endif
     "\nEXAMPLES:\n"
     " %s             - Run all cameras at 15 fps\n"
     " %s --scanbus   - List cameras, with IDs\n"
     " %s --fps 47    - Run all cameras at 47 fps (with SW triggering, It may run out of bandwith)\n"
     " %s --rate 30   - Run all cameras at 30 fps (internal triggering, It may run out of bandwith)\n"
     " %s --rate 60 --triggersource 0  - Run all cameras at max 60 fps with hardware trigger(It may run out of bandwith)\n"
     " %s --rate 60 --triggersource 0 --triggermode 0 - Run all cameras at max 60 fps with trigger source 0 and trigger mode 0(It may run out of bandwith)\n"
     " %s --fps 120 --format7 1 --triggersource 0 --triggermode 0  --format7area \"60 0 356 206\" - Test high-speed triggered format 7 operation\n",
     7, Radiant::VideoCamera::TRIGGER_SOURCE_MAX - 1,
      app, app, app, app, app, app, app);

  fflush(0);
}


int main(int argc, char ** argv)
{
  float fps = -1.0f;

  Radiant::VideoCamera::TriggerSource triggerSource = Radiant::VideoCamera::TRIGGER_SOURCE_MAX;
  Radiant::VideoCamera::TriggerMode triggerMode = Radiant::VideoCamera::TRIGGER_MODE_MAX;
  Radiant::FrameRate rate = Radiant::FPS_15;
  Task t = TASK_SHOW_CAMERAS;
  int i, res = 0;
  bool format7 = false;
  bool listmodes = false;

  for(i = 1; i < argc; i++) {
    const char * arg = argv[i];


    if(strcmp(arg, "--debayer") == 0) {
      FireView::CamView::setDebayer(1);
    }
    else if(strcmp(arg, "--binning") == 0 && (i+1) < argc) {
      const char * tmp(argv[++i]);

      if(strcmp(tmp, "ansi") == 0)
        FireView::CamView::setBinningMethod(FireView::Binning::BINNING_ANSI_C78_377);
      else if(strcmp(tmp, "cree") == 0)
        FireView::CamView::setBinningMethod(FireView::Binning::BINNING_CREE);
      else if(strcmp(tmp, "taction7") == 0)
        FireView::CamView::setBinningMethod(FireView::Binning::BINNING_TACTION7);
      else if(strcmp(tmp, "taction7ab") == 0)
        FireView::CamView::setBinningMethod(FireView::Binning::BINNING_TACTION7AB);
      else {
        Radiant::error("%s : Unknown binning mode \"%s\"", argv[0], tmp);
        helper(argv[0]);
        return -1;
      }
    }
    else if(strcmp(arg, "--colorbal") == 0) {
      FireView::CamView::calculateColorBalance();
      FireView::CamView::setDefaultParameter(Radiant::VideoCamera::SHUTTER, 1);
      FireView::CamView::setDefaultParameter(Radiant::VideoCamera::BRIGHTNESS, 10);
      FireView::CamView::setDefaultParameter(Radiant::VideoCamera::EXPOSURE, 0);
      FireView::CamView::setDefaultParameter(Radiant::VideoCamera::GAMMA, 0);
      FireView::CamView::setDefaultParameter(Radiant::VideoCamera::FRAME_RATE, 466);
      // FireView::CamView::setDefaultParameter(Radiant::VideoCamera::SHUTTER, 30);
      FireView::CamView::setDefaultParameter(Radiant::VideoCamera::GAIN, 16);
      FireView::CamView::setDefaultParameter(Radiant::VideoCamera::PAN, 0);
    }
    else if(strcmp(arg, "--format7") == 0 && (i+1) < argc) {
      format7 = true;

      FireView::CamView::setFormat7mode(atoi(argv[++i]));
    }
#ifndef WIN32
    else if(strcmp(arg, "--busreset") == 0) {
      Radiant::VideoCamera1394::busReset();
      return 0;
    }
#endif
    else if(strcmp(arg, "--format7area") == 0 && (i+1) < argc) {
      format7 = true;
      Radiant::Variant tmp(argv[++i]);

      Nimble::Vector4f vals(0, 0, 1920, 1080);

      tmp.getFloats(vals.data(), 4);

      FireView::CamView::setFormat7area(vals[0], vals[1], vals[2], vals[3]);
    }
    else if(strcmp(arg, "--fps") == 0 && (i+1) < argc) {
      fps = atof(argv[++i]);
    }
    else if(strcmp(arg, "--help") == 0) {
      helper(argv[0]);
      return 0;
    }
    else if(strcmp(arg, "--listformat7modes") == 0) {
      listmodes = true;
    }
    else if(strcmp(arg, "--rate") == 0 && (i+1) < argc) {
      rate = Radiant::closestFrameRate(atof(argv[++i]));
    }
    else if(strcmp(arg, "--scanbus") == 0) {
      t = TASK_SCAN_BUS;
    }
    else if(strcmp(arg, "--triggermode") == 0 && (i+1) < argc) {
      triggerMode = Radiant::VideoCamera::TriggerMode(atoi(argv[++i]));
    }
    else if(strcmp(arg, "--triggerpolarity") == 0 && (i+1) < argc) {
      i++;
      if(strcmp(argv[i], "up") == 0)
        FireView::CamView::setTriggerPolarity(Radiant::VideoCamera::TRIGGER_ACTIVE_HIGH);
      else if(strcmp(argv[i], "down") == 0)
        FireView::CamView::setTriggerPolarity(Radiant::VideoCamera::TRIGGER_ACTIVE_LOW);
    }
    else if(strcmp(arg, "--triggersource") == 0 && (i+1) < argc) {
      triggerSource = Radiant::VideoCamera::TriggerSource(atoi(argv[++i]));
    }
    else if(strcmp(arg, "--verbose") == 0) {
      puts("Verbose mode");
      FireView::CamView::setVerbose(true);
      Radiant::enableVerboseOutput(true);
    }
    else if (strcmp(arg, "--celltest") == 0) {
      triggerMode = Radiant::VideoCamera::TRIGGER_MODE_0;
      triggerSource = Radiant::VideoCamera::TRIGGER_SOURCE_0;
      format7 = true;
      // full format7 area
      Nimble::Vector4f vals(0, 0, 376, 240);
      fps = 60;

      FireView::CamView::setFormat7area(vals[0], vals[1], vals[2], vals[3]);
      FireView::CamView::setDefaultParameter(Radiant::VideoCamera::SHUTTER, 30);
      FireView::CamView::setDefaultParameter(Radiant::VideoCamera::BRIGHTNESS, 200);
      FireView::CamView::setDefaultParameter(Radiant::VideoCamera::SHUTTER, 30);
      FireView::CamView::setDefaultParameter(Radiant::VideoCamera::GAIN, 20);
    }
    else if(strcmp(arg, "--wb") == 0 && (i+1) < argc) {
      Radiant::Variant tmp(argv[++i]);

      Nimble::Vector3f vals(1.f, 1.f, 1.f);
      tmp.getFloats(vals.data(), 3);

      FireView::CamView::setColorBalanceCoeffs(vals);
    }
    else {
      printf("%s Could not handle argument %s\n", argv[0], arg);
      helper(argv[0]);
      return -1;
    }
  }

  if(triggerMode >= 0 && triggerSource < 0) {
    printf("%s If you set trigger mode, you also need to set trigger source\n",
       argv[0]);
    return -1;
  }

  if(t == TASK_SCAN_BUS) {
      std::vector<Radiant::VideoCamera::CameraInfo> cameras;
      Radiant::CameraDriver * cd = Radiant::VideoCamera::drivers().getPreferredCameraDriver();
      if(cd) cd->queryCameras(cameras);

    printf("Found %d FireWire cameras\n", (int) cameras.size());

    for(i = 0; i < (int) cameras.size(); i++) {
      const Radiant::VideoCamera::CameraInfo & cam = cameras[i];
      printf("Camera %d: ID = %llx VENDOR = %s, MODEL = %s, DRIVER = %s\n",
         i + 1, (long long) cam.m_euid64,
       cam.m_vendor.toUtf8().data(), cam.m_model.toUtf8().data(), cam.m_driver.toUtf8().data());
      fflush(0);

      if(listmodes)
          Radiant::error("listmodes not implemented");
    }
  } else {
    QApplication qa(argc, argv);

    FireView::MainWindow * mw =
      new FireView::MainWindow(rate, fps, triggerSource, triggerMode, format7);

    mw->resize(800, 600);
    mw->init();
    mw->show();

    QObject::connect( & qa, SIGNAL(lastWindowClosed()), & qa, SLOT(quit()));
    res = qa.exec();

    delete mw;
  }

  return res;
}
