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

#ifndef RADIANT_VIDEO_CAMERA_1394_HPP
#define RADIANT_VIDEO_CAMERA_1394_HPP

#include <Radiant/VideoCamera.hpp>
#include <Radiant/CameraDriver.hpp>

#include <Nimble/Rect.hpp>

#include <string>
#include <stdint.h>
#include <dc1394/control.h>

namespace Radiant {

  /// FireWire video capture
  /** This class works on top of the libdc1394 library.

      For more information on libdc1394, see:
      http://damien.douxchamps.net/ieee1394/libdc1394/ */
  class VideoCamera1394 : public VideoCamera
  {
  public:
    VideoCamera1394(CameraDriver * driver);
    virtual ~VideoCamera1394();

    virtual const Radiant::VideoImage * captureImage();
    virtual void doneImage();

    /// The width of the video stream images.
    virtual int width() const;
    /// The height of the video stream images.
    virtual int height() const;
    /// The frame rate of the video stream.
    virtual float fps() const;
    /// Native image format of the stream.
    virtual ImageFormat imageFormat() const;
    virtual unsigned int size() const;



     /** Sets a given feature to relative value, in range 0-1.*/
    virtual void setFeature(VideoCamera::FeatureType feature, float value);
    /** Sets a given feature to an absolute integer value.

  The minimum/maximum range of the value depends on the
  parameter, and the camera model.
     */
    virtual void setFeatureRaw(VideoCamera::FeatureType feature, int32_t value);
    virtual void getFeatures(std::vector<CameraFeature> * features);

    virtual void setWhiteBalance(float u_to_blue, float v_to_red);
    virtual bool setCaptureTimeout(int ms);

    /// Sets the triggering mode for the camera
    virtual bool enableTrigger(TriggerSource src);
    virtual bool setTriggerMode(TriggerMode mode);
    virtual bool setTriggerPolarity(TriggerPolarity polarity);
    virtual bool disableTrigger();
    /// Sends a software trigger signal to the camera
    virtual void sendSoftwareTrigger();

    /// Initializes the FireWire camera
    /** Some of the arguments are frankly ignored at the moment, for
  the example the device name (which is selected
  automatically). */
    virtual bool open(uint64_t euid, int width, int height, ImageFormat fmt = IMAGE_UNKNOWN, FrameRate framerate = FPS_IGNORE);

    /// Initializes the FireWire camera to format 7 mode
    virtual bool openFormat7(uint64_t cameraeuid, Nimble::Recti roi, float fps, int mode);

    virtual bool isInitialized() const;
    virtual bool start();
    virtual bool stop();
    virtual bool close();

    /// Sets the camera EUID that will be used to select the camera
    void setCameraEuid64(uint64_t euid) { m_euid = euid; }

    /// Returns information about this particular camera object
    virtual CameraInfo cameraInfo();

    dc1394camera_t * dc1394Camera() { return m_camera; }

    /** Returns the number of frames that would be immediately readable.

        This function is not implemented on all platforms, so use it
        with some care.
     */
    virtual int framesBehind() const { return m_framesBehind; }

  private:
    bool enableCameraFeature(unsigned int feature,
                             const std::string & description,
                             bool automatic_mode,
                             unsigned int * feature_min_value,
                             unsigned int * feature_max_value);

    bool findCamera(uint64_t euid);
    void captureSetup(int buffers);

    std::string    m_videodevice;

    /** camera capture information. */
    dc1394camera_t * m_camera;
    dc1394video_frame_t * m_frame;

    /** camera feature information. */
    dc1394featureset_t m_features;
    dc1394speed_t      m_speed; // FW400, FW800 etc.

    uint64_t m_euid;
    int m_cameraNum;

    int m_isoChannel;

    /// Number of images the user is holding.
    int m_outside;
    int m_framesBehind;

    bool  m_initialized;
    bool  m_started;
    unsigned  m_fps;

    VideoImage m_image;

    int m_timeoutUs;
  };

  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

  class CameraDriver1394 : public CameraDriver
  {
  public:
    CameraDriver1394() {}
    virtual ~CameraDriver1394();

    virtual size_t queryCameras(std::vector<VideoCamera::CameraInfo> & cameras);
    virtual VideoCamera * createCamera();
    virtual std::string driverName() const { return "libdc"; }
  };

}


#endif

