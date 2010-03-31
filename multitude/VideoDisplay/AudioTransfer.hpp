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

#ifndef VIDEODISPLAY_AUDIO_TRANSFER_HPP
#define VIDEODISPLAY_AUDIO_TRANSFER_HPP

#include <Radiant/IODefs.hpp>
#include <Radiant/TimeStamp.hpp>

#include <Resonant/Module.hpp>

#include <VideoDisplay/VideoIn.hpp>

namespace VideoDisplay {

  /** Transer sound stream from video input to audio output. 
      
      This class transfers sound data from the low-level video input
      thread to audio playback engine.

      AudioTransfer object are disposable, so they can be used only
      once.

      @see VideoIn
   */
  class AudioTransfer : public Resonant::Module
  {
  public:
    AudioTransfer(Resonant::Application *, VideoIn * video);
    virtual ~AudioTransfer();

    virtual bool prepare(int & channelsIn, int & channelsOut);
    virtual void process(float ** in, float ** out, int n);
    virtual bool stop();

    bool started() const { return m_started; }
    bool stopped() const { return m_stopped; }

    bool atEnd() const { return m_end; } 

    /// Returns the video frame that should be played right now.
    unsigned videoFrame();

    /// Forgets the video source object, and shuts down.
    void forgetVideo();

  private:

    static void deInterleave(float ** dest, const float * src,
			     int chans, int frames, int offset);
    static void zero(float ** dest,
		     int chans, int frames, int offset);

    void checkEnd(const VideoIn::Frame * f)
    {
      if(!f)
        m_ending = true;
      else if(f->m_absolute.secondsD() > m_video->durationSeconds() - 0.5f)
        m_ending = true;
    }

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
    int                m_sinceBase;
    bool      m_ending;
    bool      m_end;
    bool      m_first;
    double    m_audioLatency;
    Radiant::MutexAuto m_mutex;
  };

}

#endif

