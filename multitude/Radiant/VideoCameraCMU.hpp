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

#ifndef RADIANT_VIDEO_CAMERA_CMU_HPP
#define RADIANT_VIDEO_CAMERA_CMU_HPP

#include <Radiant/Export.hpp>
#include <Radiant/VideoCamera.hpp>
#include <Radiant/CameraDriver.hpp>

#include <string>

class C1394Camera;
#define _WINSOCKAPI_		// timeval redefinition
#include <windows.h>
#include <1394camapi.h>

namespace Radiant {

    class RADIANT_API VideoCameraCMU : public VideoCamera
    {
    public:
        VideoCameraCMU(CameraDriver * driver);
        virtual ~VideoCameraCMU();

        //static bool queryCameras(std::vector<CameraInfo> * cameras);

        virtual bool open(uint64_t euid, int width, int height, ImageFormat fmt = IMAGE_UNKNOWN, FrameRate framerate = FPS_IGNORE);
        virtual bool openFormat7(uint64_t cameraeuid, Nimble::Recti roi, float fps, int mode);

        virtual bool start();
        virtual bool stop();
        virtual bool close();

        virtual const VideoImage * captureImage();

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

    private:
        void queryFeature(VideoCamera::FeatureType id, std::vector<VideoCamera::CameraFeature> * features);

        C1394Camera * m_camera;
        bool m_initialized;
        int m_timeoutMs;
        bool m_restartImageAcquisition;

        VideoImage m_image;
    };

    //////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////

    class CameraDriverCMU : public CameraDriver
    {
    public:
        CameraDriverCMU() {}

        virtual size_t queryCameras(std::vector<VideoCamera::CameraInfo> & cameras);
        virtual VideoCamera * createCamera();
        virtual std::string driverName() const { return "cmu"; }
    };

}

#endif
