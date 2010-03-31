/* COPYRIGHT
 *
 * This file is part of Applications/FireView.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Applications/FireView.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
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

enum Task {
  TASK_SHOW_CAMERAS,
  TASK_SCAN_BUS
};

void helper(const char * app)
{
  printf("USAGE:\n %s [options]\n\n", app);
  printf
    ("OPTIONS:\n"
     " --format7 +int - Uses Format 7 mode in the argument\n"
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
     "\nEXAMPLES:\n"
     " %s             - Run all cameras at 15 fps\n"
     " %s --scanbus   - List cameras, with IDs\n"
     " %s --fps 47    - Run all cameras at 47 fps (with SW triggering)\n"
     " %s --rate 30   - Run all cameras at 30 fps (internal triggering)\n"
     " %s --rate 60 --triggersource 0  - Run all cameras at max 60 fps with hardware trigger\n"
     " %s --rate 60 --triggersource 0 --triggermode 0 - Run all cameras at max 60 fps with trigger source 0 and trigger mode 0\n"
     " %s --fps 120 --format7 1 --triggersource 0 --triggermode 0  --format7area \"60 0 356 206\" - Test high-speed triggered format 7 operation\n",
     7, Radiant::VideoCamera::TRIGGER_SOURCE_MAX - 1,
      app, app, app, app, app, app, app);

  fflush(0);
}


int main(int argc, char ** argv)
{
  QApplication qa(argc, argv);
  
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

    if(strcmp(arg, "--format7") == 0 && (i+1) < argc) {
      format7 = true;

      FireView::CamView::setFormat7mode(atoi(argv[++i]));
    }
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
    else {
      printf("%s Could not handle argument %s\n", argv[0], arg);
      helper(argv[0]);
      return -1;
    }
  }

  if(triggerMode >= 0 && triggerSource < 0) {
    printf("%s If you set trigger mode, you also need to set trigger mode\n",
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
      printf("Camera %d: ID = %llx, VENDOR = %s, MODEL = %s, DRIVER = %s\n",
	     i + 1, (long long) cam.m_euid64,
       cam.m_vendor.c_str(), cam.m_model.c_str(), cam.m_driver.c_str());
      fflush(0);

      if(listmodes)
          Radiant::error("listmodes not implemented");
    }
  } else {

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
