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

#include "Export.hpp"
#include "SubTitles.hpp"
#include "VideoIn.hpp"

#include <Luminous/Collectable.hpp>
#include <Luminous/GLSLProgramObject.hpp>
#include <Luminous/Texture.hpp>

#include <Nimble/Vector2.hpp>
#include <Nimble/Matrix3.hpp>

#include <Radiant/Color.hpp>
#include <memory>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/VideoImage.hpp>
#include <Radiant/VideoInput.hpp>

#include <Resonant/DSPNetwork.hpp>

#include <Valuable/Node.hpp>
#include <Valuable/AttributeFloat.hpp>

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
  class VIDEODISPLAY_API ShowGL : public Luminous::Collectable,
  public Valuable::Node
  {
    MEMCHECKED_USING(Valuable::Node);
  private:

    class YUVProgram : public Luminous::GLSLProgramObject
    {
    public:
      YUVProgram(Luminous::RenderContext * resources);
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
      MyTextures(Luminous::RenderContext * resources);
      ~MyTextures();

      virtual void bind();
      virtual void unbind();
      void doTextures(int frame, Radiant::VideoImage *);

      Vector2i planeSize(const Radiant::VideoImage *img, size_t i);

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

    /// The playback state of the video display
    enum State {
      PLAY,
      PAUSE
    };

    /// @cond
    enum {
      HISTOGRAM_POINTS = 256
                       };
    /// @endcond

    /// Constructs an empty ShowGL object
    ShowGL();
    ~ShowGL();

    /// Load a subtitle file
    bool loadSubTitles(const char * filename, const char * type = 0);

    /// The time-stamp of the first video frame
    Radiant::TimeStamp firstFrameTime() const;

    /// Initialize the file, but does not play it.
    /** Does not actually start playback, just loads in information
        about the video.

        @param filename The name ofthe video file to play.

        @param previewpos The position for taking the preview frame from the video.
        Currently ignored.

        @param targetChannel The sound output channel for the audio. If this value
        is less than zero, then the sound-track of the video will be spread over
        all output channels. For example if the file had two channels, and one was
        running a sound system with 8 loudspeaker, then the stereo sound would be
        replicated four times across the speakers. If the value is at least zero,
        then the sound is directed only that speaker. Stereo (or multi-channel
        sound-tracks) are spread over outputs so that the first audio channel goes
        to the specified channel, and the other channels go to the speakers
        after the first channel.

        @param flags Flags for the video playback. For the playback to work, the flags
        should include Radiant::WITH_VIDEO and Radiant::WITH_AUDIO.

        @return True if initialization succeeds or this file was already playing.
                False on error.
    */
    bool init(const char * filename,
              float previewpos = 0.05f,
              int targetChannel = -1,
              int flags =
              Radiant::WITH_VIDEO | Radiant::WITH_AUDIO);

    /// Sets the gain factor for the video sounds
    /// The gain coefficient is a linear multiplier for the video sound-track.
    /// Default value for the gain is 1.0, which equals unity gain.
    /// @param gain The new gain factor
    void setGain(float gain);

    /// Starts file playback. If the video is already playing and fromOldPos is
    /// false, we just seek to beginning.
    /// @param fromOldPos True if the playing should continue from the last
    ///                   playback position
    /// @return True if video is now playing or was already at play-state. False
    ///         on error.
    bool start(bool fromOldPos = true);
    /// Stops file playback
    bool stop();

    /// Toggles play/pause state
    bool togglePause();

    /// Pauses the video
    bool pause();

    /// Starts video playback from current position
    bool unpause();

    /// Returns the state of this video
    State state() const { return m_state; }

    /// Update the video image from reader-thread
    void update();
    /// Render the video to the specified rectangle
    /** @param resources The container of the OpenGL resources

        @param topleft Top-left corner of the video image

        @param bottomright Bottom-right corner of the video image. If
        bottomright = topleft, then the player will use the size of
        the video.

        @param baseColor color used to modulate the video with

        @param transform The coordinates can be optionally transformed
        with the "transform" matrix.

        @param subtitleFont The font to be used for rendering subtitles

        @param subTitleSpace The amount of space allocated for the subtitles
        The caller can indicate the amount of space it has allocated beneath the
        video widget for the subtitles. The player will place the subtitles beneath
        the player if there is enough spaec for two lines of text. Otherwise the
        the subtitles will be placed inside the video area.*/
    void render(Luminous::RenderContext * resources,
                                 Vector2 topleft,
                                 Vector2 bottomright,
                                 Radiant::Color baseColor,
                                 const Nimble::Matrix3f * transform = 0,
                                 Poetic::GPUFont * subtitleFont = 0,
                                 float subTitleSpace = 0);

    /// Pixel size of the video image.
    Nimble::Vector2i size() const;

    /// Returns the duration (lenght) of the video
    Radiant::TimeStamp duration() const { return m_duration; }
    /// Returns the current playback position of the video
    Radiant::TimeStamp position() const { return m_position; }
    /// The relative playback position of the current video
    double relativePosition() const { return position().value() / (double) duration().value(); }

    /// Seek to given position. Due to limitations of underlying seek
    /// algorithms, this method is usually not exact.
    /// @param time New position timestamp so that 0 is the beginning of the video
    ///             This is clamped between 0 and duration().
    void seekTo(Radiant::TimeStamp time);
    /// Seeks to a relative position within the video
    /** @param relative The relative position, in range [0,1]. */
    void seekToRelative(double relative);

    /// Seek forward, or backward by a given amount
    void seekBy(const Radiant::TimeStamp & ts) { seekTo(position() + ts); }

    /// Pans the video sounds to a given location
    void panAudioTo(Nimble::Vector2 location);

    /// Information on how the frames have been displayed. The
    /// histogram information is useful mostly for debug purposes.
    /// @param index Index to histogram data, 0 <= index < HISTOGRAM_POINTS
    /// @return Frame display count
    int histogramPoint(int index) const { return m_histogram[index]; }
    /// Returns the number of histogram updates.
    size_t histogramIndex() const { return m_updates; }

    /// Query if the video has subtitles.
    /// @return true if this video has been loaded with subtitles.
    bool hasSubTitles() { return m_subTitles.size() != 0; }

    /// Returns the currently used filename
    const QString & filename() const { return m_filename; }

    /// Adjusts the contrast
    /** Contrast of 1.0f means that the video image is unmodified,
        which is the default. Values greater than 1.0 amplify the dark and
        bright areas, with midtones retaining their brightness.
        Values between zero and 1.0 reduce the contrast. You can also use
        negative contrast values, to create special effects.

        The contrast parameter may not be honored by all rendering back-ends.

        @param contrast The new contrast value
          */
    void setContrast(float contrast) { m_contrast = contrast; }

    /// Sets different synchronizing mode. If SyncToTime is enabled, current
    /// visible frame is calculated using wall clock time. That usually means
    /// smoother playback, but can cause some audio/video synchronization issues.
    /// Those issues are fixed by doing automatic av-resync if the difference
    /// becomes too large.
    /// If this is disabled, current frame is taken directly from
    /// AudioTransfer::videoFrame(). This option minimizes audio sync problems,
    /// but on some setups causes significant jerkiness / frame skipping.
    /// @param flag True to enable sync to time feature
    void setSyncToTime(bool flag);

  private:

    void clearHistogram();

    QString             m_filename;
    VideoIn               * m_video;
    VideoIn::Frame        * m_frame;
    VideoIn::Frame          m_preview;
    std::shared_ptr<Resonant::DSPNetwork> m_dsp;
    Resonant::DSPNetwork::Item m_dspItem;
    AudioTransfer         * m_audio;
    int                     m_targetChannel;
    float                   m_gain;
    int                     m_videoFrame;
    int                     m_count;
    State                   m_state;
    int                     m_histogram[HISTOGRAM_POINTS];
    size_t                  m_updates;
    bool                    m_seeking;

    Radiant::TimeStamp      m_duration;
    Radiant::TimeStamp      m_position;

    SubTitles               m_subTitles;

    Valuable::AttributeFloat    m_contrast;

    Radiant::TimeStamp      m_started;
    float m_fps;
    bool m_syncToTime;
    int m_outOfSync;
    int m_outOfSyncTotal;
    int m_syncing;
    int m_frames;
  };

}

#endif