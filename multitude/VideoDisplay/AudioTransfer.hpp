/* COPYRIGHT
 */

#ifndef VIDEODISPLAY_AUDIO_TRANSFER_HPP
#define VIDEODISPLAY_AUDIO_TRANSFER_HPP

#include "Export.hpp"
#include "VideoIn.hpp"

#include <Radiant/IODefs.hpp>
#include <Radiant/TimeStamp.hpp>

#include <Resonant/Module.hpp>

namespace VideoDisplay {

  /** Transer sound stream from video input to audio output.

      This class transfers sound data from the low-level video input
      thread to audio playback engine.

      AudioTransfer object are disposable, so they can be used only
      once.

      @see VideoIn
   */
  class VIDEODISPLAY_API AudioTransfer : public Resonant::Module
  {
  public:
    /// Constructs an audio transfer object
    AudioTransfer(Resonant::Application *, VideoIn * video);
    virtual ~AudioTransfer();

    virtual bool prepare(int & channelsIn, int & channelsOut);
    virtual void process(float ** in, float ** out, int n, const Resonant::CallbackTime &);
    virtual bool stop();

    /// Returns true if the audio transfer has actually started
    bool started() const { return m_started; }
    /// Returns true if the audio transfer has been stopped
    bool stopped() const { return m_stopped; }

    /// Check if the file has reached its end
    bool atEnd() const { return m_end; }

    /// Returns the video frame that should be played right now.
    unsigned videoFrame();

    /// Forgets the video source object, and shuts down.
    void forgetVideo();

    /// Sets the gain factor for the sound-track
    void setGain(float gain) { m_gain = gain; }

  private:

    void deInterleave(float ** dest, const float * src,
                 int chans, int frames, int offset);
    static void zero(float ** dest,
             int chans, int frames, int offset);

    void checkEnd(const VideoIn::Frame * f);

    VideoIn * m_video;
    int       m_channels;
    bool      m_started;
    bool      m_stopped;
    Radiant::AudioSampleFormat m_sampleFmt;
    long      m_frames;

    int       m_videoFrame;
    int       m_showFrame;
    int       m_availAudio;
    int       m_total;
    Radiant::TimeStamp m_startTime;

    // Time stamp at the beginning of the current audio package.
    Radiant::TimeStamp m_baseTS;
    Radiant::TimeStamp m_timingBase;
    Radiant::TimeStamp m_showTime;
    int                m_sinceBase;
    bool      m_ending;
    bool      m_end;
    bool      m_first;
    double    m_audioLatency;
    float     m_gain;
    Radiant::Mutex m_mutex;
  };

}

#endif
