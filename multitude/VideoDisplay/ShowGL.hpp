/* COPYRIGHT
 *
 * This file is part of VideoDisplay.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "VideoDisplay.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */


#ifndef VIDEODISPLAY_SHOW_GL_HPP
#define VIDEODISPLAY_SHOW_GL_HPP

#include <Luminous/Collectable.hpp>
#include <Luminous/GLSLProgramObject.hpp>
#include <Luminous/Texture.hpp>

#include <Nimble/Vector2.hpp>
#include <Nimble/Matrix3.hpp>

#include <Radiant/RefPtr.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/VideoImage.hpp>
#include <Radiant/VideoInput.hpp>

#include <Resonant/DSPNetwork.hpp>

#include <Valuable/HasValues.hpp>
#include <Valuable/ValueFloat.hpp>

#include <VideoDisplay/Export.hpp>
#include <VideoDisplay/SubTitles.hpp>
#include <VideoDisplay/VideoIn.hpp>

namespace Resonant {
  class DSPNetwork;
}

namespace Poetic {
  class GPUFont;
}

namespace VideoDisplay {

  using Nimble::Vector2;
  using Nimble::Vector2i;
  using namespace Radiant;

  class AudioTransfer;

  /// Objects that displays video using an OpenGL device
  /** This class manages the process of reading the file from a disc,
      playing back the audio and displaying the video with OpenGL. The
      video rendering is performed with the aid of shaders, so this
      class will not work in cases where OpenGL 2.0 -level hardware is
      not available.

      In the rendering phase one can adjust the contrast. This is
      internally done by using a shader to with relevant controls.

      From application-programmers perspective, this is the main class
      of the VideoDisplay framework. */
  class ShowGL : public Luminous::Collectable,
  public Valuable::HasValues
  {
  private:

    class YUVProgram : public Luminous::GLSLProgramObject
    {
    public:
      YUVProgram(Luminous::GLResources * resources);
      virtual ~YUVProgram();

      bool init();
      virtual void bind(float contrast);
      virtual bool link();

    private:

      enum {
        PARAM_YTEX,
        PARAM_UTEX,
        PARAM_VTEX,
        PARAM_MATRIX,
        PARAM_SIZEOF
      };

      int m_uniforms[PARAM_SIZEOF];
    };

    class MyTextures : public Luminous::GLResource
    {
    public:
      MyTextures(Luminous::GLResources * resources);
      ~MyTextures();

      virtual void bind();
      virtual void unbind();
      void doTextures(int frame, Radiant::VideoImage *);

      Vector2i planeSize(const Radiant::VideoImage *img, uint i);

      Luminous::Texture2D & blankTex() { return m_blankTex; }

    private:

      void doTexturesRGB(Radiant::VideoImage *);
      void doTexturesYUV(Radiant::VideoImage *);

      int m_frame;
      Luminous::Texture2D  m_texIds[3];
      Vector2i             m_texSizes[3];
      Luminous::Texture2D  m_blankTex;

    };

  public:

    enum State {
      PLAY,
      PAUSE
    };

    enum {
      HISTOGRAM_POINTS = 256
                       };

    VIDEODISPLAY_API ShowGL();
    VIDEODISPLAY_API ~ShowGL();

    /// Load a subtitle file
    VIDEODISPLAY_API bool loadSubTitles(const char * filename, const char * type = 0);

    /// Initialize the file, but does not play it.
    /** Does not actually start playback, just loads in information
        about the video.

        @arg filename The name ofthe video file to play.

        @arg dsp The DSP graph that is used for audio playback. If null, then
        this method will pick up the default network.

        @arg targetChannel The sound output channel for the audio. If this value
        is less than zero, then the sound-track of the video will be spread over
        all output channels. For example if the file had two channels, and one was
        running a sound system with 8 loudspeaker, then the stereo sound would be
        replicated four times across the speakers. If the value is at least zero,
        then the sound is directed only that speaker. Stereo (or multi-channel
        sound-tracks) are spread over outputs so that the first audio channel goes
        to the specified channel, and the other channels go to the speakers
        after the first channel.

        @arg flags Flags for the video playback. For the playback to work, the flags
        should include Radiant::WITH_VIDEO and Radiant::WITH_AUDIO.

    */
    VIDEODISPLAY_API bool init(const char * filename,
                               Resonant::DSPNetwork  * dsp,
                               float previewpos = 0.05f,
                               int targetChannel = -1,
                               int flags =
                               Radiant::WITH_VIDEO | Radiant::WITH_AUDIO);
    /// Opens the file for playing.
    /* VIDEODISPLAY_API bool open(const char * filename, Resonant::DSPNetwork  * dsp,
                               Radiant::TimeStamp pos = 0);
    */
    /// Starts file playback, from the last playback position.
    VIDEODISPLAY_API bool start(bool fromOldPos = true);
    /// Stops file playback
    VIDEODISPLAY_API bool stop();

    /// Toggles play/pause state
    VIDEODISPLAY_API bool togglePause();

    VIDEODISPLAY_API bool pause();

    VIDEODISPLAY_API bool unpause();

    // VIDEODISPLAY_API void enableLooping(bool enable);

    State state() const { return m_state; }

    /// Update the video image from reader-thread
    VIDEODISPLAY_API void update();
    /// Render the video to the specified rectangle
    /**
        @arg topleft Top-left corner of the video image

        @arg bottomright Bottom-right corner of the video image. If
        bottomright = topleft, then the player will use the size of
        the video.

        @arg ransform The coordinates can be optionally transformed
        with the "transform" matrix. */
    VIDEODISPLAY_API void render(Luminous::GLResources * resources,
                                 Vector2 topleft, Vector2 bottomright,
                                 const Nimble::Matrix3f * transform = 0,
                                 Poetic::GPUFont * subtitleFont = 0,
                                 float subTitleSpace = 0);

    /// Pixel size of the video image.
    VIDEODISPLAY_API Nimble::Vector2i size() const;

    Radiant::TimeStamp duration() { return m_duration; }
    Radiant::TimeStamp position() { return m_position; }
    /// The relative playback position of the current video
    double relativePosition() { return position() / (double) duration(); }

    /** Seek to given position. Due to limitations of underlying seek
    algorithms, this method is usually not exact. */
    VIDEODISPLAY_API void seekTo(Radiant::TimeStamp time);
    VIDEODISPLAY_API void seekToRelative(double relative);
    void seekBy(const Radiant::TimeStamp & ts) { seekTo(position() + ts); }

    VIDEODISPLAY_API void panAudioTo(Nimble::Vector2 location);

    /** Information on how the frames have been displayed. The
    histogram information is useful mostly for debug purposes. */
    int histogramPoint(int index) const { return m_histogram[index]; }
    int histogramIndex() const { return m_updates; }

    /** Returns true if this video has been loaded with subtitles. */
    bool hasSubTitles() { return m_subTitles.size() != 0; }

    /// Returns the currently used filename
    const std::string & filename() const { return m_filename; }

    /// Adjusts the contrast
    /** Contrast of 1.0f means that the video image is unmodified,
        which is the default. Values greater than 1.0 amplify the dark and
        bright areas, with midtones retaining their brightness.
        Values between zero and 1.0 reduce the contrast. You can also use
        negative contrast values, to create special effects.

        The contrast parameter may not be honored by all rendering back-ends.•

          */
    void setContrast(float contrast) { m_contrast = contrast; }

  private:

    void clearHistogram();

    std::string             m_filename;
    VideoIn               * m_video;
    VideoIn::Frame        * m_frame;
    VideoIn::Frame          m_preview;
    Resonant::DSPNetwork  * m_dsp;
    Resonant::DSPNetwork::Item m_dspItem;
    AudioTransfer         * m_audio;
    int                     m_targetChannel;
    int                     m_videoFrame;
    int                     m_count;
    State                   m_state;
    int                     m_histogram[HISTOGRAM_POINTS];
    uint                    m_updates;
    bool                    m_seeking;

    Radiant::TimeStamp      m_duration;
    Radiant::TimeStamp      m_position;

    SubTitles               m_subTitles;

    Valuable::ValueFloat    m_contrast;
  };

}

#endif
