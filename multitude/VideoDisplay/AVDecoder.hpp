#ifndef VIDEODISPLAY_AVDECODER_HPP
#define VIDEODISPLAY_AVDECODER_HPP

#include "Export.hpp"

#include <Nimble/Size.hpp>
#include <Nimble/Vector2.hpp>
#include <Nimble/Matrix4.hpp>

#include <Radiant/Allocators.hpp>
#include <Radiant/TimeStamp.hpp>
#include <Radiant/Thread.hpp>
#include <Radiant/Flags.hpp>

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

  /// Initialize underlying video library. This is called automatically when
  /// using AVDecoder, but should also be called manually if there is a need to
  /// call raw functions of the implementation (libav/ffmpeg) outside VideoDisplay
  /// library.
  /// This will:
  ///  * Register Cornerstone log handlers
  ///  * Register Cornerstone lock manager
  ///  * Initialize avcodec, avdevice, libavformat, avformat_network and avfilter
  VIDEODISPLAY_API void init();

  /// @cond

  class Timestamp
  {
  public:
    Timestamp(double pts = 0.0, int seekGeneration = 0)
      : m_pts(pts),
        m_seekGeneration(seekGeneration)
    {}

    double pts() const { return m_pts; }
    void setPts(double pts) { m_pts = pts; }

    int seekGeneration() const { return m_seekGeneration; }
    void setSeekGeneration(int seekGeneration) { m_seekGeneration = seekGeneration; }

    bool operator<(const Timestamp & ts) const
    {
      return m_seekGeneration == ts.m_seekGeneration ?
            m_pts < ts.m_pts : m_seekGeneration < ts.m_seekGeneration;
    }

  private:
    double m_pts;
    int m_seekGeneration;
  };

  class DecodedImageBuffer
  {
  public:
    typedef std::vector<uint8_t, Radiant::aligned_allocator<uint8_t, 32>> AlignedData;

  public:
    DecodedImageBuffer() {}

    void ref() { m_refcount.ref(); }
    bool deref() { return m_refcount.deref(); }

    QAtomicInt & refcount() { return m_refcount; }
    const QAtomicInt & refcount() const { return m_refcount; }

    AlignedData & data() { return m_data; }
    const AlignedData & data() const { return m_data; }

  private:
    QAtomicInt m_refcount;
    AlignedData m_data;

  private:
    DecodedImageBuffer(const DecodedImageBuffer &);
    DecodedImageBuffer & operator=(DecodedImageBuffer &);
  };

  class VideoFrame
  {
  public:
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

  public:
    VideoFrame()
      : m_imageSize(0, 0),
        m_imageBuffer(nullptr),
        m_format(UNKNOWN),
        m_planes(0),
        m_index(-1)
    {}

    Timestamp timestamp() const { return m_timestamp; }
    void setTimestamp(Timestamp ts) { m_timestamp = ts; }

    Nimble::Vector2i imageSize() const { return m_imageSize; }
    void setImageSize(Nimble::Vector2i size) { m_imageSize = size; }

    Nimble::Vector2i planeSize(int plane) const { return m_planeSize[plane]; }
    void setPlaneSize(int plane, Nimble::Vector2i size) { m_planeSize[plane] = size; }

    int lineSize(int plane) const { return m_lineSize[plane]; }
    void setLineSize(int plane, int size) { m_lineSize[plane] = size; }

    const uint8_t * data(int plane) const { return m_data[plane]; }
    void setData(int plane, uint8_t * data) { m_data[plane] = data; }

    void clear(int plane)
    {
      m_planeSize[plane] = Nimble::Vector2i(0, 0);
      m_lineSize[plane] = 0;
      m_data[plane] = nullptr;
    }

    int bytes(int plane) const
    {
      return m_lineSize[plane] * m_planeSize[plane].y;
    }

    DecodedImageBuffer * imageBuffer() { return m_imageBuffer; }
    const DecodedImageBuffer * imageBuffer() const { return m_imageBuffer; }
    void setImageBuffer(DecodedImageBuffer * imageBuffer) { m_imageBuffer = imageBuffer; }

    Format format() const { return m_format; }
    void setFormat(Format format) { m_format = format; }

    int planes() const { return m_planes; }
    void setPlanes(int planes) { m_planes = planes; }

    // Decoder sets a new unique frame index to every decoded frame.
    // Index means how many frames was decoded before this frame.
    // It is not the same as frame number.
    int index() const { return m_index; }
    void setIndex(int index) { m_index = index; }

  private:
    Timestamp m_timestamp;

    Nimble::Vector2i m_imageSize;

    std::array<Nimble::Vector2i, 4> m_planeSize;
    std::array<int, 4> m_lineSize;
    std::array<const uint8_t *, 4> m_data;

    DecodedImageBuffer * m_imageBuffer;

    Format m_format;
    int m_planes;
    int m_index;

  private:
    VideoFrame(const VideoFrame &);
    VideoFrame(VideoFrame &&);
    VideoFrame& operator=(const VideoFrame &);
    VideoFrame& operator=(VideoFrame &&);
  };

  /// @endcond

  /// This class provices the actual audio/video decoder for the video player.
  /// To use this class, first make a new instance with create() and then start
  /// the decoder thread with run().
  class VIDEODISPLAY_API AVDecoder : public Radiant::Thread
  {
  public:
    /// AVDecoder loading state
    enum State
    {
      STATE_LOADING          = 1 << 1, ///< Decoder is opening the source
      STATE_HEADER_READY     = 1 << 2, ///< Decoder has opened the source and codecs,
                                       ///  file meta info like video size is known
      STATE_READY            = 1 << 3, ///< First frame has been decoded successfully
      STATE_ERROR            = 1 << 4, ///< There was an error while opening/decoding the source
      STATE_FINISHED         = 1 << 5  ///< Playback was finished without errors
    };
    /// State class used in the decoder
    typedef Valuable::State<State> DecoderState;

    /// Describes the unit used in SeekRequest. Not all of these might be
    /// supported by the demuxer or video / audio codecs.
    enum SeekType
    {
      SEEK_NONE = 0,     ///< No seeking requested
      SEEK_BY_SECONDS,   ///< Timestamp is specified in seconds
      SEEK_RELATIVE,     ///< Timestamp is between 0 and 1, 0 meaning the beginning
                         ///  of the video, 1 is the end of the video
      SEEK_BY_BYTES      ///< Raw byte seek in the data stream. This is fast,
                         ///  but might cause rendering artifacts
    };

    /// Seeking direction constraint for SeekRequest.
    /// This eliminates twitching while seeking.
    enum SeekDirection
    {
      SEEK_ANY_DIRECTION = 0, ///< No limitations on the seeking
      SEEK_ONLY_FORWARD,      ///< Only seek forward in the stream
      SEEK_ONLY_BACKWARD      ///< Only seek backward in the stream
    };

    /// Seeking flags
    enum SeekFlags
    {
      SEEK_FLAGS_NONE = 0,    ///< No special seeking flags
      SEEK_REAL_TIME          ///< Low-latency seeking mode, no buffering, special audio handling
                              ///  Use this if you want to make continuous seek requests
    };

    /// Decoder playing state
    enum PlayMode
    {
      PAUSE,                  ///< Media is playing
      PLAY                    ///< Media is paused
    };

    enum ErrorFlagsEnum
    {
      ERROR_VIDEO_FRAME_BUFFER_UNDERRUN = 1 << 0
    };
    typedef Radiant::FlagsT<ErrorFlagsEnum> ErrorFlags;

    /// Seeking request that can be sent to the decoder
    class SeekRequest
    {
    public:
      /// Construct a new request
      /// @param value media timestamp, value interpretation depends on selected SeekType
      /// @param type unit of the timestamp
      /// @param direction seek direction constraint
      SeekRequest(double value = 0.0, SeekType type = SEEK_NONE,
                  SeekDirection direction = SEEK_ANY_DIRECTION)
        : m_value(value), m_type(type), m_direction(direction) {}

      /// @returns media timestamp, value interpretation depends on selected SeekType
      double value() const { return m_value; }
      /// @param value seek timestamp
      void setValue(double value) { m_value = value; }

      /// @returns unit of the timestamp
      SeekType type() const { return m_type; }
      /// @param type unit of the timestamp
      void setType(SeekType type) { m_type = type; }

      /// @returns seek direction constraint
      SeekDirection direction() const { return m_direction; }
      /// @param direction seek direction constraint
      void setDirection(SeekDirection direction) { m_direction = direction; }

    private:
      double m_value;
      SeekType m_type;
      SeekDirection m_direction;
    };

    /// Video and audio parameters for AVDecoder when opening a new media file.
    class Options
    {
    public:
      /// Creates an empty parameter set. You must set at least source().
      Options()
        : m_channelLayout("downmix")
        , m_looping(false)
        , m_audioEnabled(true)
        , m_videoEnabled(true)
        , m_playMode(PAUSE)
        , m_videoStreamIndex(-1)
        , m_audioStreamIndex(-1)
        , m_audioBufferSeconds(2.0)
        , m_videoBufferFrames(10)
        , m_pixelFormat(VideoFrame::UNKNOWN)
        , m_videoDecodingThreads(2)
      {}

    public:
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
      /// List all available formats (demuxers): avconv -formats
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
      /// You need to have at least one track enabled (audio or video)
      /// Default: true
      /// @sa setAudioEnabled
      /// @return true if audio is enabled
      bool isAudioEnabled() const { return m_audioEnabled; }
      /// @sa isAudioEnabled
      /// @param audioEnabled should audio tracks be enabled
      void setAudioEnabled(bool audioEnabled) { m_audioEnabled = audioEnabled; }

      /// Should any video tracks be opened and decoded
      /// You need to have at least one track enabled (audio or video)
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

      /// Number of threads to use in the video decoder, value 0 selects the
      /// value automatically. Use value 0 carefully, it's optimal only when
      /// playing one video.
      int videoDecodingThreads() const { return m_videoDecodingThreads; }
      void setVideoDecodingThreads(int t) { m_videoDecodingThreads = t; }

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
      int m_videoDecodingThreads;
    };

  public:
    /// Deletes the decoder, blocks until all decoder threads have died
    virtual ~AVDecoder();

    /// Current decoder state
    /// @returns the current state of the decoder
    DecoderState & state();
    /// @copydoc state()
    const DecoderState & state() const;
    /// Checks if decoder has done running
    /// @returns true if the decoder in any of the final states (STATE_ERROR or STATE_FINISHED)
    bool finished() const;
    /// Has the decoder opened the media file correctly. When this is true,
    /// videoSize() will return valid size
    /// @returns true if the decoder is in STATE_HEADER_READY, STATE_READY or STATE_FINISHED
    bool isHeaderReady() const;
    /// Checks if there has been an unrecoverable error
    /// @return true if the decoder is in STATE_ERROR
    bool hasError() const;

    /// This decoder might require that the previous version of the decoder
    /// should be first deleted. Since it might block take a long time,
    /// this decoder is deleted in the child loop of the new decoder
    /// @param decoder old decoder that will deleted before this decoder is
    ///                started, assuming that there are no more references
    ///                to the old decoder outside this class.
    void setPreviousDecoder(std::shared_ptr<AVDecoder> decoder);

    /// Marks the decoder for shutting down, doesn't block
    virtual void close() = 0;

    /// @returns decoder current playing mode
    virtual PlayMode playMode() const = 0;
    /// @param mode new play mode
    virtual void setPlayMode(PlayMode mode) = 0;

    /// Schedules a seek. If the previous request is still waiting, new request will replace the old one
    /// @param req new seek request
    virtual void seek(const SeekRequest & req) = 0;
    /// Special mode for low-latency seeking without buffering. This should be
    /// used only with certain UI elements, where the seeking target
    /// might change in real-time.
    /// When in real-time seeking mode, the video acts like it's paused
    virtual bool realTimeSeeking() const = 0;
    /// @sa realTimeSeeking
    /// @param value true if real-time seeking is asked
    virtual void setRealTimeSeeking(bool value) = 0;

    /// Shorthand for making a relative seek request
    /// @param pos relative position, value should be between 0 and 1
    void seekRelative(double pos) { seek(SeekRequest(pos, SEEK_RELATIVE, SEEK_ANY_DIRECTION)); }
    /// Shorthand for making absolute seeking request
    /// @param seconds timestamp in seconds
    void seek(double seconds) { seek(SeekRequest(seconds, SEEK_BY_SECONDS, SEEK_ANY_DIRECTION)); }

    /// Decoded video resolution. Will return invalid Nimble::Size if
    /// isHeaderReady returns false, we are not decoding a video stream,
    /// or if video decoding is disabled.
    /// @returns video resolution
    virtual Nimble::Size videoSize() const = 0;

    /// @returns looping mode
    virtual bool isLooping() const = 0;
    /// @param doLoop new looping mode
    virtual void setLooping(bool doLoop) = 0;

    /// First this value is based on stream headers, but might be fine-tuned
    /// after the stream reaches to the end / maybe starts a new loop cycle.
    /// @returns media duration in seconds.
    virtual double duration() const = 0;

    /// Based on the current playback and audio state, converts an absolute
    /// wall-clock timestamp to video timestamp. This is most useful for
    /// video/audio synchronization
    /// @param ts real (wall-clock) timestamp
    /// @returns video timestamp
    virtual Timestamp getTimestampAt(const Radiant::TimeStamp & ts) const = 0;
    /// Decoder might have many frames in a buffer, this is the video timestamp
    /// of the latest decoded frame in that buffer.
    /// @returns video timestamp
    virtual Timestamp latestDecodedVideoTimestamp() const = 0;

    /// Gets a video frame from buffer that should be visible at the given timestamp.
    /// If this frame can't be found, then the closest frame will be used.
    /// @param ts video timestamp
    /// @param[out] errors ErrorFlags
    /// @returns decoded video frame from a buffer, or null if the buffer is empty
    virtual VideoFrame * getFrame(const Timestamp & ts, ErrorFlags & errors) const = 0;
    /// Deletes older video frames from the buffer, this needs to be called after
    /// the frame has been consumed, otherwise the buffer will fill quickly.
    /// @param ts timestamp of the previous consumed frame
    /// @param eof null or pointer to bool that will be set to true if the stream is at EOF
    virtual int releaseOldVideoFrames(const Timestamp & ts, bool * eof = nullptr) = 0;

    /// YUV to RGB conversion matrix using the active video color profile.
    /// This can be used directly in GLSL: vec4 rgb = m * vec4(y, u, v, 1.0);
    /// @returns YUV to RGB conversion matrix
    virtual Nimble::Matrix4f yuvMatrix() const = 0;

    /// Sets audio panning to specific location
    /// @param location 2D location of the audio
    virtual void panAudioTo(Nimble::Vector2f location) const = 0;

    /// Controls the gain (volume) of the video sound-track.
    /// @param gain new audio gain, typical range is 0-1, although larger values
    ///             can be used as well. Default value is 1.
    virtual void setAudioGain(float gain) = 0;

    /// Tries to minimize the audio playback latency by dropping extra samples
    /// from the buffer. Use this only with streaming sources, otherwise
    /// the audio playback will break.
    virtual void setMinimizeAudioLatency(bool minimize) = 0;

    /// Creates a new decoder and loads it with given options
    /// @param options options given to load()
    /// @param backend use empty string for automatic backend
    /// @returns new decoder
    static std::shared_ptr<AVDecoder> create(const Options & options,
                                             const QString & backend = "");

    virtual void audioTransferDeleted() {}

  protected:
    /// Constructs a new empty decoder, load() function will always be called after this
    AVDecoder();

    /// Initializes the decoder, but doesn't start the decoder thread.
    /// @param options opening options
    virtual void load(const Options & options) = 0;

    /// Run the actual decoder, called from the decoder thread
    virtual void runDecoder() = 0;

  private:
    virtual void childLoop() FINAL;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
  /// Smart pointer to AVDecoder
  typedef std::shared_ptr<AVDecoder> AVDecoderPtr;
}

#endif // VIDEODISPLAY_AVDECODER_HPP
