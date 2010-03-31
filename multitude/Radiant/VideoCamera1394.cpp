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

#include "VideoCamera1394.hpp"

#include <Radiant/Trace.hpp>
#include <Radiant/Mutex.hpp>
#include <Radiant/Sleep.hpp>
#include <Radiant/Types.hpp>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <set>

#include <dc1394/camera.h>

#include <sys/utsname.h>

#define NUM_BUFFERS 10

namespace {

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

  dc1394trigger_source_t triggerSourceToNative(Radiant::VideoCamera::TriggerSource id)
  {
    dc1394trigger_source_t result = dc1394trigger_source_t(int(DC1394_TRIGGER_SOURCE_0) + int(id));

    return result;
  }

}

namespace Radiant {

  static std::vector<dc1394camera_t *> g_infos;

  static std::set<int> g_iso_channels;
  static std::set<int> g_taken;

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
    //    feat.num_modes = native.modes.num;

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

  static dc1394_t * g_dc = 0;
  static int g_count = 0;

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

    if(!g_dc)
      g_dc = dc1394_new();

    g_count++;
  }

  VideoCamera1394::~VideoCamera1394()
  {
    if (m_initialized)
      close();

    g_count--;

    /*if(!g_count) {
      dc1394_free(g_dc);
      g_dc = 0;
    }
    */
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

  /**
   * Try to adjust white balance feature for the camera.
   * @note NOTE: may fail, if, for example, the camera doesn't
   * support this feature.
   * @param u_to_blue manual U to blue ratio, setting either
   * one of these to below zero sets auto white balance mode.
   * @param v_to_red manual V to red ratio, setting either one
   * of these to below zero sets auto white balance mode. These values are
   * basicly device-dependent. You should test suitable configuration
   * with for example coriander software. Setting values out-of-range
   * gives a warning.
   */
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
        Radiant::debug("%s # no auto mode for white balance", fname);
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
        Radiant::debug("%s # no manual mode for white balance", fname);
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

  /** Set a camera feature (=control parameter) to some value. Possible
   * features can be found from dc1394_control.h. If the "value" is less
   * than zero, then the camera will be switched to automatic control of
   * the feature. */

  void VideoCamera1394::setFeature(VideoCamera::FeatureType id, float value)
  {
    const char * fname = "VideoCamera1394::setFeature1394";

    dc1394feature_t feature = featureTypeToNative(id);

    const char * name = dc1394_feature_get_string(feature);

    // trace2("%s # %s %f", fname, name, value);

    bool automatic = value < 0.0f;

    dc1394bool_t b = DC1394_FALSE;

    if(automatic) {

      has_auto_mode(m_camera, feature, & b);

      if(b)
        dc1394_feature_set_mode(m_camera, feature, DC1394_FEATURE_MODE_AUTO);
      else
        Radiant::debug("%s # no auto mode for %s", fname, name);
    }
    else {
      has_manual_mode(m_camera, feature, & b);

      if(b)
        dc1394_feature_set_mode(m_camera, feature, DC1394_FEATURE_MODE_MANUAL);
      else {
        Radiant::debug("%s # no manual mode for %s", fname, name);
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
    debug("VideoCamera1394::setTriggerPolarity # %d", (int) tp);

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

  static MutexStatic g_mutex;

  /**
   * Initialize this instance and open connnection to the device to be
   * controlled.
   */
  bool VideoCamera1394::open(uint64_t euid,
                             int width,
                             int height,
                             ImageFormat fmt,
                             FrameRate framerate)
  {
    // Only one thread at a time, just to make things sure.
    GuardStatic guard(&g_mutex);

    /* On some systems, sleep is needed for proper multi-camera operation. Sigh.*/
    Radiant::Sleep::sleepMs(850);

    std::string videodevice("/dev/video1394");

    uint32_t i;

    const char * fname = "VideoCamera1394::initialize";

    if(!findCamera(euid)) {
      error("%s # Could not find FireWire camera %s", fname, euid);
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

    // if(width >= 500)
    video_mode = DC1394_VIDEO_MODE_640x480_MONO8;
    // else
    // video_mode = DC1394_VIDEO_MODE_320x240_MONO8;

    dc1394framerates_t framerates;
    framerates.num = 0;

    for(i = 0; video_modes[i] != 0; i++) {
      video_mode = video_modes[i];
      if (dc1394_video_get_supported_framerates(m_camera,
                                                video_mode, &framerates)
        != DC1394_SUCCESS) {
        Radiant::error("%s # dc1394_video_get_supported_framerates",
                       fname);
        // cleanup_and_exit(m_camera);
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

    debug("%s # The video mode id = %d", fname, (int) video_mode);
    debug("%s # The frame rate id = %d (target = %d)",
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
      debug("%s # Camera is already running, stopping it", fname);

      if(dc1394_video_set_transmission(m_camera, DC1394_OFF) !=DC1394_SUCCESS)
        Radiant::error("%s # dc1394_video_set_transmission failed", fname);
    }

    captureSetup(NUM_BUFFERS);

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

    debug("%s # EXIT OK with difmt = %d", fname, (int) m_image.m_format);

    return true;
  }

  bool VideoCamera1394::openFormat7(uint64_t cameraeuid,
                                    Nimble::Recti roi,
                                    float fps,
                                    int mode)
  {
    GuardStatic guard(&g_mutex);

    Radiant::Sleep::sleepMs(850);

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

    bzero( & modeset, sizeof(modeset));

    err = dc1394_format7_get_modeset(m_camera, & modeset);

    uint32_t maxw = 0;
    uint32_t maxh = 0;

    err = dc1394_format7_get_max_image_size
          (m_camera, vmode, & maxw, & maxh);

    debug("%s # fps = %f", fname, fps);
    debug("%s # Maximum image size = %d x %d", fname, (int) maxw, (int) maxh);

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

      debug("%s # Limiting packet size to %u", fname, maxbytes);
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

    captureSetup(NUM_BUFFERS);

    // Here we only support grayscale for the time being...
    m_image.m_format = IMAGE_GRAYSCALE;
    m_image.m_planes[0].m_type = PLANE_GRAYSCALE;
    m_image.m_planes[0].m_linesize = roi.width();
    m_image.m_width  = roi.width();
    m_image.m_height = roi.height();

    debug("%s # initialized format-7 mode with resolution %d x %d",
          fname, m_image.m_width, m_image.m_height);

    m_initialized = true;

    return true;
  }
  /*
  bool VideoCamera1394::printFormat7Modes(const char * cameraeuid)
  {
    const char * fname = "VideoCamera1394::printFormat7Modes";

    if(!findCamera(cameraeuid))
      return false;

    int err;
    dc1394format7modeset_t modeset;

    bzero( & modeset, sizeof(modeset));

    err = dc1394_format7_get_modeset(m_camera, & modeset);

    if(err != DC1394_SUCCESS) {
      Radiant::error("%s # Could not get modeset", fname);
      close();
      return false;
    }

    CameraInfo ci(cameraInfo());

    info("Format 7 mode information for %s %s (id = %s)",
   ci.m_vendor.c_str(), ci.m_model.c_str(), cameraeuid);

    for(int i = 0; i < DC1394_VIDEO_MODE_FORMAT7_NUM; i++) {

      dc1394format7mode_t & mode = modeset.mode[i];

      if(!mode.present) {
  info(" Format7 mode %d not present", i);
  continue;
      }

      info(" Format7 mode %d:", i);
      info("  size    = [%d %d]\n"
     "  maxsize = [%d %d]\n"
     "  pos     = [%d %d]",
     mode.size_x, mode.size_y,
     mode.max_size_x, mode.max_size_y,
     mode.pos_x, mode.pos_y);
      info("  unitsize    = [%d %d]\n"
     "  unitpos     = [%d %d]",
     mode.unit_size_x, mode.unit_size_y,
     mode.unit_pos_x, mode.unit_pos_y);
      info("  pixum    = %d",
     mode.pixnum);
    }

    return close();
  }
*/
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

  /**
   * Starts the camera data transmission.
   */
  bool VideoCamera1394::start()
  {
    assert(isInitialized());

    if(m_started)
      return true;

    // assert(m_started == false);

    if (dc1394_video_set_transmission(m_camera, DC1394_ON) != DC1394_SUCCESS) {
      Radiant::error("VideoCamera1394::start # unable to start camera iso transmission");

      return false;
    }
    else {
      dc1394switch_t trans;
      dc1394_video_get_transmission(m_camera, & trans);
      debug("VideoCamera1394::start # %d", (int) trans);
    }

    m_started = true;

    return true;
  }


  /**
   * Starts the camera data transmission.
   */
  bool VideoCamera1394::stop()
  {
    assert(isInitialized());

    assert(m_started);

    dc1394_capture_stop(m_camera);
    if (dc1394_video_set_transmission(m_camera, DC1394_OFF) !=DC1394_SUCCESS) {
      Radiant::error("VideoCamera1394::stop # unable to stop iso transmission");
    }

    m_started = false;

    return true;
  }

  /**
   * Capture a camera frame.
   */
  const VideoImage * VideoCamera1394::captureImage()
  {
    // trace("VideoCamera1394::captureImage");

    assert(isInitialized());

    if (!m_started)
      start();

    m_frame = 0;

    if(m_timeoutUs > 0) {

      int fd = dc1394_capture_get_fileno(m_camera);
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
        // error("VideoCamera1394::captureImage # no image available fd = %d r = %d", fd, r);
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

    if(!m_frame)
      return 0;

    /* assert(m_frame->size[0] == (uint) m_image.m_width &&
       m_frame->size[1] == (uint) m_image.m_height); */

    m_image.m_planes[0].m_data = (uchar *) m_frame->image;

    m_framesBehind = m_frame->frames_behind;

    // trace("VideoCamera1394::captureImage # EXIT");

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

  /**
   * Shuts down the connection.
   */
  bool VideoCamera1394::close()
  {
    // assert(isInitialized());

    if(!m_camera)
      return false;

    if (m_started)
      stop();

    if(m_camera) {
      dc1394_camera_free(m_camera);
    }

    m_initialized = false;
    m_camera = 0;

    std::set<int>::iterator it = g_taken.find(m_cameraNum);
    if(it == g_taken.end())
      error("VideoCamera1394::close # taken mismatch %d", (int) m_cameraNum);
    else
      g_taken.erase(it);

    return true;
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

  bool VideoCamera1394::findCamera(uint64_t euid)
  {
    const char * fname = "VideoCamera1394::findCamera";

    uint32_t i;

    bzero( & m_camera, sizeof(m_camera));

    m_euid = euid ? euid : m_euid;

    // if(euid != 0)
    debug("VideoCamera1394::findCamera # m_euid = %.8x%.8x", (int) (m_euid >> 32), (int) m_euid);

    if (m_initialized)
      close();

    {
      std::vector<CameraInfo> cameras;
      if(driver()->queryCameras(cameras) == 0) return false;
    }

    if(!g_infos.size()) {
      Radiant::error("%s # No FireWire cameras found", fname);
      return false;
    }

    bool isleopard = false;

#ifndef WIN32
    struct utsname sn;
    uname(& sn);

    debug("%s # System: %s # %s # %s", fname, sn.sysname, sn.release, sn.version);
    isleopard = strcmp(sn.sysname, "Darwin") == 0 &&
                ((strncmp(sn.release, "9.", 2) == 0) ||
                 (strncmp(sn.release, "10.", 3) == 0));
#endif

    // Clean up in the first start:
    static int initcount = 0;
    if(!initcount) {

      if(isleopard)
        debug("%s # Running Leopard, no FireWire bus reset", fname);
      else {
        for(int c = 0; c < (int) g_infos.size(); c++) {
          dc1394_reset_bus(g_infos[c]);
          Sleep::sleepMs(100);
        }
      }
    }
    initcount++;

    // Now seek the camera we are interested in:

    bool foundCorrect = false;

    for(i = 0; i < g_infos.size() && m_euid != 0; i++) {
      if(g_infos[i]->guid == m_euid) {
        m_cameraNum = (int) i;
        foundCorrect = true;
        debug("%s # Got camera %d based on euid", fname, (int) i);
        break;
      }
    }

    if(m_euid != 0 && !foundCorrect) {
      debug("%s # Could not find the camera with euid = %llx",
            fname, (long long) m_euid);
      return false;
    }

    if(g_taken.find(m_cameraNum) != g_taken.end()) {
      error("%s # Camera index %d is already taken", fname, (int) m_cameraNum);
    }

    g_taken.insert(m_cameraNum);

    assert(m_cameraNum < (int) g_infos.size());

    m_camera = g_infos[m_cameraNum];

    debug("%s # Initializing camera %s \"%s\"",
          fname, m_camera->vendor, m_camera->model);

    /* int isochan = m_cameraNum + 2;
       if(dc1394_video_specify_iso_channel(m_camera, isochan) !=DC1394_SUCCESS){
       error(ERR_UNKNOWN, "%s # unable to set ISO channel to %d",
       fname, isochan);
       }
       */

    if(dc1394_feature_get_all(m_camera, & m_features)
      != DC1394_SUCCESS) {
      debug("%s # unable to get feature set %d",
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

      debug("PTGrey Firefly camera detected, going for FW400");
      try1394b = false;
    }

    debug("%s # Try %s FW800", fname, try1394b ? "with" : "without");

    if(try1394b) {
      bool is1394b = false;

      if(dc1394_video_set_operation_mode(m_camera, DC1394_OPERATION_MODE_1394B)
        != DC1394_SUCCESS) {
        dc1394_video_set_operation_mode(m_camera, DC1394_OPERATION_MODE_LEGACY);
        debug("%s # Could not set operation mode to 1394B", fname);
      }
      else
        is1394b = true;

      info("%s # is1394b = %d", fname, (int) is1394b);

      if(is1394b) {
        if(dc1394_video_set_iso_speed(m_camera, DC1394_ISO_SPEED_800)
          != DC1394_SUCCESS) {

          debug("%s # Could not set ISO speed to 800", fname);

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
      debug("%s # ISO speed = %d Mbits per second", fname, speedbits);
    }
    return true;
  }

  void VideoCamera1394::captureSetup(int buffers)
  {
    int flags = DC1394_CAPTURE_FLAGS_DEFAULT;

#ifdef __linux__
    if(getenv("WITHOUT_1394_BANDWIDTH_ALLOC")) {
      flags = DC1394_CAPTURE_FLAGS_CHANNEL_ALLOC;
      debug("VideoCamera1394::captureSetup # Ignoring bandwidth allocation");
    }
#endif
    dc1394error_t res = dc1394_capture_setup(m_camera, buffers, flags);
    if(res != DC1394_SUCCESS) {

      Radiant::error("VideoCamera1394::captureSetup # "
                     "unable to setup camera- check that the video mode,"
                     "framerate and format are supported (%s)",
                     dc1394_error_get_string(res));
    }

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
    static MutexStatic s_mutex;
    GuardStatic guard(&s_mutex);

    const char * fname = "CameraDriver1394::queryCameras";

    uint32_t i, j;
    dc1394error_t err;
    dc1394camera_list_t * camlist = 0;

    if(!g_dc)
      g_dc = dc1394_new();

    if(!g_dc) {
      Radiant::error("%s::queryCameras # failed to initialize libdc1394. Make sure you have permissions to access FireWire devices.", fname);
      return false;
    }

#ifndef __linux__
    // For OSX
    static bool first = true;

    if(!first) {
      goto fillquery;
    }

    first = false;
#endif

    if((err = dc1394_camera_enumerate(g_dc, & camlist))
      != DC1394_SUCCESS) {

      // if(err != DC1394_NO_CAMERA)

#ifdef __linux__
      const char * username = getenv("USERNAME");

      if(!username)
        username = "username";

      error("%s # dc1394_find_cameras failed (%s)\n"
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
#else
      Radiant::error("%s # dc1394_find_cameras failed (%s)\n",
                     fname, dc1394_error_get_string(err));
#endif
      return false;
    }

    debug("%s::Getting %d FireWire cameras", fname, (int) camlist->num);

    g_infos.clear();

    for(i = 0; i < camlist->num; i++) {
      bool already = false;

      for(j = 0; j < g_infos.size(); j++)
        if(g_infos[j]->guid == camlist->ids[i].guid)
          already = true;

      if(!already)
        g_infos.push_back(dc1394_camera_new(g_dc, camlist->ids[i].guid));
    }

    debug("Copying FireWire camera information to user", (int) camlist->num);

#ifndef __linux__
    fillquery:
#endif

    //query->clear();

    for(i = 0; i < g_infos.size(); i++) {
      dc1394camera_t * c = g_infos[i];
      VideoCamera::CameraInfo ci;

      if(!c) {
        error("NULL camera");
        continue;
      }

      if(!c->guid || !c->vendor || !c->model)
        continue;

      debug("Got camera %p: %s %s (%llx)", c, c->vendor, c->model, c->guid);

      ci.m_euid64 = c->guid;
      ci.m_vendor = c->vendor;
      ci.m_model  = c->model;
      ci.m_driver = std::string("libdc1394");

      cameras.push_back(ci);
    }

    debug("Clearing camera list");

#ifdef __linux__
    dc1394_camera_free_list(camlist);
#endif

    return true;
  }

  VideoCamera * CameraDriver1394::createCamera()
  {
    return new VideoCamera1394(this);
  }

}

