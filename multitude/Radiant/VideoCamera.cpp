#include "VideoCamera.hpp"
#include "CameraDriver.hpp"

#include <Radiant/Trace.hpp>
#include <Radiant/StringUtils.hpp>

namespace Radiant
{

  VideoCamera::VideoCamera(CameraDriver * driver)
      : VideoInput(),
      m_driver(driver)
  {}

  VideoCamera::~VideoCamera()
  {}

  void VideoCamera::setPan(float value)
  {
    setFeature(PAN, value);
  }

  void VideoCamera::setTilt(float value)
  {
    setFeature(TILT, value);
  }

  void VideoCamera::setGamma(float value)
  {
    setFeature(GAMMA, value);
  }

  void VideoCamera::setShutter(float value)
  {
    setFeature(SHUTTER, value);
  }

  void VideoCamera::setGain(float value)
  {
    setFeature(GAIN, value);
  }

  void VideoCamera::setExposure(float value)
  {
    setFeature(EXPOSURE, value);
  }

  void VideoCamera::setBrightness(float value)
  {
    setFeature(BRIGHTNESS, value);
  }

  void VideoCamera::setFocus(float value)
  {
    setFeature(FOCUS, value);
  }

  const char * VideoCamera::featureName(FeatureType id)
  {
    static const char * names [] = {
      "brightness",
      "exposure",
      "sharpness",
      "white-balance",
      "hue",
      "saturation",
      "gamma",
      "shutter",
      "gain",
      "iris",
      "focus",
      "temperature",
      "trigger",
      "trigger delay",
      "white shading",
      "frame rate",
      "zoom",
      "pan",
      "tilt",
      "optical filter",
      "capture size",
      "capure quality"
    };

    int index = id;

    return names[index];
  }

  bool VideoCamera::hasMode(const CameraFeature & feature,
                            FeatureMode mode)
  {
    for(uint32_t i = 0; i < feature.num_modes; i++)
      if(feature.modes[i] == mode)
        return true;

    return false;
  }

  CameraDriverFactory g_factory;

  CameraDriverFactory & VideoCamera::drivers()
  {
    return g_factory;
  }
}
