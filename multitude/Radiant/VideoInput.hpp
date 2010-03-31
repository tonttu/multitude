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

#ifndef RADIANT_VIDEO_INPUT_HPP
#define RADIANT_VIDEO_INPUT_HPP

#include <Radiant/Export.hpp>
#include <Radiant/IODefs.hpp>
#include <Radiant/VideoImage.hpp>

#include <vector>


namespace Radiant {


  /// Different video frame-rates
  enum FrameRate {
    FPS_IGNORE,
    FPS_5,
    FPS_10,
    FPS_15,
    FPS_30,
    FPS_60,
    FPS_120,
    FPS_COUNT
  };

  enum VideoInputFlags {
    DONT_CARE = -1,
    /// Try to decode the video data from the data stream
    WITH_VIDEO = 0x1,
    /// Try to decode the PCM audio from the data stream
    WITH_AUDIO = 0x2,
    /// Loop the video source, when it reaches the end
    DO_LOOP = 0x4,
    /// If the audio has multiple channels, force the audio to mono
    MONOPHONIZE_AUDIO = 0x8
  };

  RADIANT_API float asFloat(FrameRate);
  RADIANT_API FrameRate closestFrameRate(float fps);

  /// Base class for video input classes.

  /** Potential child classes are: FireWire video input, USB video
      input and movie video input.

      @authors Tommi Ilmonen and Juha Laitinen
  */

  class RADIANT_API VideoInput
  {
  public:


    virtual ~VideoInput();

    /**
     * This method captures an image from the video source. The image
     * is returned in the native format of the device.
     * @see #imageFormat() for the current format of the device.
     * @see #Radiant::ImageConversion for conversions into application video formats
     * @note The device has to be initialized, and the stream transmission has
     * to be started before this method can be called.
     * @see #open()
     * @see #start()
     */
    virtual const Radiant::VideoImage * captureImage() = 0;
    /// Inform the video handler that the application has used the image
    /** This function is necessary as some handlers need to release
    the resources that relate to a particular frame. */
    virtual void doneImage();

    /// Get audio data
    /** This function returns a pointer to the internal audio PCM buffer. The audio PCM buffer
        is filled in the #captureFrame function, and this function only returns a pointer to the
        captured data.

        This function should be called frequently, typically after each video frame.
        For many video sources (movie files in particular) this function will return null
        most of the time, as the audio is encoded in chunks so that audio frames cover
        multiple video frames. Thus you may have this function return the audio for one
        full second of the movie.

        @param frameCount The number of frames available is stored inside this pointer.
        @return Pointer to the raw PCM audio data. Usually the data is in int16_t format,
        but this is not mandatory. Use the #getAudioParameters function to determine
        the audio sample format, and other paramters. The returned memory is usable until
        the next call to captureVideo.


   */
    virtual const void * captureAudio(int * frameCount);
    /// Get audio parameters
    /** @param channels The number of channels in the video sound-track.
    @param sample_rate Audio sample rate
    @param format The audio sample format
     */
    virtual void getAudioParameters(int * channels,
                                    int * sample_rate,
                                    AudioSampleFormat * format);

    /// Returns the current width of a frame in the video stream images.
    /** Note that it is quite common for video devices to not report the correct frame
        size before at least one frame has been captured. */
    virtual int width() const = 0;
    /// Returns the current height of a frame in the video stream images.
    virtual int height() const = 0;
    /** Returns the current frame rate (int frames per second) of the
    video stream. */
    virtual float fps() const = 0;
    /// Returns the current image format of the stream.
    virtual ImageFormat imageFormat() const = 0;
    /// Returns the total size of one captured image frame in bytes.
    virtual unsigned int size() const = 0;

    virtual void setGamma(float);
    /** Sets the shutter time. Larger values lead to longer shutter
    times. Negative values tell the system to use automatic shutter
    timing. Manual range [0-1]. */
    virtual void setShutter(float);
    /** Sets the camera gain (if possible). Negative values tell the
    system to use automatic gain control. Manual range [0-1]. */
    virtual void setGain(float value);
    /// Set the exposure control of the device (if possible)
    virtual void setExposure(float value);
    /// Set the brightness control of the device (if possible)
    virtual void setBrightness(float value);

    /**
     * Starts the data transmission.
     */
    virtual bool start() = 0;

    /**
     * Stops the data transmission.
     */
    virtual bool stop() = 0;

    /**
     * Close the device.
     */
    virtual bool close() = 0;

  protected:
    /// Disabled
    VideoInput() {}

  };

}

#endif


