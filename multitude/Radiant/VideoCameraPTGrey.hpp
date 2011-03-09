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

#ifndef RADIANT_VIDEOCAMERAPTGREY_HPP
#define RADIANT_VIDEOCAMERAPTGREY_HPP

/// @cond

#include <Radiant/Export.hpp>
#include <Radiant/VideoCamera.hpp>
#include <Radiant/CameraDriver.hpp>

#ifndef WIN32
#include <flycapture/FlyCapture2.h>
#else
#include <FlyCapture2.h>
#endif

namespace Radiant
{

    /** This is a low-level camera driver wrapper. You are not supposed to use this class directly.
        @sa VideoCamera */
  class RADIANT_API VideoCameraPTGrey : public VideoCamera
  {
  public:
    VideoCameraPTGrey(CameraDriver * driver);
    virtual ~VideoCameraPTGrey();

    virtual bool open(uint64_t euid, int width, int height, ImageFormat fmt = IMAGE_UNKNOWN, FrameRate framerate = FPS_IGNORE);
    virtual bool openFormat7(uint64_t cameraeuid, Nimble::Recti roi, float fps, int mode);

    virtual bool start();
    virtual bool stop();
    virtual bool close();
    virtual uint64_t uid();

    virtual const Radiant::VideoImage * captureImage();

    virtual CameraInfo cameraInfo();

    virtual int width() const;
    virtual int height() const;
    virtual float fps() const;
    virtual ImageFormat imageFormat() const;
    virtual unsigned int size() const;

    virtual void setFeature(FeatureType id, float value);
    virtual void setFeatureRaw(FeatureType id, int32_t value);
    virtual void getFeatures(std::vector<CameraFeature> * features);

    virtual void setWhiteBalance(float u_to_blue, float v_to_red);
    virtual bool setCaptureTimeout(int ms);

    virtual bool enableTrigger(TriggerSource src);
    virtual bool setTriggerMode(TriggerMode mode);
    virtual bool setTriggerPolarity(TriggerPolarity polarity);
    virtual bool disableTrigger();
    virtual void sendSoftwareTrigger();

    virtual int framesBehind() const { return 0; }

    static void useFakeFormat7(bool fake) { m_fakeFormat7 = fake; }

  private:

    enum State {
      UNINITIALIZED,
      OPENED,
      RUNNING
    };

    void queryFeature(FlyCapture2::PropertyType id,
              std::vector<VideoCamera::CameraFeature> * features);

    FlyCapture2::Camera m_camera;
    VideoImage m_image;

    Nimble::Recti m_format7Rect;
    static bool   m_fakeFormat7;

    int m_captureTimeoutMs;
    State m_state;
    CameraInfo m_info;
  };

  class CameraDriverPTGrey : public CameraDriver
  {
  public:
    CameraDriverPTGrey();

    virtual size_t queryCameras(std::vector<VideoCamera::CameraInfo> & cameras);
    virtual VideoCamera * createCamera();
    virtual QString driverName() const { return "ptgrey"; }
  };

}

/// @endcond

#endif
