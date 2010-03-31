/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

// Some original source code by Juha Laitinen still may be around.

#include "VideoCameraCMU.hpp"

#include <Radiant/Trace.hpp>
#include <Radiant/Mutex.hpp>
#include <Radiant/Sleep.hpp>
#include <Radiant/Types.hpp>
#include <Radiant/Mutex.hpp>

#include <cassert>
#include <math.h>
#include <stdio.h>

#include <iostream>
#include <set>
#include <map>

#include <1394Camera.h>

#define NUM_BUFFERS 10

namespace Radiant {
    static MutexAuto g_mutex;

    static std::map<VideoCamera::FeatureType, CAMERA_FEATURE> g_featureToCMU;

    static CAMERA_FEATURE featureToCMU(VideoCamera::FeatureType id)
    {
        static bool once = true;
        if(once) {
            g_featureToCMU.insert(std::make_pair(VideoCamera::BRIGHTNESS, FEATURE_BRIGHTNESS));
            g_featureToCMU.insert(std::make_pair(VideoCamera::EXPOSURE, FEATURE_AUTO_EXPOSURE));
            g_featureToCMU.insert(std::make_pair(VideoCamera::SHARPNESS, FEATURE_SHARPNESS));
            g_featureToCMU.insert(std::make_pair(VideoCamera::WHITE_BALANCE, FEATURE_WHITE_BALANCE));
            g_featureToCMU.insert(std::make_pair(VideoCamera::HUE, FEATURE_HUE));
            g_featureToCMU.insert(std::make_pair(VideoCamera::SATURATION, FEATURE_SATURATION));
            g_featureToCMU.insert(std::make_pair(VideoCamera::GAMMA, FEATURE_GAMMA));
            g_featureToCMU.insert(std::make_pair(VideoCamera::IRIS, FEATURE_IRIS));
            g_featureToCMU.insert(std::make_pair(VideoCamera::FOCUS, FEATURE_FOCUS));
            g_featureToCMU.insert(std::make_pair(VideoCamera::ZOOM, FEATURE_ZOOM));
            g_featureToCMU.insert(std::make_pair(VideoCamera::PAN, FEATURE_PAN));
            g_featureToCMU.insert(std::make_pair(VideoCamera::TILT, FEATURE_TILT));
            g_featureToCMU.insert(std::make_pair(VideoCamera::SHUTTER, FEATURE_SHUTTER));
            g_featureToCMU.insert(std::make_pair(VideoCamera::GAIN, FEATURE_GAIN));
            g_featureToCMU.insert(std::make_pair(VideoCamera::TRIGGER, FEATURE_TRIGGER_MODE));
            g_featureToCMU.insert(std::make_pair(VideoCamera::TRIGGER_DELAY, FEATURE_TRIGGER_DELAY));
            g_featureToCMU.insert(std::make_pair(VideoCamera::FRAME_RATE, FEATURE_FRAME_RATE));
            g_featureToCMU.insert(std::make_pair(VideoCamera::TEMPERATURE, FEATURE_TEMPERATURE));
            g_featureToCMU.insert(std::make_pair(VideoCamera::WHITE_SHADING, FEATURE_WHITE_SHADING));
            g_featureToCMU.insert(std::make_pair(VideoCamera::OPTICAL_FILTER, FEATURE_OPTICAL_FILTER));
            g_featureToCMU.insert(std::make_pair(VideoCamera::CAPTURE_SIZE, FEATURE_CAPTURE_SIZE));
            g_featureToCMU.insert(std::make_pair(VideoCamera::CAPTURE_QUALITY, FEATURE_CAPTURE_QUALITY));

            once = false;
        }

        assert(g_featureToCMU.find(id) != g_featureToCMU.end());

        return g_featureToCMU[id];
    }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  VideoCameraCMU::VideoCameraCMU(CameraDriver * driver)
  : VideoCamera(driver),
    m_camera(0),
    m_initialized(false),
    m_timeoutMs(0),
    m_restartImageAcquisition(false)
  {
  }

  VideoCameraCMU::~VideoCameraCMU()
  {
    if (m_initialized)
      close();
  }

  ImageFormat VideoCameraCMU::imageFormat() const
  {
    return m_image.m_format;
  }

  unsigned int VideoCameraCMU::size() const
  {
    return m_image.size();
  }

  void VideoCameraCMU::setWhiteBalance(float /*u_to_blue*/, float /*v_to_red*/)
  {
  }

  void VideoCameraCMU::setFeature(VideoCamera::FeatureType feat, float value)
  {
      CAMERA_FEATURE feature = featureToCMU(feat);

      // If less than zero, use automatic mode
      if(value < 0.f) {
          setFeatureRaw(feat, -1);
          return;
      }

      C1394CameraControl * pcc = m_camera->GetCameraControl(feature);
      if (!pcc) {
          Radiant::error("VideoCameraCMU::setFeature # feature not found");
          return;
      };

    unsigned short low  = 0;
    unsigned short high = 0;

    pcc->GetRange(&low, &high);
    int32_t intVal = low + (value * (high - low));

    setFeatureRaw(feat, intVal);
  }

  void VideoCameraCMU::setFeatureRaw(VideoCamera::FeatureType feat, int32_t value)
  {    
    CAMERA_FEATURE feature = featureToCMU(feat);

    C1394CameraControl * pcc = m_camera->GetCameraControl(feature);
    if(!pcc) {
        Radiant::error("VideoCameraCMU::setFeatureRaw # feature not found");
        return;
    }

    pcc->SetAutoMode(value == -1 ? TRUE : FALSE);
    pcc->SetValue(unsigned short(value));
  }

  void VideoCameraCMU::getFeatures(std::vector<CameraFeature> * features)
  {
      features->clear();

      for(int id = VideoCamera::BRIGHTNESS; id < FEATURE_TYPE_MAX; id++)
        queryFeature(FeatureType(id), features);
  }

  void VideoCameraCMU::queryFeature(VideoCamera::FeatureType id, std::vector<CameraFeature> * features)
  {
    m_camera->RefreshControlRegisters(TRUE);

    CAMERA_FEATURE feature = featureToCMU(id);

    C1394CameraControl * pcc = m_camera->GetCameraControl(feature);
    if (!pcc)
        return;

    CameraFeature feat;

    feat.id = id;

    feat.absolute_capable = pcc->HasAbsControl();
    pcc->GetRangeAbsolute(&feat.abs_min, &feat.abs_max);
    feat.available = pcc->HasPresence();
    unsigned short valMin, valMax;
    pcc->GetRange(&valMin, &valMax);
    feat.min = valMin;
    feat.max = valMax;
    feat.on_off_capable = pcc->HasOnOff();

    // Figure out supported modes
    feat.num_modes = 0;
    if(pcc->HasManualMode())
        feat.modes[feat.num_modes++] = MODE_MANUAL;

    if(pcc->HasAutoMode())
        feat.modes[feat.num_modes++] = MODE_AUTO;

    if(pcc->HasOnePush())
        feat.modes[feat.num_modes++] = MODE_ONE_PUSH_AUTO;

    pcc->GetValueAbsolute(&feat.abs_value);

    unsigned short val;
    pcc->GetValue(&val);
    feat.value = val;

    feat.is_on = pcc->StatusOnOff();

    features->push_back(feat);
  }

  bool VideoCameraCMU::enableTrigger(VideoCamera::TriggerSource source)
  {    
      C1394CameraControlTrigger * pcct = m_camera->GetCameraControlTrigger();
      if (!pcct) {
          Radiant::error("VideoCameraCMU::enableTrigger # Could not get trigger control");
          return false;
      }

      if(pcct->SetOnOff(TRUE) != CAM_SUCCESS) {
          Radiant::error("VideoCameraCMU::enableTrigger # Failed to turn on trigger");
          return false;
      }

      unsigned short cmuSrc = unsigned short(source);
      // Not really sure about the software trigger value
      if(source == VideoCamera::TRIGGER_SOURCE_SOFTWARE)
          cmuSrc = 7;

      if(pcct->SetTriggerSource(cmuSrc) != CAM_SUCCESS) {
          Radiant::error("VideoCameraCMU::enableTrigger # Could not set trigger source");
          return false;
      }

      return true;
  }
  
  bool VideoCameraCMU::setTriggerMode(VideoCamera::TriggerMode mode)
  {
      C1394CameraControlTrigger * pcct = m_camera->GetCameraControlTrigger();
      if (!pcct) {
          Radiant::error("VideoCameraCMU::setTriggerMode # Failed to get trigger control");
          return false;
      }

      unsigned short cmuMode = unsigned short(mode);

      if (pcct->SetMode(cmuMode) != CAM_SUCCESS) {
          Radiant::error("VideoCameraCMU::setTriggerMode # Failed to set trigger mode");
          return false;
      }

      return true;
  }

  bool VideoCameraCMU::setTriggerPolarity(TriggerPolarity polarity)
  {
      C1394CameraControlTrigger * pcct = m_camera->GetCameraControlTrigger();
      if (!pcct) {
          Radiant::error("VideoCameraCMU::setTriggerPolarity # Failed to get trigger control");
          return false;
      }

      if(pcct->SetPolarity(polarity == TRIGGER_ACTIVE_HIGH ? TRUE : FALSE) != CAM_SUCCESS) {
          Radiant::error("VideoCameraCMU::setTriggerPolarity # Failed to set trigger polarity");
          return false;
      }

      return true;
  }

  bool VideoCameraCMU::disableTrigger()
  {
      C1394CameraControlTrigger * pcct = m_camera->GetCameraControlTrigger();
      if (!pcct) {
          Radiant::error("VideoCameraCMU::disableTrigger # Could not get trigger control");
          return false;
      }

      if(pcct->SetOnOff(FALSE) != CAM_SUCCESS) {
          Radiant::error("VideoCameraCMU::enableTrigger # Failed to turn on trigger");
          return false;
      }

      return true;
  }

  void VideoCameraCMU::sendSoftwareTrigger()
  {
    C1394CameraControlTrigger * pcct = m_camera->GetCameraControlTrigger();
    if(!pcct) {
        Radiant::error("VideoCameraCMU::sendSoftwareTrigger # Failed to get trigger control");
        return;
    }

    if(pcct->DoSoftwareTrigger() != CAM_SUCCESS)
        Radiant::error("VideoCameraCMU::sendSoftwareTrigger # failed to send software trigger");
  }

  bool VideoCameraCMU::setCaptureTimeout(int ms)
  {
      if(m_timeoutMs != ms) {
          m_restartImageAcquisition = true;
          m_timeoutMs = ms;
      }

      return true;
  }

  bool VideoCameraCMU::open(uint64_t euid, int, int, ImageFormat /*fmt*/, FrameRate framerate)
    {
      Guard guard(&g_mutex);

      m_camera = new C1394Camera();
      m_camera->RefreshCameraList();

      // Given the euid, find which camera it is on the bus
      std::vector<VideoCamera::CameraInfo> cameras;

      size_t cameraNum;
      for(cameraNum = 0; cameraNum < cameras.size(); cameraNum++)
          if(cameras[cameraNum].m_euid64 == euid) break;

      if(m_camera->SelectCamera(cameraNum) != CAM_SUCCESS) {
          Radiant::error("VideoCameraCMU::open # Failed to select camera %d", cameraNum);
          return false;
      }

      // Initialize the camera and force it to reset
      if(m_camera->InitCamera(TRUE) != CAM_SUCCESS) {
          Radiant::error("VideoCameraCMU::open # Failed to initialize camera");
          return false;
      }

      m_initialized = true;

      m_image.allocateMemory(IMAGE_GRAYSCALE, 640, 480);

      Radiant::debug("VideoCameraCMU::open # Camera max speed %d", m_camera->GetMaxSpeed());

    int mbps = m_camera->GetMaxSpeed();
    trace(Radiant::DEBUG, "CAMERA MAX SPEED %d", mbps);
		
    if(m_camera->SetVideoFormat(0) != CAM_SUCCESS) {
        Radiant::error("VideoCameraCMU::open # Failed to set video format 0");
        return false;
    }

    // Video mode 5 is 640x480 8-bit gray scale
    if(m_camera->SetVideoMode(5) != CAM_SUCCESS) {
        Radiant::error("VideoCameraCMU::open # Failed to set video mode 5");
        return false;
    }

    if(m_camera->SetVideoFrameRate(framerate) != CAM_SUCCESS) {
        Radiant::error("VideoCameraCMU::open # Failed to set video frame rate");
        return false;
    }

    return true;
  }

  bool VideoCameraCMU::openFormat7(uint64_t , Nimble::Recti , float , int )
  {
      Radiant::error("VideoCameraCMU::openFormat7 # not implemented");
      return false;
  }

  bool VideoCameraCMU::start()
  {
      if(m_camera->StartImageAcquisitionEx(NUM_BUFFERS, m_timeoutMs, ACQ_START_VIDEO_STREAM) != CAM_SUCCESS) {
          Radiant::error("VideoCameraCMU::start # Failed to start image acquisition");
          return false;
      }

      return true;
  }

  bool VideoCameraCMU::stop()
  {
      if(m_camera->StopImageAcquisition() != CAM_SUCCESS) {
          Radiant::error("VideoCameraCMU::stop # Failed to stop image acquisition");
          return false;
      }

      return true;
  }

  const VideoImage* VideoCameraCMU::captureImage()
  {
      if(m_restartImageAcquisition) {
          if(m_camera->StopImageAcquisition() != CAM_SUCCESS)
              Radiant::error("VideoCameraCMU::captureImage # Failed to stop image acquisition");

          //Radiant::info("VideoCameraCMU::captureImage # %d,%d", NUM_BUFFERS, m_timeoutMs);
          if(m_camera->StartImageAcquisitionEx(NUM_BUFFERS, m_timeoutMs, ACQ_START_VIDEO_STREAM) != CAM_SUCCESS)
              Radiant::error("VideoCameraCMU::Failed to start image acquisition");

          m_restartImageAcquisition = false;
      }

      if(m_camera->AcquireImageEx(TRUE, NULL) != CAM_SUCCESS) {
          Radiant::error("VideoCameraCMU::captureImage # Failed to acquire image");
          return 0;
      }

    unsigned long len = 0;
    m_image.m_planes[0].m_data = m_camera->GetRawData(&len);

    return & m_image;
  }

  bool VideoCameraCMU::close()
  {
      // Nothing to do here
      return false;
  }

  int VideoCameraCMU::width() const
  {
      return m_image.width();
  }

  int VideoCameraCMU::height() const
  {
      return m_image.height();
  }

  float VideoCameraCMU::fps() const
  {
      return -1;
  }

  VideoCameraCMU::CameraInfo VideoCameraCMU::cameraInfo()
  {
    CameraInfo info;

    if (m_camera) {
        char vendor[256],model[256];
        LARGE_INTEGER guid; 
        m_camera->GetCameraUniqueID(&guid);
        m_camera->GetCameraVendor(vendor, 256);
        m_camera->GetCameraName(model, 256);
        info.m_euid64 = guid.QuadPart;
        info.m_vendor = vendor;
        info.m_model  = model;
        info.m_driver = "cmu";
      }

    return info;
  }

  //////////////////////////////////////////////////////////////////////
   //////////////////////////////////////////////////////////////////////

  size_t CameraDriverCMU::queryCameras(std::vector<VideoCamera::CameraInfo> & cameras)
  {
    // Create a temporary CMU camera object
    C1394Camera tmpCam;
    size_t numCameras = tmpCam.RefreshCameraList();

    for(size_t i = 0; i < numCameras; i++) {
      tmpCam.SelectCamera(i);

      LARGE_INTEGER guid;
      tmpCam.GetCameraUniqueID(&guid);

      VideoCamera::CameraInfo info;
      info.m_driver = driverName();
      info.m_euid64 = guid.QuadPart;

      char buf[64];
      tmpCam.GetCameraVendor(buf, 64);

      info.m_vendor = buf;

      tmpCam.GetCameraName(buf, 64);
      info.m_model = buf;

      cameras.push_back(info);
    }

    return numCameras;
  }

  VideoCamera * CameraDriverCMU::createCamera()
  {
    return new VideoCameraCMU(this);
  }

}
