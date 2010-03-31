#include "VideoCameraPTGrey.hpp"

#include "Mutex.hpp"
#include "Trace.hpp"

#include <map>
#ifndef WIN32
#include <flycapture/FlyCapture2.h>
#else
#include <FlyCapture2.h>
#endif

#define NUM_BUFFERS 10

namespace Radiant
{
  /* It seems that ptgrey drivers are not 100% thread-safe. To overcome this we 
     use a mutex to lock captureImage calls to one-thread at a time. */
  static MutexStatic __cmutex; 

  typedef std::map<uint64_t, FlyCapture2::PGRGuid> GuidMap;
  GuidMap g_guidMap;

  static FlyCapture2::FrameRate framerateToPGR(Radiant::FrameRate fr)
  {
    switch(fr) {
    case FPS_5:
      return FlyCapture2::FRAMERATE_3_75;
      break;
    case FPS_10:
      return FlyCapture2::FRAMERATE_7_5;
      break;
    case FPS_30:
      return FlyCapture2::FRAMERATE_30;
      break;
    case FPS_60:
      return FlyCapture2::FRAMERATE_60;
      break;
    case FPS_120:
      return FlyCapture2::FRAMERATE_120;
    default:
    case FPS_15:
      return FlyCapture2::FRAMERATE_15;
      break;
    }
  }

  static std::map<FlyCapture2::PropertyType, VideoCamera::FeatureType> g_propertyFC2ToRadiant;
  static std::map<VideoCamera::FeatureType, FlyCapture2::PropertyType> g_propertyRadiantToFC2;

  static VideoCamera::FeatureType propertyToRadiant(FlyCapture2::PropertyType id)
  {
    static bool once = true;
    if(once) {
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::BRIGHTNESS, VideoCamera::BRIGHTNESS));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::AUTO_EXPOSURE, VideoCamera::EXPOSURE));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::SHARPNESS, VideoCamera::SHARPNESS));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::WHITE_BALANCE, VideoCamera::WHITE_BALANCE));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::HUE, VideoCamera::HUE));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::SATURATION, VideoCamera::SATURATION));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::GAMMA, VideoCamera::GAMMA));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::IRIS, VideoCamera::IRIS));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::FOCUS, VideoCamera::FOCUS));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::ZOOM, VideoCamera::ZOOM));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::PAN, VideoCamera::PAN));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::TILT, VideoCamera::TILT));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::SHUTTER, VideoCamera::SHUTTER));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::GAIN, VideoCamera::GAIN));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::TRIGGER_MODE, VideoCamera::TRIGGER));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::TRIGGER_DELAY, VideoCamera::TRIGGER_DELAY));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::FRAME_RATE, VideoCamera::FRAME_RATE));
      g_propertyFC2ToRadiant.insert(std::make_pair(FlyCapture2::TEMPERATURE, VideoCamera::TEMPERATURE));

      once = false;
    }

    assert(g_propertyFC2ToRadiant.find(id) != g_propertyFC2ToRadiant.end());

    return g_propertyFC2ToRadiant[id];
  }

  static FlyCapture2::PropertyType propertyToFC2(VideoCamera::FeatureType id)
  {
    static bool once = true;
    if(once) {
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::BRIGHTNESS, FlyCapture2::BRIGHTNESS));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::EXPOSURE, FlyCapture2::AUTO_EXPOSURE));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::SHARPNESS, FlyCapture2::SHARPNESS));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::WHITE_BALANCE, FlyCapture2::WHITE_BALANCE));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::HUE, FlyCapture2::HUE));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::SATURATION, FlyCapture2::SATURATION));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::GAMMA, FlyCapture2::GAMMA));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::IRIS, FlyCapture2::IRIS));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::FOCUS, FlyCapture2::FOCUS));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::ZOOM, FlyCapture2::ZOOM));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::PAN, FlyCapture2::PAN));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::TILT, FlyCapture2::TILT));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::SHUTTER, FlyCapture2::SHUTTER));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::GAIN, FlyCapture2::GAIN));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::TRIGGER, FlyCapture2::TRIGGER_MODE));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::TRIGGER_DELAY, FlyCapture2::TRIGGER_DELAY));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::FRAME_RATE, FlyCapture2::FRAME_RATE));
      g_propertyRadiantToFC2.insert(std::make_pair(VideoCamera::TEMPERATURE, FlyCapture2::TEMPERATURE));

      once = false;
    }

    assert(g_propertyRadiantToFC2.find(id) != g_propertyRadiantToFC2.end());

    return g_propertyRadiantToFC2[id];
  }

  ////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////

  static FlyCapture2::BusManager * g_bus = 0;

  void g_busResetCallback(void * /*param*/)
  {
    Radiant::info("FIREWIRE BUS RESET");
  }

  VideoCameraPTGrey::VideoCameraPTGrey(CameraDriver * driver)
    : VideoCamera(driver),
      m_state(UNINITIALIZED)
  {
  }

  bool VideoCameraPTGrey::open(uint64_t euid, int , int , ImageFormat , FrameRate framerate)
  {

    GuardStatic g(__cmutex);

	debug("VideoCameraPTGrey::open # %llx", (long long) euid);

    FlyCapture2::PGRGuid guid;

    // If the euid is zero, take the first camera
    if(euid == 0) {
		if(g_guidMap.empty()) {
				error("VideoCameraPTGrey::open # No Cameras found");
        return false;
		}
      guid = g_guidMap.begin()->second;
    } else {
      GuidMap::iterator it = g_guidMap.find(euid);
      if(it == g_guidMap.end()) {
        Radiant::error("VideoCameraPTGrey::open # guid not found");
        return false;
      }

      guid = it->second;
    }

    // Connect camera
    FlyCapture2::Error err = m_camera.Connect(&guid);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      return false;
    }

    // Set video mode and framerate
    err = m_camera.SetVideoModeAndFrameRate(FlyCapture2::VIDEOMODE_640x480Y8, framerateToPGR(framerate));
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      return false;
    }

    m_image.allocateMemory(IMAGE_GRAYSCALE, 640, 480);

    // Set BUFFER_FRAMES & capture timeout
    FlyCapture2::FC2Config config;
    /*
    err = m_camera.GetConfiguration(&config);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      return false;
    }
    */
    config.grabMode = FlyCapture2::BUFFER_FRAMES;
    config.numBuffers = NUM_BUFFERS;
    config.bandwidthAllocation = FlyCapture2::BANDWIDTH_ALLOCATION_ON;
    config.isochBusSpeed = FlyCapture2::BUSSPEED_S400;
    config.asyncBusSpeed = FlyCapture2::BUSSPEED_ANY;
    config.grabTimeout = 0;
    config.numImageNotifications = 1;

    err = m_camera.SetConfiguration(&config);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      err.PrintErrorTrace();
      return false;
    }

    FlyCapture2::VideoMode vm;
    FlyCapture2::FrameRate fr;

    err = m_camera.GetVideoModeAndFrameRate(&vm, &fr);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      err.PrintErrorTrace();
      return false;
    }

    // Set trigger delay to zero just in case
    FlyCapture2::TriggerDelay td;
    td.type = FlyCapture2::TRIGGER_DELAY;
    td.valueA = 0;
    td.valueB = 0;

    err = m_camera.SetTriggerDelay(&td, true);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      err.PrintErrorTrace();
      return false;
    }

	m_state = OPENED;


    FlyCapture2::CameraInfo camInfo;
    err = m_camera.GetCameraInfo(&camInfo);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
    }

    m_info.m_vendor = camInfo.vendorName;
    m_info.m_model = camInfo.modelName;
    m_info.m_euid64 = 0;
    m_info.m_driver = driver()->driverName();

    return true;
  }

  bool VideoCameraPTGrey::openFormat7(uint64_t euid, Nimble::Recti roi, float fps, int mode)
  {
    GuardStatic g(__cmutex);

	debug("VideoCameraPTGrey::openFormat7 # %llx", (long long) euid);

    // Look up PGRGuid from our map (updated in queryCameras())
    GuidMap::iterator it = g_guidMap.find(euid);
    if(it == g_guidMap.end()) {
      Radiant::error("VideoCameraPTGrey::open # guid not found");
      return false;
    }

    FlyCapture2::PGRGuid guid = it->second;

    // Connect camera
    FlyCapture2::Error err = m_camera.Connect(&guid);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      return false;
    }

    // Set BUFFER_FRAMES & capture timeout
    FlyCapture2::FC2Config config;
    err = m_camera.GetConfiguration(&config);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      return false;
    }

    config.grabMode = FlyCapture2::BUFFER_FRAMES;
    config.numBuffers = NUM_BUFFERS;
    config.bandwidthAllocation = FlyCapture2::BANDWIDTH_ALLOCATION_ON;
    config.isochBusSpeed = FlyCapture2::BUSSPEED_S400;

    err = m_camera.SetConfiguration(&config);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      return false;
    }

    // Make sure the image size is divisible by four
    int roiWidth = (roi.width() + 3) & ~0x3;
    int roiHeight = (roi.height() + 3) & ~0x3;
    roi.high().x += roiWidth - roi.width();
    roi.high().y += roiHeight - roi.height();

    // Query format7 info for the requested mode
    FlyCapture2::Format7Info f7info;
    f7info.mode = FlyCapture2::Mode(mode);

    bool supported;
    err = m_camera.GetFormat7Info(&f7info, &supported);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      return false;
    }

    // Set Format7 frame size
    FlyCapture2::Format7ImageSettings f7s;
    f7s.offsetX = roi.low().x;
    f7s.offsetY = roi.low().y;
    f7s.width = (roi.width() < int(f7info.maxWidth)) ? roi.width() : f7info.maxWidth;
    f7s.height = (roi.height() < int(f7info.maxHeight)) ? roi.height() : f7info.maxHeight;
    f7s.pixelFormat = FlyCapture2::PIXEL_FORMAT_MONO8;
    f7s.mode = FlyCapture2::Mode(mode);

    // Define set fps by adjusting the packet size

    // Cycles in the FireWire bus
    const int BUS_CYCLES_PER_SECOND = 8000;
    // How many bus cycles per frame we need?
    unsigned int busCyclesPerFrame = ceil(BUS_CYCLES_PER_SECOND / fps);
    // Frame size in bytes
    unsigned int frameSizeInBytes = f7s.width * f7s.height;
    // Needed packet size
    unsigned int packetSize = frameSizeInBytes / busCyclesPerFrame;

    // If the requested packetSize exceeds the maximum supported, clamp it
    if(packetSize > f7info.maxPacketSize) {
      Radiant::error("VideoCameraPTGrey::open # requested camera fps (%f) is too high. Using slower.", fps);
      packetSize = f7info.maxPacketSize;
    }

    // Validate
    Radiant::info("Validating format7 settings...");
    FlyCapture2::Format7PacketInfo f7pi;
    err = m_camera.ValidateFormat7Settings(&f7s, &supported, &f7pi);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      err.PrintErrorTrace();
    }

    Radiant::info("FORMAT7 SETTINGS:");
    Radiant::info("\tOffset %d %d", f7s.offsetX, f7s.offsetY);
    Radiant::info("\tSize %d %d", f7s.width, f7s.height);
    Radiant::info("\tMode %d", f7s.mode);
    Radiant::info("\tPacket size: %d [%d, %d]", packetSize, f7info.minPacketSize, f7info.maxPacketSize);

    Radiant::info("PACKET INFO");
    Radiant::info("\tRecommended packet size: %d", f7pi.recommendedBytesPerPacket);
    Radiant::info("\tMax bytes packet size: %d", f7pi.maxBytesPerPacket);
    Radiant::info("\tUnit bytes per packet: %d", f7pi.unitBytesPerPacket);

    err = m_camera.SetFormat7Configuration( &f7s, f7pi.recommendedBytesPerPacket);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
      err.PrintErrorTrace();
      return false;
    }

    // Allocate space for image
    m_image.allocateMemory(IMAGE_GRAYSCALE, f7s.width, f7s.height);

	m_state = OPENED;

    FlyCapture2::CameraInfo camInfo;
    err = m_camera.GetCameraInfo(&camInfo);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::open # %s", err.GetDescription());
    }

    m_info.m_vendor = camInfo.vendorName;
    m_info.m_model = camInfo.modelName;
    m_info.m_euid64 = 0;
    m_info.m_driver = driver()->driverName();

    return true;
  }

  bool VideoCameraPTGrey::start()
  {
    GuardStatic g(__cmutex);

	  if(m_state != OPENED) {
		  error("VideoCameraPTGrey::start # State != OPENED");
		  /* If the device is already running, then return true. */
		  return m_state == RUNNING;
	  }

    FlyCapture2::Error err = m_camera.StartCapture();
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::start # %s", err.GetDescription());
      err.PrintErrorTrace();
      return false;
    }

	m_state = RUNNING;

    return true;
  }

  bool VideoCameraPTGrey::stop()
  {
    // GuardStatic g(__cmutex);

	  if(m_state != RUNNING) {
		  error("VideoCameraPTGrey::stop # State != RUNNING");
		  /* If the device is already stopped, then return true. */
		  return m_state == OPENED;
	  }

    Radiant::info("VideoCameraPTGrey::stop");
    FlyCapture2::Error err = m_camera.StopCapture();
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::stop # %s", err.GetDescription());
      return false;
    }

	m_state = OPENED;

    return true;
  }

  bool VideoCameraPTGrey::close()
  {
    // GuardStatic g(__cmutex);

    Radiant::info("VideoCameraPTGrey::close");
    m_camera.Disconnect();

	m_state = UNINITIALIZED;

    return true;
  }

  const Radiant::VideoImage * VideoCameraPTGrey::captureImage()
  {
    GuardStatic g(__cmutex);

    FlyCapture2::Image img;
    FlyCapture2::Error err = m_camera.RetrieveBuffer(&img);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::captureImage # %s", err.GetDescription());
      return false;
    }
    /*
    if(m_image.size() != img.GetDataSize()) {
      Radiant::info("ALLOCATED %dx%d bytes %d", m_image.width(), m_image.height(), m_image.size());
      Radiant::info("FRAME %dx%d stride %d bytes %d", img.GetCols(), img.GetRows(), img.GetStride(), img.GetDataSize());

      //assert(m_image.size() == img.GetDataSize());
    }
*/
    memcpy(m_image.m_planes[0].m_data, img.GetData(), m_image.size());

    return &m_image;
  }

  VideoCamera::CameraInfo VideoCameraPTGrey::cameraInfo()
  {
    // GuardStatic g(__cmutex);
    return m_info;
	
  }

  int VideoCameraPTGrey::width() const
  {
    return m_image.m_width;
  }

  int VideoCameraPTGrey::height() const
  {
    return m_image.m_height;
  }

  float VideoCameraPTGrey::fps() const
  {
    return -1;
  }

  ImageFormat VideoCameraPTGrey::imageFormat() const
  {
    return Radiant::IMAGE_GRAYSCALE;
  }

  unsigned int VideoCameraPTGrey::size() const
  {
    return width() * height() * sizeof(uint8_t);
  }
  
  void VideoCameraPTGrey::setFeature(FeatureType id, float value)
  {
    // Radiant::debug("VideoCameraPTGrey::setFeature # %d %f", id, value);

    // If less than zero, use automatic mode
    if(value < 0.f) {
      setFeatureRaw(id, -1);
      return;
    }

    FlyCapture2::PropertyInfo pinfo;
    pinfo.type = propertyToFC2(id);

    FlyCapture2::Error err = m_camera.GetPropertyInfo(&pinfo);
    if(err != FlyCapture2::PGRERROR_OK) {
		Radiant::debug("VideoCameraPTGrey::setFeature # Failed: \"%s\"",
			err.GetDescription());
      return;
    }

    int32_t intVal = pinfo.min + (value * (pinfo.max - pinfo.min));

    setFeatureRaw(id, intVal);
  }

  void VideoCameraPTGrey::setFeatureRaw(FeatureType id, int32_t value)
  {
    // Radiant::debug("VideoCameraPTGrey::setFeatureRaw # %d %d", id, value);

    FlyCapture2::Property prop;
    prop.type = propertyToFC2(id);

    m_camera.GetProperty(&prop);
    /*
    Radiant::info("DEBUG: BEFORE ADJUSTMENT");
    Radiant::info("type %d", prop.type);
    Radiant::info("present %d", prop.present);
    Radiant::info("abs control %d", prop.absControl);
    Radiant::info("one push %d", prop.onePush);
    Radiant::info("on/off %d", prop.onOff);
    Radiant::info("autoManual %d", prop.autoManualMode);
    Radiant::info("value A %d", prop.valueA);
    Radiant::info("value B %d", prop.valueB);
    Radiant::info("abs value %f", prop.absValue);
*/

    prop.valueA = value;
    prop.valueB = value;

    // Automatic or manual mode?
    prop.autoManualMode = value < 0 ? true : false;

    FlyCapture2::Error err = m_camera.SetProperty(&prop);
    if(err != FlyCapture2::PGRERROR_OK) {
		Radiant::debug("VideoCameraPTGrey::setFeatureRaw # Failed: \"%s\"",
			err.GetDescription());
      err.PrintErrorTrace();
    }
    /*
    m_camera.GetProperty(&prop);
    Radiant::info("DEBUG: AFTER ADJUSTMENT");
    Radiant::info("abs control %d", prop.absControl);
    Radiant::info("value A %d", prop.valueA);
    Radiant::info("value B %d", prop.valueB);
*/
  }

  void VideoCameraPTGrey::setWhiteBalance(float /*u_to_blue*/, float /*v_to_red*/)
  {
    Radiant::error("VideoCameraPTGrey::setWhiteBalance # not implemented");
    assert(0);
  }

  bool VideoCameraPTGrey::enableTrigger(TriggerSource src)
  {

    FlyCapture2::TriggerMode tm;

    FlyCapture2::Error err = m_camera.GetTriggerMode(&tm);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::enableTrigger # %s", err.GetDescription());
      return false;
    }

    tm.onOff = true;
    tm.source = (unsigned int)(src);

    err = m_camera.SetTriggerMode(&tm);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::enableTrigger # %s", err.GetDescription());
      return false;
    }

    return true;
  }

  bool VideoCameraPTGrey::setTriggerMode(TriggerMode mode)
  {
    FlyCapture2::TriggerMode tm;

    FlyCapture2::Error err = m_camera.GetTriggerMode(&tm);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::enableTrigger # %s", err.GetDescription());
      return false;
    }

    tm.mode = (unsigned int)(mode);

    err = m_camera.SetTriggerMode(&tm);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::enableTrigger # %s", err.GetDescription());
      return false;
    }

    return true;
  }

  bool VideoCameraPTGrey::setTriggerPolarity(TriggerPolarity polarity)
  {
    FlyCapture2::TriggerMode tm;

    FlyCapture2::Error err = m_camera.GetTriggerMode(&tm);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::setTriggerPolarity # %s", err.GetDescription());
      return false;
    }

    tm.polarity = (unsigned int)(polarity);

    err = m_camera.SetTriggerMode(&tm);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::setTriggerPolarity # %s", err.GetDescription());
      return false;
    }

    return true;
  }

  bool VideoCameraPTGrey::disableTrigger()
  {
    // The TriggerMode is initialized to disabled state
    FlyCapture2::TriggerMode mode;

    FlyCapture2::Error err = m_camera.SetTriggerMode(&mode);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::disableTrigger # %s", err.GetDescription());
      return false;
    }

    return true;
  }

  void VideoCameraPTGrey::sendSoftwareTrigger()
  {
    FlyCapture2::Error err = m_camera.FireSoftwareTrigger();
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::sendSoftwareTrigger # %s", err.GetDescription());
    }
  }

  bool VideoCameraPTGrey::setCaptureTimeout(int ms)
  {
    // Read the current camera configuration
    FlyCapture2::FC2Config config;
    FlyCapture2::Error err = m_camera.GetConfiguration(&config);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::setCaptureTimeout # %s", err.GetDescription());
      return false;
    }

    // Modify the capture timeout
    config.grabTimeout = ms;

    // Set the new configuration
    err = m_camera.SetConfiguration(&config);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::setCaptureTimeout # %s", err.GetDescription());
      return false;
    }

    return true;
  }

  void VideoCameraPTGrey::queryFeature(FlyCapture2::PropertyType id, std::vector<CameraFeature> * features)
  {
    FlyCapture2::PropertyInfo pinfo;
    pinfo.type = id;

    FlyCapture2::Error err = m_camera.GetPropertyInfo(&pinfo);
    if(err != FlyCapture2::PGRERROR_OK) {
      //Radiant::error("VideoCameraPTGrey::getFeatures # %s", err.GetDescription());
      return;
    }

    if(!pinfo.present) { Radiant::debug("Skipping feature %d, not present", id); return; }

    VideoCamera::CameraFeature feat;
    feat.id = propertyToRadiant(id);

    feat.absolute_capable = pinfo.absValSupported;
    feat.abs_max = pinfo.absMax;
    feat.abs_min = pinfo.absMin;
    feat.available = pinfo.present;
    feat.max = pinfo.max;
    feat.min = pinfo.min;
    feat.on_off_capable = pinfo.onOffSupported;

    // Figure out supported modes
    feat.num_modes = 0;
    if(pinfo.manualSupported)
      feat.modes[feat.num_modes++] = MODE_MANUAL;

    if(pinfo.autoSupported)
      feat.modes[feat.num_modes++] = MODE_AUTO;

    if(pinfo.onePushSupported)
      feat.modes[feat.num_modes++] = MODE_ONE_PUSH_AUTO;

    FlyCapture2::Property prop;
    prop.type = pinfo.type;
    err = m_camera.GetProperty(&prop);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::getFeatures # %s", err.GetDescription());
      return;
    }

    feat.abs_value = prop.absValue;
    feat.value = prop.valueA;
    feat.is_on = prop.onOff;

    features->push_back(feat);
  }

  void VideoCameraPTGrey::getFeatures(std::vector<CameraFeature> * features)
  {
    features->clear();

    for(int type = FlyCapture2::BRIGHTNESS; type <= FlyCapture2::TEMPERATURE; type++)
      queryFeature(FlyCapture2::PropertyType(type), features);
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////
  static std::vector<VideoCamera::CameraInfo> g_cameras;

  CameraDriverPTGrey::CameraDriverPTGrey()
  {
    // Initialize the mutex.
    __cmutex.lock();
	__cmutex.unlock();
  }

  size_t CameraDriverPTGrey::queryCameras(std::vector<VideoCamera::CameraInfo> & suppliedCameras)
  {
    static bool wasRun = false;
    if(wasRun) {
      suppliedCameras.insert(suppliedCameras.begin(), g_cameras.begin(), g_cameras.end());
	  return g_cameras.size();
    }

	std::vector<VideoCamera::CameraInfo> myCameras;

    // Clear guid map
    g_guidMap.clear();

    if(!g_bus) g_bus = new FlyCapture2::BusManager();

    g_bus->RegisterCallback(g_busResetCallback, FlyCapture2::BUS_RESET, 0, 0);

    // Get the number of available cameras
    unsigned int numCameras;
    FlyCapture2::Error err = g_bus->GetNumOfCameras(&numCameras);
    if(err != FlyCapture2::PGRERROR_OK) {
      Radiant::error("VideoCameraPTGrey::queryCameras # %s", err.GetDescription());
      return false;
    }

    for(uint32_t i = 0; i < numCameras; i++) {
      FlyCapture2::PGRGuid guid;

      // Query camera guid
      err = g_bus->GetCameraFromIndex(i, &guid);
      if(err != FlyCapture2::PGRERROR_OK) {
        Radiant::error("VideoCameraPTGrey::queryCameras # %s", err.GetDescription());
        return false;
      }

      // Connect camera based on guid
      FlyCapture2::Camera camera;
      err = camera.Connect(&guid);
      if(err != FlyCapture2::PGRERROR_OK) {
        Radiant::error("VideoCameraPTGrey::queryCameras # %s", err.GetDescription());
        return false;
      }

      // Query camera info
      FlyCapture2::CameraInfo cameraInfo;
      err = camera.GetCameraInfo(&cameraInfo);
      if(err != FlyCapture2::PGRERROR_OK) {
        Radiant::error("VideoCameraPTGrey::queryCameras # %s", err.GetDescription());
        return false;
      }

      uint64_t a = cameraInfo.configROM.nodeVendorId;
      uint64_t b = cameraInfo.configROM.chipIdHi;
      //uint64_t c = cameraInfo.configROM.unitSpecId;
      uint64_t d = cameraInfo.configROM.chipIdLo;

      // This is a complete guess. Need to verify if this is actually correct
      uint64_t uuid = (a << 40) | (b << 32) | (d);

      g_guidMap.insert(std::make_pair(uuid, guid));

      VideoCamera::CameraInfo myInfo;

      myInfo.m_vendor = cameraInfo.vendorName;
      myInfo.m_model = cameraInfo.modelName;
      myInfo.m_euid64 = uuid;
      myInfo.m_driver = driverName();

      myCameras.push_back(myInfo);
    }
	
	// Cache the results for later
	/// @todo caching the results is not so good idea, because you can't hotplug cameras now	
	g_cameras = myCameras;
	// Append to the camera vector
	suppliedCameras.insert(suppliedCameras.end(), myCameras.begin(), myCameras.end());

    wasRun = true;

    return numCameras;
  }

  VideoCamera * CameraDriverPTGrey::createCamera()
  {
    return new VideoCameraPTGrey(this);
  }

}
