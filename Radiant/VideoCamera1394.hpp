/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RADIANT_VIDEO_CAMERA_1394_HPP
#define RADIANT_VIDEO_CAMERA_1394_HPP
/// @cond
#include <Radiant/VideoCamera.hpp>
#include <Radiant/CameraDriver.hpp>

#include <Nimble/Rect.hpp>

#include <QString>
#include <cstdint>
#include <dc1394/control.h>

namespace Radiant {

  /// FireWire video capture
  /** This class works on top of the libdc1394 library.

      For more information on libdc1394, see:
      http://damien.douxchamps.net/ieee1394/libdc1394/ */
  class RADIANT_API VideoCamera1394 : public VideoCamera
  {
  public:
    /// Construtor
    /// @param driver Driver for using the camera
    VideoCamera1394(CameraDriver * driver);
    /// Destrutor
    virtual ~VideoCamera1394();

    /// Captures the camera frame
    /// @return Pointer to the frame captured
    virtual const Radiant::VideoImage * captureImage();
    /// Marks current frame ready for reusage
    virtual void doneImage();

    /// The width of the video stream images.
    /// @return Width of the image in pixels
    virtual int width() const;
    /// The height of the video stream images.
    /// @return Height of the image in pixels
    virtual int height() const;
    /// The frame rate of the video stream.
    /// @param Frame rate of the video stream
    virtual float fps() const;
    /// Native image format of the stream.
    /// @return ImageFormat of the stream
    virtual ImageFormat imageFormat() const;
    /// Bytes used by single image
    /// @return How many bytes single video frame uses
    virtual unsigned int size() const;



    /// Sets a given feature (control paramater) to some value. Possible
    /// features can be found from dc1394_control.h. If the "value" is less
    /// than zero, then the camera will be switched to automatic control of
    /// the feature.
    /// @param feature Feature to set
    /// @param value Value for the feature
    virtual void setFeature(VideoCamera::FeatureType feature, float value);

    /// Sets a given feature to an absolute integer value.
    /// The minimum/maximum range of the value depends on the parameter, and the camera model.
    /// @param feature Feature to set
    /// @param value Value for the feature
    /// @sa setFeature
    virtual void setFeatureRaw(VideoCamera::FeatureType feature, int32_t value);
    /// Returns the list of camera features
    /// @param features[out] Vector to fill with features
    virtual void getFeatures(std::vector<CameraFeature> * features);

    /// Try to adjust white balance feature for the camera.
    /// The parameter values are
    /// basically device-dependent. You should test suitable configuration
    /// with for example coriander software. Setting values out-of-range
    /// gives a warning.
    /// @param u_to_blue Manual U to blue ratio, setting either
    /// one of these to below zero sets auto white balance mode.
    /// @param v_to_red Manual V to red ratio, setting either one
    /// of these to below zero sets auto white balance mode.
    /// @note This may fail, if, for example, the camera doesn't support this feature.
    virtual void setWhiteBalance(float u_to_blue, float v_to_red);
    /// Set how long @ref captureImage waits for capturing of the image
    /// @return True if the operation was succesful
    virtual bool setCaptureTimeout(int ms);

    /// Sets the external trigger source for the camera
    /// @param src Source to set
    /// @return True if succeeded, false otherwise
    virtual bool enableTrigger(TriggerSource src);
    /// Sets the external trigger mode for the camera
    /// @param mode Mode to set
    /// @return True if succeeded, false otherwise
    virtual bool setTriggerMode(TriggerMode mode);
    /// Sets the polarity of external trigger
    /// @param polarity Polarity to set
    /// @return True if succeeded, false otherwise
    virtual bool setTriggerPolarity(TriggerPolarity polarity);
    /// Disables external trigger for the camera
    /// @return True is succeeded, false otherwise
    virtual bool disableTrigger();
    /// Sends a software trigger signal to the camera
    virtual void sendSoftwareTrigger();

    /// Initializes the FireWire camera and opens connection to the device to be controlled
    /// @param euid User id of the camera to open
    /// @param width Width of the requested image
    /// @param height Height of the requested image
    /// @param fmt Format of the requested image
    /// @param framerate Requested framerate
    virtual bool open(uint64_t euid, int width, int height, ImageFormat fmt = IMAGE_UNKNOWN, FrameRate framerate = FPS_IGNORE);

    /// @param cameraeuid User id of the camera to open
    /// @param roi Size of the requested image
    /// @param fps Requested framerate
    /// @param fmt mode Reqested video mode
    virtual bool openFormat7(uint64_t cameraeuid, Nimble::Recti roi, float fps, int mode);

    /// Starts the camera data transmission.
    /// @return True is succeeded, false otherwise
    virtual bool start();
    /// Stops the camera data transmission
    /// @return True is succeeded, false otherwise
    virtual bool stop();
    /// Shuts down the connection
    /// @return True is succeeded, false otherwise
    virtual bool close();

    virtual bool isInitialized() const;
    virtual uint64_t uid();

    /// Sets the camera EUID that will be used to select the camera
    void setCameraEuid64(uint64_t euid) { m_euid = euid; }

    /// Returns information about this particular camera object
    virtual CameraInfo cameraInfo();

    dc1394camera_t * dc1394Camera() { return m_camera; }

    virtual int framesBehind() const { return m_framesBehind; }

    /// Reset all Firewire buses
    static void busReset();

  private:
    bool enableCameraFeature(unsigned int feature,
                             const QString & description,
                             bool automatic_mode,
                             unsigned int * feature_min_value,
                             unsigned int * feature_max_value);

    bool findCamera(uint64_t euid);
    bool captureSetup(int buffers);

    QString    m_videodevice;

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

  class RADIANT_API CameraDriver1394 : public CameraDriver
  {
  public:
    CameraDriver1394() {}
    virtual ~CameraDriver1394();

    virtual size_t queryCameras(std::vector<VideoCamera::CameraInfo> & cameras);
    virtual VideoCamera * createCamera();
    virtual QString driverName() const { return "libdc"; }
  };

}

/// @endcond
#endif

