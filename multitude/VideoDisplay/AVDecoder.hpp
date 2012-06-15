#ifndef VIDEODISPLAY_AVDECODER_HPP
#define VIDEODISPLAY_AVDECODER_HPP

#include "Export.hpp"

#include <Nimble/Vector2.hpp>
#include <Nimble/Matrix4.hpp>

#include <Radiant/Allocators.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/Thread.hpp>

#include <Valuable/Node.hpp>

#include <QMap>
#include <QString>

#include <array>
#include <cassert>

namespace VideoPlayer2
{
  struct Timestamp
  {
    Timestamp (double p = 0.0, int sg = 0) : pts(p), seekGeneration(sg) {}
    double pts;
    int seekGeneration;
    bool operator<(const Timestamp & ts) const
    {
      return seekGeneration == ts.seekGeneration ?
            pts < ts.pts : seekGeneration < ts.seekGeneration;
    }
  };

  struct DecodedImageBuffer
  {
    DecodedImageBuffer() {}
    QAtomicInt refcount;
    std::vector<uint8_t, Radiant::aligned_allocator<uint8_t, 32>> data;
  private:
    DecodedImageBuffer(const DecodedImageBuffer &);
    DecodedImageBuffer & operator=(DecodedImageBuffer &);
  };

  class VideoFrame
  {
  public:
    VideoFrame() : imageSize(0, 0), imageBuffer(nullptr), format(UNKNOWN), planes(0) {}

    Timestamp timestamp;

    Nimble::Vector2i imageSize;

    std::array<Nimble::Vector2i, 4> planeSize;
    std::array<int, 4> lineSize;
    std::array<const uint8_t *, 4> data;

    DecodedImageBuffer * imageBuffer;

    enum Format
    {
      UNKNOWN,
      GRAY,
      GRAY_ALPHA,
      RGB,
      RGBA,
      YUV,
      YUVA
    };
    Format format;
    int planes;

  private:
    VideoFrame(const VideoFrame &);
    VideoFrame(VideoFrame &&);
    VideoFrame& operator=(const VideoFrame &);
    VideoFrame& operator=(VideoFrame &&);
  };

  class VIDEODISPLAY_API AVDecoder : public Radiant::Thread, public Valuable::Node
  {
  public:
    enum SeekType
    {
      SeekNone = 0,
      SeekBySeconds,
      SeekRelative,
      SeekByBytes
    };

    enum SeekDirection
    {
      SeekAnyDirection = 0,
      SeekOnlyForward,
      SeekOnlyBackward
    };

    enum SeekFlags
    {
      SeekFlagsNone = 0,
      SeekRealTime
    };

    enum PlayMode
    {
      Pause,
      Play
    };

    struct SeekRequest
    {
      SeekRequest(double value = 0.0, SeekType type = SeekNone,
                  SeekDirection direction = SeekAnyDirection)
        : value(value), type(type), direction(direction) {}
      double value;
      SeekType type;
      SeekDirection direction;
    };

    /// @todo This documentation (and struct) is very ffmpeg-specific. In
    ///       theory we could support also other engines, so maybe fine-tune
    ///       the documentation a bit
    struct Options
    {
      Options()
        : audioChannels(2)
        , loop(true)
        , audio(true)
        , video(true)
        , playMode(Pause)
        , videoStreamIndex(-1)
        , audioStreamIndex(-1)
        , audioBufferSeconds(2.0)
        , videoBufferFrames(10)
      {}

      /// Input file, device, URL or special parameter to input format.
      ///
      /// Some examples: "/home/multi/Videos/video.mkv"
      ///                "background-music.ogg"
      ///                "rtmp://live.example.com/stream"
      ///                "udp://127.0.0.1:1234"
      ///                "image_sequence_%04.jpg"
      ///                "/dev/video0"      (video4linux2)
      ///                "/dev/video1394/0" (libdc1394)
      ///
      /// For detailed information, see ffmpeg documentation. To list all
      /// available protocols, run ffmpeg -protocols
      ///
      /// Required field
      QString src;

      /// Input format.
      ///
      /// Specify the input format. Usually this is not needed, but for example
      /// "video4linux2" isn't automatically detected.
      ///
      /// List all available formats (demuxers): ffmpeg -formats
      ///
      /// Default: auto detect (empty string)
      QString format;

      /// Down-mix or up-mix to given number of audio channels, eg. 5.1 -> stereo
      ///
      /// This will use usual down/up-mix algorithms, if you need to do something
      /// special, you can leave this to default and add a new audio filter
      /// instead, for example if you don't want to do the normal down-mix from
      /// 5.1 to 2.0, but just drop the extra channels, you can add new filter:
      /// pan="stereo: c0=FL : c1=FR"
      ///
      /// 0 or negative value means disabled down/up-mix
      ///
      /// Default: 2
      int audioChannels;

      /// Seek to this position before starting the playback
      SeekRequest seek;

      /// Play media in loop, can be changed with AVDecoder::setLooping()
      /// Default: true
      bool loop;

      /// Enable audio
      /// Default: true
      bool audio;

      /// Enable video
      /// Default: true
      bool video;

      /// Initial play mode
      PlayMode playMode;

      /// Demuxer and AVFormatContext options, see ffmpeg -help for full list.
      ///
      /// Examples: "fflags": "ignidx" to ignore index
      ///           "cryptokey": "<binary>" specify decryption key
      ///
      /// Default: empty
      QMap<QString, QString> demuxerOptions;

      /// Codec and AVCodecContext options, see ffmpeg -help for full list
      /// Examples: "threads": "3" use multi-threaded decoding
      ///           "lowres": "2" decode in 1/4 resolution
      ///
      /// Default: empty
      QMap<QString, QString> videoOptions;
      /// @copydoc videoOptions
      QMap<QString, QString> audioOptions;

      /// Ask for a specific stream index for video/audio.
      /// For example if media file has two video or audio tracks, setting 1
      /// here will select the second one.
      ///
      /// Default: select "best" stream (-1)
      ///          (How "best" is defined is implementation specific detail)
      int videoStreamIndex;
      /// @copydoc videoStreamIndex
      int audioStreamIndex;

      /// Libavfilter video and audio filters.
      ///
      /// Video filter examples:
      ///
      /// crop=2/3*in_w:2/3*in_h
      ///   Crop the central input area with size 2/3 of the input video
      ///
      /// lutyuv=y=gammaval(0.5)
      ///   Correct luminance gamma by a 0.5 factor
      ///
      /// yadif
      ///   Deinterlace the input video
      ///
      /// ass=subtitles.ass
      ///   Draw Advanced Substation Alpha subtitles on the video.
      ///
      /// Audio filter examples:
      ///
      /// earwax
      ///   Make audio easier to listen to on headphones by adding 'cues' so
      ///   that the stereo image is moved from inside your head to outside and
      ///   in front of the listener.
      ///
      /// pan=1:c0=0.9*c0+0.1*c1
      ///   Down-mix from stereo to mono, but with a bigger factor for the left
      ///   channel.
      ///
      /// pan="stereo:c1=c1"
      ///   Mute left channel in stereo channel layout
      ///
      /// @note Adding filters might hurt performance. In some cases converting
      ///       filters are added automatically, for example to resample audio
      ///       to Resonant sample rate or convert the pixel format to any of
      ///       the supported formats.
      /// @see http://ffmpeg.org/libavfilter.html
      ///
      /// Default: No extra filters added (empty string)
      QString videoFilters;
      /// @copydoc videoFilters
      QString audioFilters;

      /// Preferred decoded audio buffer size in seconds. Note that this has
      /// nothing to do with latency. Implementation might not obey this.
      /// Normally there is no reason to touch this.
      /// Default: 2.0 seconds
      double audioBufferSeconds;

      /// Preferred decoded video buffer size in frames.
      /// @see audioBufferSeconds
      /// Default: 10
      int videoBufferFrames;
    };

  public:
    virtual ~AVDecoder();

    virtual void close() = 0;

    virtual PlayMode playMode() const = 0;
    virtual void setPlayMode(PlayMode mode) = 0;
    virtual void seek(const SeekRequest & req) = 0;
    /// Special mode for low-latency seeking without buffering. This should be
    /// used only with certain UI elements, where the seeking target
    /// might change in real-time.
    /// When in real-time seeking mode, the video acts like it's paused
    virtual void setRealTimeSeeking(bool value) = 0;

    virtual Nimble::Vector2i videoSize() const = 0;

    void seekRelative(double pos) { seek(SeekRequest(pos, SeekRelative, SeekAnyDirection)); }
    void seek(double seconds) { seek(SeekRequest(seconds, SeekBySeconds, SeekAnyDirection)); }

    virtual void setLooping(bool doLoop) = 0;

    /// Media duration in seconds
    virtual double duration() const = 0;

    virtual Timestamp getTimestampAt(const Radiant::TimeStamp & ts) const = 0;
    virtual VideoFrame * getFrame(const Timestamp & ts) const = 0;
    virtual void releaseOldVideoFrames(const Timestamp & ts, bool * eof = nullptr) = 0;

    static std::unique_ptr<AVDecoder> create(const Options & options,
                                             const QString & backend = "");

    virtual Nimble::Matrix4f yuvMatrix() const = 0;

  protected:
    AVDecoder();
    virtual void load(const Options & options) = 0;
  };

}

#endif // VIDEODISPLAY_AVDECODER_HPP
