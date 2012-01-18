/* COPYRIGHT
 */

#include "VideoInFFMPEG.hpp"
#include "VideoDisplay.hpp"

#include <Radiant/Trace.hpp>
#include <Radiant/Mutex.hpp>

#include <map>
#include <string>
#include <algorithm>

namespace VideoDisplay {
  namespace
  {
    class VideoFirstFrame
    {
    public:
      VideoFirstFrame() : m_channels(0), m_duration(0.0f) {}

      Radiant::VideoImage m_firstFrame;
      int   m_channels;
      float m_duration;
      /* Can be used later on to drop frames out of memory selectively. */
      Radiant::TimeStamp m_used;
      Radiant::TimeStamp m_firstFrameTime;
    };

    /// how many videos to cache
    /// @see VideoFirstFrame
    const unsigned int s_MaxCached = 100;

    bool comp_ffvideodebug_timestamp(const std::pair<std::string, VideoFirstFrame> & a, const std::pair<std::string, VideoFirstFrame> & b)
    {
      return a.second.m_used < b.second.m_used;
    }
  }

  // Cache of the first frames of all loaded videos
  static std::map<std::string, VideoFirstFrame> s_firstFrameCache;
  // Mutex to guard access to s_firstFrameCache
  static Radiant::Mutex s_firstFrameCacheMutex;

  const VideoFirstFrame * __cacheddebugVideoDisplay(const std::string & filename)
  {
    Radiant::Guard g(s_firstFrameCacheMutex);

    std::map<std::string, VideoFirstFrame>::iterator it = s_firstFrameCache.find(filename);

    if(it == s_firstFrameCache.end())
      return 0;

    (*it).second.m_used = Radiant::TimeStamp::getTime();

    return & (*it).second;
  }

  using namespace Radiant;

  VideoInFFMPEG::VideoInFFMPEG()
      : m_channels(0),
      m_sampleRate(44100),
      m_audioCount(0),
      m_auformat(ASF_INT16),
      m_mutex(true)
  {
    // m_audiobuf.resize(2 * 44100);
  }

  VideoInFFMPEG::~VideoInFFMPEG()
  {
    debugVideoDisplay("VideoInFFMPEG::~VideoInFFMPEG");
    if(isRunning()) {
      {
        m_continue = false;
        Radiant::Guard g(m_mutex);
        m_vcond.wakeAll(m_vmutex);
      }
      waitEnd();
    }
    debugVideoDisplay("VideoInFFMPEG::~VideoInFFMPEG # EXIT");
  }

  void VideoInFFMPEG::getAudioParameters(int * channels,
                                         int * sample_rate,
                                         AudioSampleFormat * format) const
  {
    * channels = m_channels;
    * sample_rate = m_sample_rate;
    * format = m_auformat;
  }

  float VideoInFFMPEG::fps() const
  {
    float fps = m_fps > 0.0f ? m_fps : m_video.fps();

    if(fps <= 1.0f || fps >= 100.0f)
      fps = 0.0f;

    return fps;
  }

  bool VideoInFFMPEG::open(const char * filename, Radiant::TimeStamp pos)
  {
    Radiant::Guard g(m_mutex);

    static const char * fname = "VideoInFFMPEG::open";

    m_name = filename;

    m_buffered = 0;
    m_audioCount = 0;

    static float extralatency = 0.0f;
    static bool checked = false;

    if(!checked) {

      const char * lat = getenv("RESONANT_LATENCY");
      if(lat) {
        extralatency = atof(lat) * 0.001;
      }
      checked = true;
      debug("VideoInFFMPEG::open # Extra latenty set to %.3f", extralatency);
    }

    const float bufferLengthInSeconds = 1.7f + Nimble::Math::Clamp(extralatency * 1.5f, 0.0f, 5.0f);

    // Try to find the video info from cache
    const VideoFirstFrame * vi = __cacheddebugVideoDisplay(filename);

    if(vi) {

      debugVideoDisplay("%s # %s using cached preview", fname, filename);

      m_duration = vi->m_duration;

      if(m_flags & Radiant::DO_LOOP)
        m_runtime = Radiant::TimeStamp::createSecondsD(1.0e+9f);
      else
        m_runtime = m_duration;

      const VideoImage * img = & vi->m_firstFrame;

      m_info.m_videoFrameSize.make(img->m_width, img->m_height);

      //m_frames.resize(bufferLengthInSeconds * fps());
      // Assume 30fps as the video has not been opened yet
      m_frames.resize(bufferLengthInSeconds * 30.f);

      putFrame(img, FRAME_SNAPSHOT, 0, 0, false);

      m_channels = vi->m_channels;

      m_firstFrameTime = vi->m_firstFrameTime;

      return true;
    }

    debugVideoDisplay("%s # %s opening new file", fname, filename);

    Screenplay::VideoInputFFMPEG video;

    if(!video.open(filename, m_flags))
      return false;

    if(!video.hasVideoCodec()) {
      Radiant::error("%s # No video codec", fname);
      video.close();
      return false;
    }

    if(!video.hasAudioCodec()) {
      debugVideoDisplay("%s # No audio codec", fname);
      /* video.close();
     return false; */
    }
    pos = TimeStamp::createSecondsD(0.0);

    if(pos != 0) {
      debugVideoDisplay("%s # Doing a seek", fname);
      if(!video.seekPosition(pos.secondsD()))
        video.seekPosition(0);
    }

    const VideoImage * img = video.captureImage();

    if(!img)
      return false;

    if(!img->m_width)
      return false;

    m_info.m_videoFrameSize.make(img->m_width, img->m_height);

    float fp = video.fps();

    m_duration = TimeStamp::createSecondsD(video.durationSeconds());
    m_runtime = TimeStamp::createSecondsD(video.runtimeSeconds());

    debugVideoDisplay("%s # %f fps", fname, fp);

    m_frames.resize(bufferLengthInSeconds * fp);

    int channels, sample_rate;
    AudioSampleFormat fmt;

    video.getAudioParameters( & channels, & sample_rate, & fmt);

    putFrame(img, FRAME_SNAPSHOT, video.frameTime(), video.frameTime(), false);

    {
      // Cache the first frame for later use.
      Radiant::Guard g(s_firstFrameCacheMutex);

      video.getAudioParameters( & m_channels, & m_sampleRate, & m_auformat);
      // remove the item with the smallest timestamp
      if (s_firstFrameCache.size() >= s_MaxCached) {
        s_firstFrameCache.erase(std::min_element(s_firstFrameCache.begin(), s_firstFrameCache.end(), comp_ffvideodebug_timestamp));
      }

      VideoFirstFrame & vi2 = s_firstFrameCache[filename];

      vi2.m_duration = m_duration;
      vi2.m_firstFrame.allocateMemory(*img);
      vi2.m_firstFrame.copyData(*img);
      vi2.m_channels = m_channels;
      vi2.m_used = Radiant::TimeStamp::getTime();
      vi2.m_firstFrameTime = video.frameTime();

      m_firstFrameTime = video.frameTime();
    }

    video.close();

    debugVideoDisplay("%s # EXIT OK", fname);

    return true;
  }

  void VideoInFFMPEG::videoGetSnapshot(Radiant::TimeStamp pos)
  {
    debugVideoDisplay("VideoInFFMPEG::videoGetSnapshot # %lf", pos.secondsD());

    Screenplay::VideoInputFFMPEG video;

    if(!video.open(m_name.c_str(), m_flags)) {
      endOfFile();
      return;
    }

    if(pos)
      video.seekPosition(pos.secondsD());

    const VideoImage * img = video.captureImage();

    if(!img) {
      video.close();
      return;
    }

    Radiant::Guard g(m_mutex);

    putFrame(img, FRAME_SNAPSHOT, 0, video.frameTime(), false);
    m_frameTime = video.frameTime();

    video.close();
  }

  void VideoInFFMPEG::videoPlay(Radiant::TimeStamp pos)
  {
    Radiant::Guard g(m_mutex);

    //info("VideoInFFMPEG::videoPlay # %lf", pos.secondsD());

    if(!m_video.open(m_name.c_str(), m_flags)) {
      endOfFile();
      debugVideoDisplay("VideoInFFMPEG::videoPlay # Open failed for \"%s\"",
            m_name.c_str());
      return;
    }

    m_channels = 0;
    m_sampleRate = 44100;
    m_auformat = ASF_INT16;
    m_audioCount = 0;

    m_video.getAudioParameters( & m_channels, & m_sampleRate, & m_auformat);

    if(pos > 0) {

      if(pos.secondsD() >= (m_video.durationSeconds() - 2.5)) {
        ; // pos = 0;
      }
      else if(pos.secondsD() > 1.5) {
        m_video.seekPosition(TimeStamp(pos - Radiant::TimeStamp::createSecondsD(1.2)).
                             secondsD());
      }
    }

    const VideoImage * img = m_video.captureImage();
    m_frameTime = m_video.frameTime();

    int aframes = 0;
    const void * audio = m_video.captureAudio( & aframes);
    Radiant::TimeStamp audioTS = m_video.audioTime();

    if(!img) {
      debugVideoDisplay("VideoInFFMPEG::videoPlay # Image capture failed \"%s\"",
            m_name.c_str());
      endOfFile();
      return;
    }

    if(pos == 0) {

      Frame * f = putFrame(img, FRAME_STREAM, 0, m_video.frameTime(), true);

      if(aframes && f) {
        Radiant::Guard g(mutex());
        f->copyAudio(audio, m_channels, aframes, m_auformat, audioTS);
        m_audioCount = 1;
        ignorePreviousFrames();
      }
      return;
    }

    for(int tries = 0; tries < 100; tries++) {
      img = m_video.captureImage();
      m_frameTime = m_video.frameTime();

      if(!img) {
        debugVideoDisplay("VideoInFFMPEG::videoPlay # Image capture failed in scan \"%s\"",
              m_name.c_str());
        endOfFile();
        return;
      }

      int aframes2 = 0;
      const void * audio2 = m_video.captureAudio( & aframes2);
      Radiant::TimeStamp audioTS2 = m_video.audioTime();

      if(aframes2) {
        // Take pointers to the latest audio data.
        aframes = aframes2;
        audio = audio2;
        audioTS = audioTS2;
      }

      debugVideoDisplay("VideoInFFMPEG::videoPlay # Forward one frame");

      if(m_frameTime >= pos) {

        Frame * f = putFrame(img, FRAME_STREAM, 0, m_video.frameTime(), true);

        if(aframes && f) {
          Radiant::Guard g(mutex());
          f->copyAudio(audio, m_channels, aframes, m_auformat, audioTS);
          f->skipAudio(m_frameTime - audioTS, m_channels, 44100);
          m_audioCount = 1;
          ignorePreviousFrames();

          debugVideoDisplay("VideoInFFMPEG::videoPlay # EXIT OK %d %p", aframes, f);
        }

        return;
      }
    }

    endOfFile();
  }

  void VideoInFFMPEG::videoGetNextFrame()
  {
    Radiant::Guard g(m_mutex);

    debugVideoDisplay("VideoInFFMPEG::videoGetNextFrame");

    const VideoImage * img = m_video.captureImage();

    if(!img) {
      endOfFile();
      return;
    }

    TimeStamp vt = m_video.frameTime();

    m_frameDelta = m_frameTime.secsTo(vt);

    Frame * f = putFrame(img, FRAME_STREAM, vt + m_syncOffset, vt, false);

    int aframes = 0;
    const void * audio = m_video.captureAudio( & aframes);

    if(aframes && f) {
      f->copyAudio(audio, m_channels, aframes, m_auformat, m_video.audioTime());

      if(!m_audioCount) {
        ignorePreviousFrames();
      }

      m_audioCount++;
    }
    else if(f) {
      f->m_audioFrames = 0;
      f->m_audioTS = 0;
    }
    m_frameTime = vt;
  }

  void VideoInFFMPEG::videoStop()
  {
    Radiant::Guard g(m_mutex);

    debugVideoDisplay("VideoInFFMPEG::videoStop");
    m_video.close();
  }

  double VideoInFFMPEG::durationSeconds() const
  {
    return m_duration.secondsD();
  }

  double VideoInFFMPEG::runtimeSeconds() const
  {
    return m_duration.secondsD();
  }

  /*
  void VideoInFFMPEG::enableLooping(bool enable)
  {
    debugVideoDisplay("VideoInFFMPEG::enableLooping # %d", (int) enable);
    m_video.enableLooping(enable);
    m_duration = TimeStamp::createSecondsD(m_video.durationSeconds());
  }
  */

  void VideoInFFMPEG::endOfFile()
  {
    Radiant::Guard g(m_mutex);

    m_finalFrames = m_decodedFrames;
    m_playing = false;
    m_atEnd = true;
  }
}
