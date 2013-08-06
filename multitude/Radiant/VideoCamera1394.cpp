/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

// Some original source code by Juha Laitinen still may be around.

#include "Platform.hpp"

#ifdef CAMERA_DRIVER_1394

#define __STDC_FORMAT_MACROS

#include "VideoCamera1394.hpp"

#include "Radiant.hpp"
#include "Trace.hpp"
#include "Mutex.hpp"
#include "Sleep.hpp"
#include "Types.hpp"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <set>
#include <algorithm>

#include <dc1394/camera.h>

#include <sys/utsname.h>
#include <inttypes.h>

#define NUM_BUFFERS 10

namespace {

  Radiant::Mutex s_queryCamerasMutex;
  dc1394_t * s_dc = 0;

  Radiant::Mutex s_infosMutex;
  std::vector<dc1394camera_t *> s_infos;

  Radiant::Mutex s_takenMutex;
  std::set<int> s_taken;

  dc1394feature_t featureTypeToNative(Radiant::VideoCamera::FeatureType id)
  {
    dc1394feature_t result = dc1394feature_t(int(DC1394_FEATURE_BRIGHTNESS) + int(id));

    return result;
  }

  Radiant::VideoCamera::FeatureType featureTypeFromNative(dc1394feature_t id)
  {
    Radiant::VideoCamera::FeatureType result =
        Radiant::VideoCamera::FeatureType(int(id) - int(DC1394_FEATURE_BRIGHTNESS));

    return result;
  }

  void initDc()
  {
    MULTI_ONCE { s_dc = dc1394_new(); }
  }
}

namespace Radiant {

  inline int diFPS2dcFPS(FrameRate fps)
  {

    switch(fps) {
    case FPS_IGNORE:
      return DC1394_FRAMERATE_15;
    case FPS_5:
      return DC1394_FRAMERATE_3_75;
    case FPS_10:
      return DC1394_FRAMERATE_7_5;
    case FPS_15:
      return DC1394_FRAMERATE_15;
    case FPS_30:
      return DC1394_FRAMERATE_30;
    case FPS_60:
      return DC1394_FRAMERATE_60;
    case FPS_120:
      return DC1394_FRAMERATE_120;
    case FPS_COUNT:
      ;
    }

    return DC1394_FRAMERATE_15;
  }

  static VideoCamera::CameraFeature fromNativeFeature(const dc1394feature_info_t & native)
  {
    VideoCamera::CameraFeature feat;

    feat.id = featureTypeFromNative(native.id);

    feat.absolute_capable = native.absolute_capable;
    feat.abs_max = native.abs_max;
    feat.abs_min = native.abs_min;
    feat.abs_value = native.abs_value;
    feat.available = native.available;

    if(native.current_mode == DC1394_FEATURE_MODE_MANUAL)
      feat.current_mode = VideoCamera::MODE_MANUAL;
    else if(native.current_mode == DC1394_FEATURE_MODE_AUTO)
      feat.current_mode = VideoCamera::MODE_AUTO;
    else if(native.current_mode == DC1394_FEATURE_MODE_ONE_PUSH_AUTO)
      feat.current_mode = VideoCamera::MODE_ONE_PUSH_AUTO;


    feat.num_modes = 0;

    for(unsigned i = 0; i < native.modes.num && i < VideoCamera::MODE_MAX; i++) {
      dc1394feature_mode_t fm = native.modes.modes[i];

      if(fm == DC1394_FEATURE_MODE_MANUAL) {
        feat.modes[feat.num_modes++] = VideoCamera::MODE_MANUAL;
      }
      else if(fm == DC1394_FEATURE_MODE_AUTO) {
        feat.modes[feat.num_modes++] = VideoCamera::MODE_AUTO;
      }
      else if(fm == DC1394_FEATURE_MODE_ONE_PUSH_AUTO) {
        feat.modes[feat.num_modes++] = VideoCamera::MODE_ONE_PUSH_AUTO;
      }
    }

    feat.is_on = native.is_on;
    feat.max = native.max;
    feat.min = native.min;
    feat.on_off_capable = native.on_off_capable;
    feat.polarity_capable = native.polarity_capable;
    feat.readout_capable = native.readout_capable;
    feat.value = native.value;

    return feat;
  }

  const char * fps_labels[] =
  {
    "1.875 fps",
    "3.75 fps",
    "7.5 fps",
    "15 fps",
    "30 fps",
    "60 fps",
    "120 fps",
    "240 fps"
  };

  const char * format0_labels[]=
  {
    "Format 0, Mode 0: 160x120 YUV (4:4:4)",
    "Format 0, Mode 1: 320x240 YUV (4:2:2)",
    "Format 0, Mode 2: 640x480 YUV (4:1:1)",
    "Format 0, Mode 3: 640x480 YUV (4:2:2)",
    "Format 0, Mode 4: 640x480 RGB 24bpp",
    "Format 0, Mode 5: 640x480 Mono 8bpp",
    "Format 0, Mode 6: 640x480 Mono 16bpp"
  };

  dc1394video_mode_t difmt2dcfmt(ImageFormat fmt, int w, int h)
  {
    if(w == 640 && h == 480) {
      if(fmt == IMAGE_RAWBAYER || fmt == IMAGE_GRAYSCALE)
        return DC1394_VIDEO_MODE_640x480_MONO8;
      else if(fmt == IMAGE_YUV_411 || fmt == IMAGE_YUV_411P)
        return DC1394_VIDEO_MODE_640x480_YUV411;
      else if(fmt == IMAGE_YUV_422 || fmt == IMAGE_YUV_422P)
        return DC1394_VIDEO_MODE_640x480_YUV422;
      else
        return DC1394_VIDEO_MODE_640x480_YUV411;
    }
    else if(w == 1024 && h == 768) {
      if(fmt == IMAGE_RAWBAYER || fmt == IMAGE_GRAYSCALE)
        return DC1394_VIDEO_MODE_1024x768_MONO8;
      else if(fmt == IMAGE_YUV_422 || fmt == IMAGE_YUV_422P)
        return DC1394_VIDEO_MODE_1024x768_YUV422;
      else
        return DC1394_VIDEO_MODE_1024x768_YUV422;
    }

    return DC1394_VIDEO_MODE_640x480_YUV411;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  static int g_count = 0;
  static int s_openDelay = 850;

  VideoCamera1394::VideoCamera1394(CameraDriver * driver)
      : VideoCamera(driver),
      m_camera(0),
      m_frame(0),
      m_outside(0),
      m_fps(-1),
      m_timeoutUs(-1)
  {
    m_videodevice = "/dev/video1394";
    m_framesBehind = 0;
    m_initialized = false;
    m_euid = 0;
    m_cameraNum = 0;
    m_started = false;

    initDc();

    if (g_count == 0) {
      char * s = getenv("MULTI_CAM_OPEN_DELAY");
      if (s) {
        s_openDelay = atoi(s);
      }
    }

    g_count++;
  }

  VideoCamera1394::~VideoCamera1394()
  {
    if (m_initialized || m_camera)
      close();

    g_count--;
  }

  ImageFormat VideoCamera1394::imageFormat() const
  {
    return m_image.m_format;
  }

  unsigned int VideoCamera1394::size() const
  {
    return m_image.size();
  }

  dc1394error_t has_mode(dc1394camera_t * camera,
                         dc1394feature_t feature,
                         dc1394feature_mode_t mode,
                         dc1394bool_t * val)
  {
    dc1394feature_modes_t modes;
    modes.num = 0;
    dc1394error_t ret = dc1394_feature_get_modes(camera, feature, & modes);

    *val = DC1394_FALSE;

    for(uint i = 0; i < modes.num; i++)
      if(modes.modes[i] == mode)
        *val = DC1394_TRUE;

    return ret;
  }

  dc1394error_t has_auto_mode(dc1394camera_t *camera,
                              dc1394feature_t feature,
                              dc1394bool_t * val)
  {
    return has_mode(camera, feature, DC1394_FEATURE_MODE_AUTO, val);
  }

  dc1394error_t has_manual_mode(dc1394camera_t *camera,
                                dc1394feature_t feature,
                                dc1394bool_t * val)
  {
    return has_mode(camera, feature, DC1394_FEATURE_MODE_MANUAL, val);
  }

  void VideoCamera1394::setWhiteBalance(float u_to_blue, float v_to_red)
  {
    const char * fname = "VideoCamera1394::setWhiteBalance";

    dc1394bool_t b = DC1394_FALSE;

    if(u_to_blue < 0 || v_to_red < 0) {

      has_auto_mode(m_camera, DC1394_FEATURE_WHITE_BALANCE, &b);

      if(b)
        dc1394_feature_set_mode(m_camera,
                                DC1394_FEATURE_WHITE_BALANCE,
                                DC1394_FEATURE_MODE_AUTO);
      else
        debugRadiant("%s # no auto mode for white balance", fname);
    }
    else {

      has_manual_mode(m_camera,
                      DC1394_FEATURE_WHITE_BALANCE,
                      & b);

      if(b)
        dc1394_feature_set_mode(m_camera,
                                DC1394_FEATURE_WHITE_BALANCE,
                                DC1394_FEATURE_MODE_MANUAL);
      else {
        debugRadiant("%s # no manual mode for white balance", fname);
        return;
      }

      uint32_t low  = 0;
      uint32_t high = 0;

      dc1394_feature_get_boundaries(m_camera, DC1394_FEATURE_WHITE_BALANCE,
                                    & low, & high);

      uint32_t ublue, vred;

      if(low != high) {
        float s = high - low;

        ublue = (uint32_t) (s * u_to_blue + low);
        vred  = (uint32_t) (s * v_to_red  + low);

        if(ublue > high)
          ublue = high;

        if(vred > high)
          vred = high;
      }
      else {
        ublue = (uint32_t) round(u_to_blue);
        vred  = (uint32_t) round(v_to_red);
      }

      dc1394_feature_whitebalance_set_value(m_camera, ublue, vred);
    }
  }

  void VideoCamera1394::setFeature(VideoCamera::FeatureType id, float value)
  {
    const char * fname = "VideoCamera1394::setFeature1394";

    dc1394feature_t feature = featureTypeToNative(id);

    const char * name = dc1394_feature_get_string(feature);

    bool automatic = value < 0.0f;

    dc1394bool_t b = DC1394_FALSE;

    if(automatic) {

      has_auto_mode(m_camera, feature, & b);

      if(b)
        dc1394_feature_set_mode(m_camera, feature, DC1394_FEATURE_MODE_AUTO);
      else
        debugRadiant("%s # no auto mode for %s", fname, name);
    }
    else {
      has_manual_mode(m_camera, feature, & b);

      if(b)
        dc1394_feature_set_mode(m_camera, feature, DC1394_FEATURE_MODE_MANUAL);
      else {
        debugRadiant("%s # no manual mode for %s", fname, name);
        return;
      }

      uint32_t low  = 0;
      uint32_t high = 0;

      dc1394_feature_get_boundaries(m_camera, feature, & low, & high);

      uint32_t uvalue;

      if(low != high) {

        uvalue = (uint) ((high - low) * value + low);

        if(uvalue > high)
          uvalue = high;
      }
      else
        uvalue = (uint32_t) round(value);

      dc1394_feature_set_value(m_camera, feature, uvalue);
    }
  }

  void VideoCamera1394::setFeatureRaw(VideoCamera::FeatureType id, int32_t value)
  {
    dc1394feature_t feature = featureTypeToNative(id);

    dc1394_feature_set_mode(m_camera,  feature, DC1394_FEATURE_MODE_MANUAL);
    dc1394_feature_set_value(m_camera, feature, value);
  }

  void VideoCamera1394::getFeatures(std::vector<CameraFeature> * feats)
  {
    assert(isInitialized());

    dc1394featureset_t features;
    dc1394_feature_get_all(m_camera, & features);
    feats->resize(VideoCamera::FEATURE_TYPE_MAX);

    for(int i = 0; i < DC1394_FEATURE_NUM; i++) {
      (*feats)[i] = fromNativeFeature(features.feature[i]);
    }
  }

  bool VideoCamera1394::enableTrigger(TriggerSource src)
  {
    assert(m_camera != 0);

    dc1394trigger_source_t source = dc1394trigger_source_t(DC1394_TRIGGER_SOURCE_0 + src);

    if(dc1394_external_trigger_set_power(m_camera, DC1394_ON)
      != DC1394_SUCCESS) {
      Radiant::error("VideoCamera1394::enableTrigger # Could not turn trigger power on");
      return false;
    }

    if(dc1394_external_trigger_set_source(m_camera, source)
      != DC1394_SUCCESS) {
      Radiant::error("VideoCamera1394::enableTrigger # Could not set trigger source");
      return false;
    }

    return true;
  }

  bool VideoCamera1394::setTriggerMode(TriggerMode tm)
  {
    assert(m_camera != 0);

    dc1394trigger_mode_t mode = dc1394trigger_mode_t(DC1394_TRIGGER_MODE_0 + tm);

    if(dc1394_external_trigger_set_mode(m_camera, mode)
      != DC1394_SUCCESS) {
      error("VideoCamera1394::setTriggerMode # Could not set trigger mode");
      return false;
    }

    return true;
  }

  bool VideoCamera1394::setTriggerPolarity(TriggerPolarity tp)
  {
    debugRadiant("VideoCamera1394::setTriggerPolarity # %d", (int) tp);

    dc1394trigger_polarity_t polarity =
        (tp == TRIGGER_ACTIVE_HIGH) ? DC1394_TRIGGER_ACTIVE_HIGH :
        DC1394_TRIGGER_ACTIVE_LOW;

    dc1394error_t e = dc1394_external_trigger_set_polarity(m_camera, polarity);

    if(e != DC1394_SUCCESS) {
      error("VideoCamera1394::setTriggerPolarity # %s", dc1394_error_get_string(e));
      return false;
    }

    return true;
  }


  bool VideoCamera1394::disableTrigger()
  {
    assert(m_camera != 0);

    return dc1394_external_trigger_set_power(m_camera, DC1394_OFF) ==
        DC1394_SUCCESS;
  }

  void VideoCamera1394::sendSoftwareTrigger()
  {
    assert(m_camera != 0);
    dc1394_software_trigger_set_power(m_camera, DC1394_ON);
  }

  static Mutex g_mutex;

  bool VideoCamera1394::open(uint64_t euid,
                             int width,
                             int height,
                             ImageFormat fmt,
                             FrameRate framerate)
  {
    // Only one thread at a time, just to make things sure.
    Guard guard(g_mutex);

    /* On some systems, sleep is needed for proper multi-camera operation. Sigh.*/
    Radiant::Sleep::sleepMs(s_openDelay);

    QString videodevice("/dev/video1394");

    uint32_t i;

    const char * fname = "VideoCamera1394::initialize";

    if(!findCamera(euid)) {
      error("%s # Could not find FireWire camera %" PRIx64, fname, euid);
      return false;
    }

    dc1394video_mode_t video_modes[] = {
      difmt2dcfmt(fmt, width, height),
      DC1394_VIDEO_MODE_1024x768_MONO8,
      DC1394_VIDEO_MODE_640x480_MONO8,
      DC1394_VIDEO_MODE_640x480_YUV411,
      (dc1394video_mode_t) 0
    };

    dc1394video_mode_t video_mode;

    video_mode = DC1394_VIDEO_MODE_640x480_MONO8;

    dc1394framerates_t framerates;
    framerates.num = 0;

    for(i = 0; video_modes[i] != 0; i++) {
      video_mode = video_modes[i];
      if (dc1394_video_get_supported_framerates(m_camera,
                                                video_mode, &framerates)
        != DC1394_SUCCESS) {
        Radiant::error("%s # dc1394_video_get_supported_framerates",
                       fname);
      }
      if(framerates.num != 0)
        break;
    }

    assert(framerates.num);

    int targetfps = diFPS2dcFPS(framerate);
    dc1394framerate_t fps = (dc1394framerate_t) targetfps;

    for(i = 0; i < framerates.num; i++) {
      fps = framerates.framerates[i];
      if(fps == targetfps)
        break;
    }

    debugRadiant("%s # The video mode id = %d", fname, (int) video_mode);
    debugRadiant("%s # The frame rate id = %d (target = %d)",
          fname, (int) fps, targetfps);

    if(dc1394_video_set_mode(m_camera, video_mode)
      != DC1394_SUCCESS) {
      error("%s # dc1394_video_set_mode failed",
            fname);
      return false;
    }

    if(dc1394_video_set_framerate(m_camera, fps) != DC1394_SUCCESS) {
      error("%s # dc1394_video_set_framerate failed",
            fname);
      return false;
    }

    // If the camera is already running (eg. unclean exit), stop it
    dc1394switch_t isoWasOn;
    if(dc1394_video_get_transmission(m_camera, &isoWasOn) != DC1394_SUCCESS)
      Radiant::error("%s # dc1394_video_get_transmission failed", fname);

    if(isoWasOn == DC1394_ON) {
      debugRadiant("%s # Camera is already running, stopping it", fname);

      if(dc1394_video_set_transmission(m_camera, DC1394_OFF) !=DC1394_SUCCESS)
        Radiant::error("%s # dc1394_video_set_transmission failed", fname);
    }

    if(!captureSetup(NUM_BUFFERS))
      return false;

    m_initialized = true;
    m_started = false;

    // set frame size
    if(video_mode == DC1394_VIDEO_MODE_640x480_YUV411) {
      m_image.m_format = IMAGE_YUV_411;
      m_image.m_planes[0].m_type = PLANE_YUV;
      m_image.m_planes[0].m_linesize = 640 + 640 / 2;
      m_image.m_width  = 640;
      m_image.m_height = 480;
    }
    else if(video_mode == DC1394_VIDEO_MODE_640x480_MONO8) {

      if(fmt == IMAGE_RAWBAYER)
        m_image.m_format = IMAGE_RAWBAYER;
      else
        m_image.m_format = IMAGE_GRAYSCALE;

      m_image.m_planes[0].m_type = PLANE_GRAYSCALE;
      m_image.m_planes[0].m_linesize = 640;
      m_image.m_width  = 640;
      m_image.m_height = 480;
    }
    else if(video_mode == DC1394_VIDEO_MODE_1024x768_MONO8) {

      if(fmt == IMAGE_RAWBAYER)
        m_image.m_format = IMAGE_RAWBAYER;
      else
        m_image.m_format = IMAGE_GRAYSCALE;

      m_image.m_format = IMAGE_GRAYSCALE;
      m_image.m_planes[0].m_type = PLANE_GRAYSCALE;
      m_image.m_planes[0].m_linesize = 1024;
      m_image.m_width  = 1024;
      m_image.m_height = 768;
    }
    else {
      m_initialized = false;
      Radiant::error("%s # unsupported image format", fname);
    }

    debugRadiant("%s # EXIT OK with difmt = %d", fname, (int) m_image.m_format);

    return true;
  }

  bool VideoCamera1394::openFormat7(uint64_t cameraeuid,
                                    Nimble::Recti roi,
                                    float fps,
                                    int mode)
  {
    Guard guard(g_mutex);

    Radiant::Sleep::sleepMs(s_openDelay);

    const char * fname = "VideoCamera1394::openFormat7";

    if(!findCamera(cameraeuid))
      return false;

    int err;
    unsigned minbytes, maxbytes;

    dc1394video_mode_t vmode = (dc1394video_mode_t)
                               (DC1394_VIDEO_MODE_FORMAT7_0 + mode);

    err = dc1394_video_set_mode(m_camera, vmode);
    if(err != DC1394_SUCCESS) {
      Radiant::error("%s # Could not set mode to format7_0", fname);
      return false;
    }

    dc1394format7modeset_t modeset;

    memset( & modeset, 0, sizeof(modeset));

    err = dc1394_format7_get_modeset(m_camera, & modeset);

    uint32_t maxw = 0;
    uint32_t maxh = 0;

    err = dc1394_format7_get_max_image_size
          (m_camera, vmode, & maxw, & maxh);

    debugRadiant("%s # fps = %f", fname, fps);
    debugRadiant("%s # Maximum image size = %d x %d", fname, (int) maxw, (int) maxh);

    if(roi.high().x > (int) maxw)
      roi.high().x = maxw;

    if(roi.high().y > (int) maxh)
      roi.high().y = maxh;

    // Make the image size multiple of four...
    for(int i = 0; i < 2; i++) {
      while(roi.high()[i] & 0x3)
        roi.high()[i]--;
    }

    err = dc1394_format7_get_packet_parameters
          (m_camera, vmode, & minbytes, & maxbytes);

    if(err != DC1394_SUCCESS) {
      Radiant::error("%s # Could not get packet parameters", fname);
      return false;
    }

    /* Tricky to get the frame-rate right:

       http://damien.douxchamps.net/ieee1394/libdc1394/v2.x/faq/#How_can_I_work_out_the_packet_size_for_a_wanted_frame_rate
    */

    float busPeriod; // Bus period in seconds

    if(m_speed == DC1394_ISO_SPEED_400)
      busPeriod = 0.000125f;
    else if(m_speed == DC1394_ISO_SPEED_800)
      busPeriod = 0.0000625f;
    else {
      Radiant::error("%s # Cannot calculate bus speed as the speed (%d) is unknown",
                     fname, (int) m_speed);
      return false;
    }

    int numPackets = (int) (1.0f / (busPeriod * fps));
    int denom = numPackets * 8;

    int packetSize = 2.01 * (roi.area() * 8 + denom - 1) / denom;

    if(packetSize > (int) maxbytes) {

      debugRadiant("%s # Limiting packet size to %u", fname, maxbytes);
      packetSize = maxbytes;
    }

    dc1394_format7_set_color_coding(m_camera,
                                    vmode,
                                    DC1394_COLOR_CODING_MONO8);

    err = dc1394_format7_set_roi(m_camera,
                                 vmode,
                                 DC1394_COLOR_CODING_MONO8,
                                 packetSize,
                                 roi.low().x, roi.low().y,
                                 roi.width(), roi.height());

    if(err != DC1394_SUCCESS) {
      Radiant::error("%s # Could not set ROI", fname);
      return false;
    }

    if(!captureSetup(NUM_BUFFERS))
      return false;

    // Here we only support grayscale for the time being...
    m_image.m_format = IMAGE_GRAYSCALE;
    m_image.m_planes[0].m_type = PLANE_GRAYSCALE;
    m_image.m_planes[0].m_linesize = roi.width();
    m_image.m_width  = roi.width();
    m_image.m_height = roi.height();

    debugRadiant("%s # initialized format-7 mode with resolution %d x %d",
          fname, m_image.m_width, m_image.m_height);

    m_initialized = true;

    return true;
  }

  bool VideoCamera1394::isInitialized() const
  {
    return m_initialized;
  }


  int VideoCamera1394::width() const
  {
    return m_image.m_width;
  }


  int VideoCamera1394::height() const
  {
    return m_image.m_height;
  }


  float VideoCamera1394::fps() const
  {
    return m_fps;
  }

  bool VideoCamera1394::start()
  {
    assert(isInitialized());

    if(m_started)
      return true;

    if (dc1394_video_set_transmission(m_camera, DC1394_ON) != DC1394_SUCCESS) {
      Radiant::error("VideoCamera1394::start # unable to start camera iso transmission");

      return false;
    }
    else {
      dc1394switch_t trans;
      dc1394_video_get_transmission(m_camera, & trans);
      debugRadiant("VideoCamera1394::start # %d", (int) trans);
    }

    m_started = true;

    return true;
  }

  bool VideoCamera1394::stop()
  {
    m_started = false;

    if(!m_initialized) {
      Radiant::error("VideoCamera1394::stop # camera has not been initialized");
      return false;
    }

    if(dc1394_capture_stop(m_camera) != DC1394_SUCCESS) {
      Radiant::error("VideoCamera1394::stop # unable to stop capture");
    }

    if (dc1394_video_set_transmission(m_camera, DC1394_OFF) !=DC1394_SUCCESS) {
      Radiant::error("VideoCamera1394::stop # unable to stop iso transmission");
    }

    return true;
  }

  const VideoImage * VideoCamera1394::captureImage()
  {
    assert(isInitialized());

    if(!m_started && !start())
      return 0;

    m_frame = 0;

    if(m_timeoutUs > 0) {

      int fd = dc1394_capture_get_fileno(m_camera);
      if(fd == -1) {
        Radiant::error("VideoCamera1394::captureImage # dc1394_capture_get_fileno failed");
        return 0;
      }
      fd_set fds;
      struct timeval tv;

      FD_ZERO(& fds);
      FD_SET(fd, & fds);

      tv.tv_sec = m_timeoutUs / 1000000;
      tv.tv_usec = m_timeoutUs % 1000000;

      select(fd + 1, &fds, 0, 0, &tv);

      if(FD_ISSET(fd, & fds))
        ;
      else {
        FD_ZERO( & fds);
        return 0;
      }
      FD_ZERO( & fds);
    }

    int err = dc1394_capture_dequeue(m_camera,
                                     DC1394_CAPTURE_POLICY_WAIT, & m_frame);

    if(err ) {
      Radiant::error("VideoCamera1394::captureImage # Unable to capture a frame!");
      close();
      return 0;
    }

#ifndef RADIANT_OSX
    if(dc1394_capture_is_frame_corrupt(m_camera, m_frame) == DC1394_TRUE) {
      Radiant::error("VideoCamera1394::captureImage # Got corrupted frame");
      doneImage();
      return 0;
    }
#endif

    if(!m_frame)
      return 0;

    m_image.m_planes[0].m_data = (uchar *) m_frame->image;

    m_framesBehind = m_frame->frames_behind;

    m_outside++;

    if(m_outside != 1) {
      Radiant::error("VideoCamera1394::captureImage # Please release captured "
                     "frames with doneImage()");
    }

    return & m_image;
  }

  void VideoCamera1394::doneImage()
  {
    m_outside--;

    assert(m_outside == 0);

    dc1394_capture_enqueue(m_camera, m_frame);

    m_frame = 0;
  }

  bool VideoCamera1394::close()
  {
    if(!m_camera)
      return false;

    if (m_started)
      stop();

    {
      Radiant::Guard g(s_infosMutex);
      s_infos.erase(std::remove(s_infos.begin(), s_infos.end(), m_camera), s_infos.end());
    }

    if(m_camera) {
      dc1394_camera_free(m_camera);
    }

    m_initialized = false;
    m_camera = 0;

    Radiant::Guard g2(s_takenMutex);
    std::set<int>::iterator it = s_taken.find(m_cameraNum);
    if(it == s_taken.end())
      error("VideoCamera1394::close # taken mismatch %d", (int) m_cameraNum);
    else
      s_taken.erase(it);

    return true;
  }

  uint64_t VideoCamera1394::uid()
  {
    if(!m_camera)
      return 0;

    return m_camera->guid;
  }


  VideoCamera1394::CameraInfo VideoCamera1394::cameraInfo()
  {
    CameraInfo info;

    if(m_camera) {
      info.m_euid64 = m_camera->guid;
      info.m_vendor = m_camera->vendor;
      info.m_model  = m_camera->model;
    }

    return info;
  }

  void VideoCamera1394::busReset()
  {
    info("Performing FireWire bus reset");

    CameraDriver1394 driver;

    std::vector<VideoCamera::CameraInfo> tmp;
    driver.queryCameras(tmp);

    Radiant::Guard g(s_infosMutex);
    for(int c = 0; c < (int) s_infos.size(); c++) {
      dc1394_reset_bus(s_infos[c]);
      Sleep::sleepMs(100);
    }
  }

  bool VideoCamera1394::findCamera(uint64_t euid)
  {
    const char * fname = "VideoCamera1394::findCamera";

    uint32_t i;

    if(m_camera)
      close();

    m_euid = euid ? euid : m_euid;

    // if(euid != 0)
    debugRadiant("VideoCamera1394::findCamera # m_euid = %.8x%.8x", (int) (m_euid >> 32), (int) m_euid);

    if (m_initialized)
      close();

    {
      std::vector<CameraInfo> cameras;
      if(driver()->queryCameras(cameras) == 0) return false;
    }

    {
      Radiant::Guard g(s_infosMutex);
      if(!s_infos.size()) {
        Radiant::error("%s # No FireWire cameras found", fname);
        return false;
      }
    }

#ifdef RADIANT_OSX
    debugRadiant("%s # Running OS X, no FireWire bus reset", fname);
#else
    // Clean up in the first start:
    MULTI_ONCE {
     Radiant::Guard g(s_infosMutex);
     for(int c = 0; c < (int) s_infos.size(); c++) {
      dc1394_reset_bus(s_infos[c]); // no resetting bus for OSX
      Sleep::sleepMs(100);
     }
    }
#endif

    // Now seek the camera we are interested in:
    bool foundCorrect = false;

    Radiant::Guard g(s_infosMutex);
    {
      for(i = 0; i < s_infos.size() && m_euid != 0; i++) {
        if(s_infos[i]->guid == m_euid) {
          m_cameraNum = (int) i;
          foundCorrect = true;
          debugRadiant("%s # Got camera %d based on euid", fname, (int) i);
          break;
        }
      }

      if(m_euid != 0 && !foundCorrect) {
        debugRadiant("%s # Could not find the camera with euid = %llx",
              fname, (long long) m_euid);
        return false;
      }

      Radiant::Guard g2(s_takenMutex);
      if(s_taken.find(m_cameraNum) != s_taken.end()) {
        error("%s # Camera index %d is already taken (firewire id = %llx)", fname, (int) m_cameraNum, (long long)m_euid);
        //return false;
      }

      s_taken.insert(m_cameraNum);

      assert(m_cameraNum < (int) s_infos.size());

      m_camera = s_infos[m_cameraNum];
    }

    debugRadiant("%s # Initializing camera %s \"%s\"",
          fname, m_camera->vendor, m_camera->model);

    if(dc1394_feature_get_all(m_camera, & m_features)
      != DC1394_SUCCESS) {
      debugRadiant("%s # unable to get feature set %d",
            fname, m_cameraNum);
    }

    bool try1394b = true;

    if(getenv("NO_FW800") != 0)
      try1394b = false;

    if(!m_camera->bmode_capable)
      try1394b = false;

    if(strstr(m_camera->model, "Firefly") &&
       strstr(m_camera->vendor, "Point Grey")) {
      /* PTGrey Firefly is a popular camera, but it apparently reports
   itself as FW800 camera... */

      debugRadiant("PTGrey Firefly camera detected, going for FW400");
      try1394b = false;
    }

    debugRadiant("%s # Try %s FW800", fname, try1394b ? "with" : "without");

    if(try1394b) {
      bool is1394b = false;

      if(dc1394_video_set_operation_mode(m_camera, DC1394_OPERATION_MODE_1394B)
        != DC1394_SUCCESS) {
        dc1394_video_set_operation_mode(m_camera, DC1394_OPERATION_MODE_LEGACY);
        debugRadiant("%s # Could not set operation mode to 1394B", fname);
      }
      else
        is1394b = true;

      info("%s # is1394b = %d", fname, (int) is1394b);

      if(is1394b) {
        if(dc1394_video_set_iso_speed(m_camera, DC1394_ISO_SPEED_800)
          != DC1394_SUCCESS) {

          debugRadiant("%s # Could not set ISO speed to 800", fname);

          if(dc1394_video_set_iso_speed(m_camera, DC1394_ISO_SPEED_400)
            != DC1394_SUCCESS) {
            error("%s # dc1394_video_set_iso_speed 400 failed",
                  fname);
            return false;
          }
        }
      }
    } else if(dc1394_video_set_iso_speed(m_camera, DC1394_ISO_SPEED_400)
        != DC1394_SUCCESS) {
      error("%s # dc1394_video_set_iso_speed 400 failed", fname);
      return false;
    }

    if (dc1394_video_get_iso_speed(m_camera, &m_speed) != DC1394_SUCCESS) {
      error("%s # dc1394_video_get_iso_speed failed", fname);
      return false;
    } else {
      int speedbits = 0;
      if(m_speed == DC1394_ISO_SPEED_100)
        speedbits = 100;
      else if(m_speed == DC1394_ISO_SPEED_200)
        speedbits = 200;
      else if(m_speed == DC1394_ISO_SPEED_400)
        speedbits = 400;
      else if(m_speed == DC1394_ISO_SPEED_800)
        speedbits = 800;
      debugRadiant("%s # ISO speed = %d Mbits per second", fname, speedbits);
    }
    return true;
  }

  bool VideoCamera1394::captureSetup(int buffers)
  {
    int flags = DC1394_CAPTURE_FLAGS_DEFAULT;

#ifdef __linux__
    if(getenv("WITHOUT_1394_BANDWIDTH_ALLOC")) {
      flags = DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC;
      debugRadiant("VideoCamera1394::captureSetup # Ignoring bandwidth allocation");
    }
#endif
    dc1394error_t res = dc1394_capture_setup(m_camera, buffers, flags);
    if(res != DC1394_SUCCESS) {

      Radiant::error("VideoCamera1394::captureSetup # "
                     "unable to setup camera- check that the video mode,"
                     "framerate and format are supported (%s)",
                     dc1394_error_get_string(res));
      return false;
    }
    return true;
  }

  bool VideoCamera1394::setCaptureTimeout(int ms)
  {
    m_timeoutUs = 1000 * ms;

    return true;
  }

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  CameraDriver1394::~CameraDriver1394()
  {}

  size_t CameraDriver1394::queryCameras(std::vector<VideoCamera::CameraInfo> & cameras)
  {
    Guard guard(s_queryCamerasMutex);

    const char * fname = "CameraDriver1394::queryCameras";

    uint32_t i, j;
    dc1394error_t err;
    dc1394camera_list_t * camlist = 0;

    initDc();

    if(!s_dc) {
      Radiant::error("%s::queryCameras # failed to initialize libdc1394. Make sure you have permissions to access FireWire devices.", fname);
    }

    err = dc1394_camera_enumerate(s_dc, & camlist);

    if(err != DC1394_SUCCESS || camlist->num == 0) {
#ifdef __linux__
      const char * username = getenv("USERNAME");

      // try to check that files are existing, readable and writeable
      bool rawOk = access("/dev/raw1394", F_OK | R_OK | W_OK) == 0;
      bool videoOk = access("/dev/video1394/0", F_OK | R_OK | W_OK) == 0;
      if(!username)
        username = "username";

      // this happens with permission problems in older libdc1394 versions?
      if (err != DC1394_SUCCESS) {
        error("%s # dc1394_camera_enumerate failed (%s)\n"
              "*************************************************************\n"
              "Please check that FireWire device files exist:\n"
              "/dev/raw1394\n"
              "/dev/video1394 (or /dev/video1394/0 etc)\n"
              "And that you have permissions to use them.\n"
              "\n"
              "To gain permissions, try the following commands:\n\n"
              "> sudo addgroup %s video\n"
              "> sudo addgroup %s disk\n\n"
              "> sudo chmod -R 777 /dev/*1394*\n\n"
              "You may need to log in again for the changes to be effective.\n\n"
              "See also: http://www.multitouch.fi/cornerstone/cornerstone-documentation/firewire-permissions\n"
              "*************************************************************\n\n",
              fname, dc1394_error_get_string(err), username, username);
      } else {
        // no cameras found
        std::string missing;
        if (!rawOk || !videoOk) {
          missing = "Permission problems:\n";
          if (!rawOk)
            missing += "/dev/raw1394 read or write permission missing\n";
          if (!videoOk)
            missing += "/dev/video1394/0 read or write permission missing\n";
        }

        error("%s # Could not find any cameras\n"
              "*************************************************************\n"
              "%s"
              "\n"
              "To gain permissions, try the following commands:\n\n"
              "> sudo addgroup %s video\n"
              "> sudo addgroup %s disk\n\n"
              "> sudo chmod -R 777 /dev/*1394*\n\n"
              "You may need to log in again for the changes to be effective.\n\n"
              "See also: http://www.multitouch.fi/cornerstone/cornerstone-documentation/firewire-permissions\n"
              "*************************************************************\n\n",
              fname,
              missing.c_str(),
              username, username);
      }

#else
      Radiant::error("%s # dc1394_find_cameras failed (%s)\n",
                     fname, dc1394_error_get_string(err));
#endif
      return false;
    }

    debugRadiant("%s::Getting %d FireWire cameras", fname, (int) camlist->num);

    Radiant::Guard g(s_infosMutex);

    for(i = 0; i < camlist->num; i++) {
      bool already = false;

      for(j = 0; j < s_infos.size(); j++)
        if(s_infos[j]->guid == camlist->ids[i].guid)
          already = true;

      if(!already) {
        dc1394camera_t * cam = dc1394_camera_new(s_dc, camlist->ids[i].guid);
        if(!cam) {
          Radiant::error("CameraDriver1394::queryCameras # dc1394_camera_new failed for %" PRIx64,
                         camlist->ids[i].guid);
        } else {
          s_infos.push_back(cam);
        }
      }
    }

    debugRadiant("Copying FireWire camera #%d information to user", (int) camlist->num);

    for(i = 0; i < s_infos.size(); i++) {
      dc1394camera_t * c = s_infos[i];
      VideoCamera::CameraInfo ci;

      if(!c) {
        error("NULL camera");
        continue;
      }

      if(!c->guid || !c->vendor || !c->model)
        continue;

      debugRadiant("Got camera %p: %s %s (%" PRIx64")", c, c->vendor, c->model, c->guid);

      ci.m_euid64 = c->guid;
      ci.m_vendor = c->vendor;
      ci.m_model  = c->model;
      ci.m_driver = QString("libdc1394");

      cameras.push_back(ci);
    }

    debugRadiant("Clearing camera list");

    dc1394_camera_free_list(camlist);

    return true;
  }

  VideoCamera * CameraDriver1394::createCamera()
  {
    return new VideoCamera1394(this);
  }

}

#endif
