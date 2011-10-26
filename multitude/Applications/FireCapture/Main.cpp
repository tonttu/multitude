/* COPYRIGHT
 *
 * This file is part of Applications/FireCapture.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Applications/FireCapture.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include <Radiant/ConfigReader.hpp>
#include <Radiant/Directory.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/Sleep.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/Thread.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/CameraDriver.hpp>
#include <Radiant/ConfigReader.hpp>

#include <Luminous/Image.hpp>

#include <stdlib.h>
#include <string.h>
#include <cassert>

bool format7 = false;
Nimble::Recti format7area(0, 0, 640, 480);
int triggerSource = -1;
int triggerMode = -1;
Radiant::FrameRate rate = Radiant::FPS_15;

class CameraThread : public Radiant::Thread
{
public:

  CameraThread(uint64_t cameraId, const std::string & dir)
    : m_continue(true),
      m_cameraId(cameraId),
      m_format7Area(0, 0, 640, 480),
      m_format7Mode(-1),
      m_dir(dir),
      m_camera(0)
  {
    Radiant::Directory::mkdir(dir);
  }

  void stop()
  {
    m_continue = false;
    waitEnd();
  }

  void setFormat7Mode(int v) { m_format7Mode = v; }

  void setFormat7Area(const Nimble::Recti & r)
  {
    m_format7Area = r;
  }

private:

  virtual void childLoop()
  {
    Radiant::info("Camera loop started for %llx", (long long) m_cameraId);
        
    m_camera = Radiant::VideoCamera::drivers().createPreferredCamera();

    if(m_format7Mode >= 0) {

      if(!m_camera->openFormat7(m_cameraId, m_format7Area, Radiant::asFloat(rate), 1)) {
        Radiant::error("CameraThread::childLoop # failed to open camera (format7);");
        return;
      }

    } else {

      if(!m_camera->open(m_cameraId, 640, 480, Radiant::IMAGE_UNKNOWN, rate)) {
        Radiant::error("CameraThread::childLoop # failed to open camera");
        return;
      }

    }

    if(triggerMode >= 0) {
      m_camera->setTriggerMode(Radiant::VideoCamera::TriggerMode(triggerMode));
    }

    if(triggerSource >= 0) {
      m_camera->enableTrigger(Radiant::VideoCamera::TriggerSource(triggerSource));
    }

    m_camera->setCaptureTimeout(5000);
    m_camera->start();

    char buf[64];
    int count = 0;

    Radiant::TimeStamp start(Radiant::TimeStamp::getTime());

    Luminous::Image saver;

    while(m_continue) {

      const Radiant::VideoImage * im = m_camera->captureImage();

      if(!im) {
        Radiant::error("Frame capture failed for camera %llx",
                       (long long) m_cameraId);
        break;
      }

      count++;

      saver.fromData(im->m_planes[0].m_data, im->m_width, im->m_height,
                     Luminous::PixelFormat::luminanceUByte());

      if(count % 100 == 0) {
        Radiant::info("Capture %d frames from camera %llx",
                      count, (long long) m_cameraId);
      }

      m_camera->doneImage();

      if(count % 10 == 0) {
      
        sprintf(buf, "frame-%.5d.tga", count);
        
        saver.write((m_dir + buf).c_str());
      }
    }
    
    float secs = start.since().secondsD();

    Radiant::info("Captured %d frames in %.2f seconds, %.2f FPS",
                  count, secs, (float) (count / secs));

    m_camera->stop();
  }

  volatile bool m_continue;
  uint64_t      m_cameraId;

  Nimble::Recti m_format7Area;
  int           m_format7Mode;

  std::string   m_dir;

  Radiant::VideoCamera * m_camera;
};

void helper(const char * app)
{
  printf("USAGE:\n %s [options]\n\n", app);
  printf
    ("OPTIONS:\n"

     " --rate +int    - Selects one of the standard frame rates (15, 30, 60...)\n"
     " --triggermode   +int - Selects the trigger mode, range: 0-%d\n"
     " --triggerpolarity   +up/down - Selects the trigger polarity, either "
          "\"up\" or \"down\"\n"
     " --triggersource +int - Selects the trigger source, range: 0-%d\n"
     " --format7area +int +int +int +int - Selects the format7 area\n"
     " --config config.txt      - Selects config file"
     "\nEXAMPLES:\n"
     " %s --rate 60 --triggersource 0  - Run all cameras at max 60 fps with hardware trigger\n"
     , 7, 3,
     app);
  fflush(0);
}


int main(int argc, char ** argv)
{
  int secs = 20;
  
  float fps = -1.0f;

  int i, res = 0;

  std::string baseDir("capture/");

  std::string configFile;
  bool defaultconfig = false;
  bool useConfig = false;

  for(i = 1; i < argc; i++) {
    const char * arg = argv[i];

    if(strcmp(arg, "--format7") == 0) {
      format7 = true;
    }
    else if(strcmp(arg, "--format7area") == 0 && (i+1) < argc) {
      format7 = true;
      Radiant::Variant tmp(argv[++i]);

      Nimble::Vector4f vals(0, 0, 1920, 1080);

      tmp.getFloats(vals.data(), 4);

      format7area.set(vals[0], vals[1], vals[2], vals[3]);
    }
    else if(strcmp(arg, "--fps") == 0 && (i+1) < argc) {
      fps = atof(argv[++i]);
    }
    else if(strcmp(arg, "--help") == 0) {
      helper(argv[0]);
      return 0;
    }
    else if(strcmp(arg, "--rate") == 0 && (i+1) < argc) {
      rate = Radiant::closestFrameRate(atof(argv[++i]));
    }
    else if(strcmp(arg, "--time") == 0 && (i+1) < argc) {
      secs = atoi(argv[++i]);
    }
    else if(strcmp(arg, "--triggermode") == 0 && (i+1) < argc) {
      triggerMode = (atoi(argv[++i]));
    }
    else if(strcmp(arg, "--triggerpolarity") == 0 && (i+1) < argc) {
      i++;
      if(strcmp(argv[i], "up") == 0) {
	;//FireView::CamView::setTriggerPolarity(DC1394_TRIGGER_ACTIVE_HIGH);
      }
      else if(strcmp(argv[i], "down") == 0) {
	; //FireView::CamView::setTriggerPolarity(DC1394_TRIGGER_ACTIVE_LOW);
      }
    }
    else if(strcmp(arg, "--triggersource") == 0 && (i+1) < argc) {
        triggerSource = (atoi(argv[++i]));
    }
    else if(strcmp(arg, "--verbose") == 0) {
      puts("Verbose mode");
      Radiant::enableVerboseOutput(true);
    }
    else if(strcmp(arg, "--config") == 0) {
      if ( (i + 1) < argc)
        configFile = argv[++i];
      else
        defaultconfig = true;

      useConfig = true;
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

  Radiant::Config conf;

  if(defaultconfig) {
    // Try to find a default configuration
    Radiant::ResourceLocator locator;
    locator.addModuleDataPath("MultiTouch/");
    locator.addPath(".", true);
    configFile = locator.locate("config.txt");

    if(configFile.empty()) {
      Radiant::error("FireCapture: Could not locate the standard configuration file");
      return -1;
    }
  }

  if(useConfig && !Radiant::FileUtils::fileReadable(configFile.c_str())) {
    Radiant::error("FireCapture: Configuration file %s is not readable", configFile.c_str());
    return -1;
  }

  if (useConfig) {
    if(!readConfig( & conf, configFile.c_str())) {
      Radiant::error("Failed to read MultiTouch configuration file: %s", configFile.c_str());
      return -1;
    } else {
      // Overwrite globals if specified
      Radiant::Chunk globals = conf.get("Globals");
      triggerSource = globals.get("camera-sync-source").getInt(-1);
      triggerMode = globals.get("camera-sync-method").getInt(-1);;
      rate = Radiant::closestFrameRate(globals.get("camera-sync-fps").getFloat(-1));
    }
  }

  std::vector<Radiant::VideoCamera::CameraInfo> cameras;
  Radiant::CameraDriver * cd = Radiant::VideoCamera::drivers().getPreferredCameraDriver();
  if(cd)
    cd->queryCameras(cameras);

  printf("Found %d FireWire cameras\n", (int) cameras.size());

  if(cameras.empty())
    return 0;

  std::vector<CameraThread *> threads;
  char buf[64];

  Radiant::Directory::mkdir(baseDir);

  for(i = 0; i < (int) cameras.size(); i++) {

    const Radiant::VideoCamera::CameraInfo & cam = cameras[i];
    printf("Camera %d: ID = %llx, VENDOR = %s, MODEL = %s\n",
           i + 1, (long long) cam.m_euid64,
           cam.m_vendor.c_str(), cam.m_model.c_str());
    fflush(0);

    sprintf(buf, "/raw-frames-%llx/", (long long) cam.m_euid64);
    CameraThread * thread = new CameraThread(cam.m_euid64, baseDir + buf);

    thread->setFormat7Mode(format7);
    thread->setFormat7Area(format7area);

    // Use config file if specified
    for(Radiant::Config::iterator it = conf.begin(); it != conf.end(); it++) {
      if(Radiant::Config::getName(it) == "Camera") {
        Radiant::Chunk & camChunk = Radiant::Config::getType(it);
        int64_t camUID;

        #ifdef WIN32
            long long lltmp = 0;
            sscanf(camChunk.get("devuid").getString().c_str(), "%llx", &lltmp);
            camUID = lltmp;
        #else
            camUID = strtoll(camChunk.get("devuid").getString().c_str(), 0, 16);
        #endif

        if(camUID == cam.m_euid64 || cameras.size() == 1) {
          thread->setFormat7Mode(camChunk.get("format7mode").getInt(-1));

          Nimble::Recti f7a;
          if(camChunk.get("format7area").getInts( f7a.low().data(), 4) == 4)
            thread->setFormat7Area(f7a);

          std::string cameraType(buf);

          camChunk.setClearFlag(true);
          camChunk.set("device",Radiant::Variant(cameraType.substr(1,25), ""));
          camChunk.setClearFlag(false);
        }
      }
    }

    threads.push_back(thread);

    thread->run();
  }

  std::string outConfigFile = (baseDir + std::string("config.txt"));
  // Cheat Radiant::writeConfig
  std::ofstream out;
  out.open(outConfigFile.c_str());
  out.close();

  Radiant::writeConfig( & conf, outConfigFile.c_str());

  Radiant::Sleep::sleepS(secs);

  for(int i = 0; i < (int) threads.size(); i++) {
    
    threads[i]->stop();
    delete threads[i];
  }
  
  return res;
}
