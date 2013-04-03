#ifndef VIDEODISPLAY_AVDECODER_HPP
#define VIDEODISPLAY_AVDECODER_HPP

#include "Export.hpp"

#include <Nimble/Size.hpp>
#include <Nimble/Vector2.hpp>
#include <Nimble/Matrix4.hpp>

#include <Radiant/Allocators.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/Thread.hpp>

#include <Valuable/Node.hpp>
#include <Valuable/State.hpp>

#include <QMap>
#include <QString>

#include <array>
#include <cassert>

/** VideoDisplay is a video player library.

  \b Copyright: All rights reserved, MultiTouch Oy. You may use this
  library only for purposes for which you have a specific, written
  license from MultiTouch Oy.


 */

namespace VideoDisplay
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

  /// This class provices the actual audio/video decoder for the video player.
  class VIDEODISPLAY_API AVDecoder : public Radiant::Thread
  {
  public:
    enum State
    {
      STATE_LOADING          = 1 << 1,
      STATE_HEADER_READY     = 1 << 2,
      STATE_READY            = 1 << 3,
      STATE_ERROR            = 1 << 4,
      STATE_FINISHED         = 1 << 5
    };
    typedef Valuable::State<State> VideoState;

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

    /// Video and audio parameters for AVDecoder when opening a new media file.
    struct Options
    {
      /// Creates an empty parameter set. You must set at least source().
      Options()
        : m_channelLayout("downmix")
        , m_looping(false)
        , m_audioEnabled(true)
        , m_videoEnabled(true)
        , m_playMode(Pause)
        , m_videoStreamIndex(-1)
        , m_audioStreamIndex(-1)
        , m_audioBufferSeconds(2.0)
        , m_videoBufferFrames(10)
        , m_pixelFormat(VideoFrame::UNKNOWN)
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
      /// For detailed information, see libav documentation. To list all
      /// available protocols, run avconv -protocols
      /// @sa setSource
      /// @return media source
      const QString & source() const { return m_source; }
      /// @param source media source
      /// @sa source
      void setSource(const QString & source) { m_source = source; }

      /// Input format.
      ///
      /// Specify the input format. Usually this is not needed, but for example
      /// "video4linux2" isn't automatically detected.
      ///
      /// List all available formats (demuxers): ffmpeg -formats
      ///
      /// Default: auto detect (empty string)
      /// @sa setFormat
      /// @return explicitly set input format
      const QString & format() const { return m_format; }
      /// @sa format
      /// @param format input format
      void setFormat(const QString & format) { m_format = format; }

      /// Down-mix or up-mix to given number of audio channels, eg. 5.1 -> stereo
      ///
      /// This will use usual down/up-mix algorithms, if you need to do something
      /// special, you can leave this to default and add a new audio filter
      /// instead, for example if you don't want to do the normal down-mix from
      /// 5.1 to 2.0, but just drop the extra channels, you can add new filter:
      /// pan="stereo: c0=FL : c1=FR"
      ///
      /// Empty string means disabled down/up-mix
      ///
      /// Default: "downmix"
      /// @sa channelLayout
      /// @return target channel layout
      const QByteArray & channelLayout() const { return m_channelLayout; }
      /// @sa channelLayout
      /// @param channelLayout target channel layout
      void setChannelLayout(const QByteArray & channelLayout) { m_channelLayout = channelLayout; }

      /// Seek to this position before starting the playback
      /// @sa setsetSeekRequest
      /// @return initial seek request
      SeekRequest seekRequest() const { return m_seekRequest; }
      /// @sa seekRequest
      /// @param seekRequest Initial seek request
      void setSeekRequest(SeekRequest seekRequest) { m_seekRequest = seekRequest; }

      /// Play media in loop, can be changed with AVDecoder::setLooping()
      /// Default: false
      /// @sa setLooping
      /// @return looping mode
      bool isLooping() const { return m_looping; }
      /// @sa isLooping
      /// @param looping looping mode
      void setLooping(bool looping) { m_looping = looping; }

      /// Should any audio tracks be opened and decoded
      /// You need to have at least one track enabled
      /// Default: true
      /// @sa setAudioEnabled
      /// @return true if audio is enabled
      bool isAudioEnabled() const { return m_audioEnabled; }
      /// @sa isAudioEnabled
      /// @param audioEnabled should audio tracks be enabled
      void setAudioEnabled(bool audioEnabled) { m_audioEnabled = audioEnabled; }

      /// Should any video tracks be opened and decoded
      /// You need to have at least one track enabled
      /// Default: true
      /// @sa setVideoEnabled
      /// @return true if video is enabled
      bool isVideoEnabled() const { return m_videoEnabled; }
      /// @sa isVideoEnabled
      /// @param videoEnabled should video tracks be enabled
      void setVideoEnabled(bool videoEnabled) { m_videoEnabled = videoEnabled; }

      /// Initial play mode
      /// @sa setPlayMode
      /// @return initial play mode
      PlayMode playMode() const { return m_playMode; }
      /// @sa playMode
      /// @param playMode initial play mode
      void setPlayMode(PlayMode playMode) { m_playMode = playMode; }

      /// Demuxer and AVFormatContext options, see avconv -h full for full list.
      ///
      /// Examples: "fflags": "ignidx" to ignore index
      ///           "cryptokey": "<binary>" specify decryption key
      ///
      /// Default: empty
      /// @return demuxer options map
      const QMap<QString, QString> & demuxerOptions() const { return m_demuxerOptions; }
      /// Sets all demuxer options
      /// @param demuxerOptions demuxer options
      /// @sa setDemuxerOption
      /// @sa demuxerOptions
      void setDemuxerOptions(const QMap<QString, QString> & demuxerOptions)
      {
        m_demuxerOptions = demuxerOptions;
      }
      /// Sets value for one demuxer option
      /// @param key name for the option
      /// @param value value for the option
      /// @sa setDemuxerOptions
      /// @sa demuxerOptions
      void setDemuxerOption(const QString & key, const QString & value)
      {
        m_demuxerOptions[key] = value;
      }

      /// Codec and AVCodecContext options, see avconv -h full for full list
      /// Examples: "threads": "3" use multi-threaded decoding
      ///           "lowres": "2" decode in 1/4 resolution
      ///
      /// Default: empty
      /// @return video/audio options map
      const QMap<QString, QString> & videoOptions() const { return m_videoOptions; }
      /// Sets all video options
      /// @param videoOptions video and video codec options
      /// @sa setVideoOption
      /// @sa videoOptions
      void setVideoOptions(const QMap<QString, QString> & videoOptions)
      {
        m_videoOptions = videoOptions;
      }
      /// Sets value for one video option
      /// @param key name for the option
      /// @param value value for the option
      /// @sa setVideoOptions
      /// @sa videoOptions
      void setVideoOption(const QString & key, const QString & value)
      {
        m_videoOptions[key] = value;
      }

      /// @copydoc videoOptions
      const QMap<QString, QString> & audioOptions() const { return m_audioOptions; }
      /// Sets all audio options
      /// @param audioOptions audio and audio codec options
      /// @sa setAudioOption
      /// @sa audioOptions
      void setAudioOptions(const QMap<QString, QString> & audioOptions)
      {
        m_audioOptions = audioOptions;
      }
      /// Sets value for one audio option
      /// @param key name for the option
      /// @param value value for the option
      /// @sa setAudioOptions
      /// @sa audioOptions
      void setAudioOption(const QString & key, const QString & value)
      {
        m_audioOptions[key] = value;
      }

      /// Ask for a specific stream index for video/audio.
      /// For example if media file has two video or audio tracks, setting 1
      /// here will select the second one.
      ///
      /// Default: select "best" stream (-1)
      ///          (How "best" is defined is implementation specific detail)
      /// @return stream index number
      int videoStreamIndex() const { return m_videoStreamIndex; }
      /// @param videoStreamIndex selected video stream index number
      /// @sa videoStreamIndex
      void setVideoStreamIndex(int videoStreamIndex) { m_videoStreamIndex = videoStreamIndex; }

      /// @copydoc videoStreamIndex
      /// @sa setAudioStreamIndex
      int audioStreamIndex() const { return m_audioStreamIndex; }
      /// @param audioStreamIndex selected audio stream index number
      /// @sa audioStreamIndex
      void setAudioStreamIndex(int audioStreamIndex) { m_audioStreamIndex = audioStreamIndex; }

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
      /// @see http://libav.org/libavfilter.html
      ///
      /// Default: No extra filters added (empty string)
      /// @return video / audio filter string
      const QString & videoFilters() const { return m_videoFilters; }
      /// @sa videoFilters
      /// @param videoFilters video filter string
      void setVideoFilters(const QString & videoFilters) { m_videoFilters = videoFilters; }

      /// @copydoc videoFilters
      const QString & audioFilters() const { return m_audioFilters; }
      /// @sa audioFilters
      /// @param audioFilters audio filter string
      void setAudioFilters(const QString & audioFilters) { m_audioFilters = audioFilters; }

      /// Preferred decoded audio buffer size in seconds. Note that this has
      /// nothing to do with latency. Implementation might not obey this.
      /// Normally there is no reason to touch this.
      /// Default: 2.0 seconds
      /// @sa setAudioBufferSeconds
      /// @return preferred decoded audio buffer size in seconds
      double audioBufferSeconds() const { return m_audioBufferSeconds; }
      /// @sa audioBufferSeconds
      /// @param audioBufferSeconds preferred decoded audio buffer size in seconds
      void setAudioBufferSeconds(double audioBufferSeconds)
      {
        m_audioBufferSeconds = audioBufferSeconds;
      }

      /// Preferred decoded video buffer size in frames.
      /// Default: 10
      /// @see audioBufferSeconds
      /// @sa setVideoBufferFrames
      /// @return preferred decoded video buffer size in frames
      int videoBufferFrames() const { return m_videoBufferFrames; }
      /// @sa videoBufferFrames
      /// @param videoBufferFrames Preferred decoded video buffer size in frames
      void setVideoBufferFrames(int videoBufferFrames) { m_videoBufferFrames = videoBufferFrames; }

      /// Preferred output pixel format, by default choose the best pixel format
      /// to remove or at least minimize the conversion overhead.
      /// Do not touch this unless you have a good reason.
      /// @sa setPixelFormat
      /// @return preferred output pixel format
      VideoFrame::Format pixelFormat() const { return m_pixelFormat; }
      /// @sa pixelFormat
      /// @param pixelFormat preferred output pixel format
      void setPixelFormat(VideoFrame::Format pixelFormat) { m_pixelFormat = pixelFormat; }

    private:
      QString m_source;
      QString m_format;
      QByteArray m_channelLayout;
      SeekRequest m_seekRequest;
      bool m_looping;
      bool m_audioEnabled;
      bool m_videoEnabled;
      PlayMode m_playMode;
      QMap<QString, QString> m_demuxerOptions;
      QMap<QString, QString> m_videoOptions;
      QMap<QString, QString> m_audioOptions;
      int m_videoStreamIndex;
      int m_audioStreamIndex;
      QString m_videoFilters;
      QString m_audioFilters;
      double m_audioBufferSeconds;
      int m_videoBufferFrames;
      VideoFrame::Format m_pixelFormat;
    };

  public:
    virtual ~AVDecoder();

    VideoState & state();
    const VideoState & state() const;
    bool finished() const;
    bool isHeaderReady() const;
    bool hasError() const;

    virtual void close() = 0;

    virtual PlayMode playMode() const = 0;
    virtual void setPlayMode(PlayMode mode) = 0;
    virtual void seek(const SeekRequest & req) = 0;
    /// Special mode for low-latency seeking without buffering. This should be
    /// used only with certain UI elements, where the seeking target
    /// might change in real-time.
    /// When in real-time seeking mode, the video acts like it's paused
    virtual void setRealTimeSeeking(bool value) = 0;

    virtual Nimble::Size videoSize() const = 0;

    void seekRelative(double pos) { seek(SeekRequest(pos, SeekRelative, SeekAnyDirection)); }
    void seek(double seconds) { seek(SeekRequest(seconds, SeekBySeconds, SeekAnyDirection)); }

    virtual void setLooping(bool doLoop) = 0;

    /// Media duration in seconds
    virtual double duration() const = 0;

    virtual Timestamp getTimestampAt(const Radiant::TimeStamp & ts) const = 0;
    virtual Timestamp latestDecodedTimestamp() const = 0;
    virtual VideoFrame * getFrame(const Timestamp & ts) const = 0;
    virtual int releaseOldVideoFrames(const Timestamp & ts, bool * eof = nullptr) = 0;

    static std::shared_ptr<AVDecoder> create(const Options & options,
                                             const QString & backend = "");

    virtual Nimble::Matrix4f yuvMatrix() const = 0;

    virtual void panAudioTo(Nimble::Vector2f location) const = 0;


  protected:
    AVDecoder();
    virtual void load(const Options & options) = 0;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
  typedef std::shared_ptr<AVDecoder> AVDecoderPtr;
}

#endif // VIDEODISPLAY_AVDECODER_HPP
