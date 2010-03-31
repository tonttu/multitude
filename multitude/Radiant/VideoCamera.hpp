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

#ifndef RADIANT_VIDEO_CAMERA_HPP
#define RADIANT_VIDEO_CAMERA_HPP

#include <Radiant/Export.hpp>
#include <Radiant/VideoInput.hpp>

#include <Nimble/Rect.hpp>

#include <stdint.h>

namespace Radiant {
  class CameraDriverFactory;
  class CameraDriver;

  /// VideoCamera provides a common interface for different video cameras. Each new camera driver should implement this interface and also implement the CameraDriver interface as well.
  class RADIANT_API VideoCamera : public VideoInput
  {
  public:
    VideoCamera(CameraDriver * driver);
    virtual ~VideoCamera();

    /// A container of basic camera information. CameraInfo objects
    /// are used to store information about a particular camera.
    class CameraInfo {
        public:
      /// The 64-bit unique FireWire identifier
      int64_t m_euid64;
      /// Vendor name, in a human-readable format
      std::string m_vendor;
      /// Camera model, in a human-readable format
      std::string m_model;
      /// Driver that was used for this camera
      std::string m_driver;
    };

    /// Camera feature modes
    enum FeatureMode {
      MODE_MANUAL = 0,
      MODE_AUTO,
      MODE_ONE_PUSH_AUTO,
      MODE_MAX
    };

    /// Camera feature types
    enum FeatureType {
      BRIGHTNESS = 0,
      EXPOSURE,
      SHARPNESS,
      WHITE_BALANCE,
      HUE,
      SATURATION,
      GAMMA,
      SHUTTER,
      GAIN,
      IRIS,
      FOCUS,
      TEMPERATURE,
      TRIGGER,
      TRIGGER_DELAY,
      WHITE_SHADING,
      FRAME_RATE,
      ZOOM,
      PAN,
      TILT,
      OPTICAL_FILTER,
      CAPTURE_SIZE,
      CAPTURE_QUALITY,
      FEATURE_TYPE_MAX
    };

    /// Camera external trigger sources
    enum TriggerSource {
      TRIGGER_SOURCE_0 = 0,
      TRIGGER_SOURCE_1,
      TRIGGER_SOURCE_2,
      TRIGGER_SOURCE_3,
      TRIGGER_SOURCE_SOFTWARE,
      TRIGGER_SOURCE_MAX
    };

    /// Camera external trigger modes
    enum TriggerMode {
      TRIGGER_MODE_0 = 0,
      TRIGGER_MODE_1,
      TRIGGER_MODE_2,
      TRIGGER_MODE_3,
      TRIGGER_MODE_4,
      TRIGGER_MODE_5,
      TRIGGER_MODE_14 = 14,
      TRIGGER_MODE_15 = 15,
      TRIGGER_MODE_MAX
    };

    /// Camera external trigger polarity
    enum TriggerPolarity {
      TRIGGER_ACTIVE_LOW = 0,
      TRIGGER_ACTIVE_HIGH
    };

    /// A container of basic camera feature information.
    struct CameraFeature {
      FeatureType id;
      uint32_t min;
      uint32_t max;
      uint32_t value;
      bool available;
      bool absolute_capable;
      bool readout_capable;
      bool on_off_capable;
      bool polarity_capable;
      bool is_on;

      FeatureMode current_mode;
      uint32_t num_modes;
      FeatureMode modes[MODE_MAX];

      float abs_value;
      float abs_min;
      float abs_max;
    };

    virtual bool open(uint64_t euid, int width, int height, ImageFormat fmt = IMAGE_UNKNOWN, FrameRate framerate = FPS_IGNORE) = 0;
    virtual bool openFormat7(uint64_t cameraeuid, Nimble::Recti roi, float fps, int mode) = 0;

    virtual void getFeatures(std::vector<CameraFeature> * features) = 0;
    virtual void setFeature(FeatureType id, float value) = 0;
    virtual void setFeatureRaw(FeatureType id, int32_t value) = 0;
    static const char * featureName(FeatureType id);

    static bool hasMode(const CameraFeature & feature, FeatureMode mode);
    static bool hasAutoMode(const CameraFeature & feature)
    { return hasMode(feature, MODE_AUTO); }
    static bool hasManualMode(const CameraFeature & feature)
    { return hasMode(feature, MODE_MANUAL); }


    virtual void setPan(float value);
    virtual void setTilt(float value);
    virtual void setGamma(float value);
    virtual void setShutter(float value);
    virtual void setGain(float value);
    virtual void setExposure(float value);
    virtual void setBrightness(float value);
    virtual void setFocus(float value);
    virtual bool setCaptureTimeout(int ms) = 0;
    virtual void setWhiteBalance(float u_to_blue, float v_to_red) = 0;

    /// Sets the triggering mode for the camera
    virtual bool enableTrigger(TriggerSource src) = 0;
    virtual bool setTriggerMode(TriggerMode mode) = 0;
    virtual bool setTriggerPolarity(TriggerPolarity polarity) = 0;
    virtual bool disableTrigger() = 0;
    /// Sends a software trigger signal to the camera
    virtual void sendSoftwareTrigger() = 0;

    /// Returns information about this particular camera object
    virtual CameraInfo cameraInfo() = 0;

    virtual int framesBehind() const = 0;

    static CameraDriverFactory & drivers();

    CameraDriver * driver() { return m_driver; }
    private:
    CameraDriver * m_driver;
  };

}

#endif
