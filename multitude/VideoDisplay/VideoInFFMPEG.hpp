/* COPYRIGHT
 */


#ifndef VIDEODISPLAY_VIDEO_IN_FFMPEG_HPP
#define VIDEODISPLAY_VIDEO_IN_FFMPEG_HPP

#if 0
#include "VideoIn.hpp"

#include <Screenplay/VideoFFMPEG.hpp>

namespace VideoDisplay {

  /// Movie file decoder, that uses the Screenplay::VideoInputFFMPEG
  class VIDEODISPLAY_API VideoInFFMPEG : public VideoIn
  {
  public:
    VideoInFFMPEG();
    virtual ~VideoInFFMPEG();

    /// Gets the audio parameters of the movie
    virtual void getAudioParameters(int * channels,
                    int * sample_rate,
                    Radiant::AudioSampleFormat * format) const OVERRIDE;
    /// Returns the nominal fps of the movie
    virtual float fps() const OVERRIDE;

    virtual double durationSeconds() const OVERRIDE;
    virtual double runtimeSeconds() const OVERRIDE;

    /// Seek to some time in the movie
    // virtual bool seekTo(double seconds);

  private:

    virtual bool open(const char * filename, Radiant::TimeStamp pos);

    virtual void videoGetSnapshot(Radiant::TimeStamp pos);
    virtual void videoPlay(Radiant::TimeStamp pos);
    virtual void videoGetNextFrame();
    virtual void videoStop();

    void doSeek(const Radiant::VideoImage * im, const void * audio,
        int audioframes);
    // void needResync();
    // void doSync(int aframes, Radiant::TimeStamp vt);
    void endOfFile();

    int m_needSync;
    int m_gotSync;
    Radiant::TimeStamp m_syncAccu;
    Radiant::TimeStamp m_syncOffset;

    Radiant::TimeStamp m_duration;
    Radiant::TimeStamp m_runtime;
    double m_frameDelta;

    Screenplay::VideoInputFFMPEG m_video;

    int m_buffered;

    int m_channels;
    int m_sampleRate;
    int m_audioCount;
    Radiant::AudioSampleFormat m_auformat;

    // Protect all critical variables in the class
    Radiant::Mutex m_mutex;
  };

}
#endif

#endif
