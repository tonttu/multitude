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

#include "VideoInFFMPEG.hpp"
#include "VideoDisplay.hpp"

#include <Radiant/Trace.hpp>

#include <map>
#include <vector>
#include <algorithm>

namespace VideoDisplay {
  namespace
  {
    class FFVideodebug
    {
    public:
      FFVideodebug() : m_channels(0), m_duration(0.0f) {}

      Radiant::VideoImage m_firstFrame;
      int   m_channels;
      float m_duration;
      /* Can be used later on to drop frames out of memory selectively. */
      Radiant::TimeStamp m_used;
      Radiant::TimeStamp m_firstFrameTime;
    };

    /// how many videos to cache
    /// @see FFVideodebug
    const unsigned int s_MaxCached = 100;

    bool comp_ffvideodebug_timestamp(const std::pair<std::string, FFVideodebug> & a, const std::pair<std::string, FFVideodebug> & b)
    {
      return a.second.m_used < b.second.m_used;
    }
  }

  /* Here we cache the first frames off all viedos. */
  static std::map<std::string, FFVideodebug> __ffcache;

  static Radiant::Mutex __mutex;

  const FFVideodebug * __cacheddebugVideoDisplay(const std::string & filename)
  {
    Radiant::Guard g(__mutex);

    std::map<std::string, FFVideodebug>::iterator it = __ffcache.find(filename);

    if(it == __ffcache.end())
      return 0;

    (*it).second.m_used = Radiant::TimeStamp::getTime();

    return & (*it).second;
  }

  using namespace Radiant;

  VideoInFFMPEG::VideoInFFMPEG()
      : m_channels(0),
      m_sampleRate(44100),
      m_audioCount(0),
      m_auformat(ASF_INT16)

  {
    // m_audiobuf.resize(2 * 44100);
  }

  VideoInFFMPEG::~VideoInFFMPEG()
  {
    debugVideoDisplay("VideoInFFMPEG::~VideoInFFMPEG");
    if(isRunning()) {
      m_continue = false;
      m_vcond.wakeAll(m_vmutex);
      waitEnd();
    }
    debugVideoDisplay("VideoInFFMPEG::~VideoInFFMPEG # EXIT");
  }

  void VideoInFFMPEG::getAudioParameters(int * channels,
                                         int * sample_rate,
                                         AudioSampleFormat * format)
  {
    * channels = m_channels;
    * sample_rate = m_sample_rate;
    * format = m_auformat;
  }

  float VideoInFFMPEG::fps()
  {
    float fps = m_fps > 0.0f ? m_fps : m_video.fps();

    if(fps <= 1.0f)
      fps = 30.0f;

    while(fps > 100)
      fps = fps / 10.0f;

    return fps;
  }

  bool VideoInFFMPEG::open(const char * filename, Radiant::TimeStamp pos)
  {

    static const char * fname = "VideoInFFMPEG::open";

    m_name = filename;

    m_buffered = 0;
    m_audioCount = 0;

    const float bufferLengthInSeconds = 1.7f;

    // Try to find the video info from cache
    const FFVideodebug * vi = __cacheddebugVideoDisplay(filename);

    if(vi) {

      debugVideoDisplay("%s # %s using cached preview", fname, filename);

      m_duration = vi->m_duration;
      const VideoImage * img = & vi->m_firstFrame;

      m_info.m_videoFrameSize.make(img->m_width, img->m_height);

      m_frames.resize(bufferLengthInSeconds * fps());

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

    float fp = fps();

    m_duration = TimeStamp::createSecondsD(video.durationSeconds());

    debugVideoDisplay("%s # %f fps", fname, fp);

    m_frames.resize(bufferLengthInSeconds * fp);

    int channels, sample_rate;
    AudioSampleFormat fmt;

    video.getAudioParameters( & channels, & sample_rate, & fmt);

    putFrame(img, FRAME_SNAPSHOT, video.frameTime(), video.frameTime(), false);

    {
      // Cache the first frame for later use.
      Radiant::Guard g(__mutex);

      video.getAudioParameters( & m_channels, & m_sampleRate, & m_auformat);
      // remove the item with the smallest timestamp
      if (__ffcache.size() >= s_MaxCached) {
        __ffcache.erase(std::min_element(__ffcache.begin(), __ffcache.end(), comp_ffvideodebug_timestamp));
      }

      FFVideodebug & vi2 = __ffcache[filename];

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

    putFrame(img, FRAME_SNAPSHOT, 0, video.frameTime(), false);

    m_frameTime = video.frameTime();

    video.close();
  }

  void VideoInFFMPEG::videoPlay(Radiant::TimeStamp pos)
  {
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

      debugVideoDisplay("ideoInFFMPEG::videoPlay # Forward one frame");

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
    debugVideoDisplay("VideoInFFMPEG::videoStop");
    m_video.close();

  }

  double VideoInFFMPEG::durationSeconds()
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
    m_finalFrames = m_decodedFrames;
    m_playing = false;
    m_atEnd = true;
  }
}
