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

#include "AudioTransfer.hpp"
#include "VideoIn.hpp"
#include "VideoDisplay.hpp"

#include <Radiant/PlatformUtils.hpp>
#include <Radiant/Trace.hpp>

#include <assert.h>
#include <stdlib.h>


namespace VideoDisplay {

  using namespace Radiant;

  static Radiant::Mutex __countermutex;
  int    __instancecount = 0;

  AudioTransfer::AudioTransfer(Resonant::Application * a, VideoIn * video)
      : Module(a),
      m_video(video),
      m_channels(0),
      m_started(false),
      m_stopped(false),
      m_sampleFmt(Radiant::ASF_INT16),
      m_frames(0),
      m_videoFrame(0),
      m_showFrame(-1),
      m_ending(false),
      m_end(false),
      m_audioLatency(0.0f),
      m_gain(1.0f),
      m_mutex(true)
  {
    const char * lat = getenv("RESONANT_LATENCY");
    if(lat) {
      double ms = atof(lat);
      debugVideoDisplay("Adjusted audio latency to %lf milliseconds", ms);
      m_audioLatency = ms * 0.001;
    }

    Radiant::Guard g2(m_mutex);

    if(m_video)
      m_video->setAudioListener(this);

    int tmp = 0;
    {
      Radiant::Guard g(__countermutex);
      __instancecount++;
      tmp = __instancecount;
    }
    debugVideoDisplay("AudioTransfer::AudioTransfer # %p Instance count at %d", this, tmp);

    m_timingBase = Radiant::TimeStamp::getTime();
  }

  AudioTransfer::~AudioTransfer()
  {
    int tmp = 0;
    {
      Radiant::Guard g(__countermutex);
      __instancecount--;
      tmp = __instancecount;
    }
    debugVideoDisplay("AudioTransfer::~AudioTransfer # %p Instance count at %d", this, tmp);
  }

  bool AudioTransfer::prepare(int & channelsIn, int & channelsOut)
  {
    debugVideoDisplay("AudioTransfer::prepare");

    Radiant::Guard g2(m_mutex);

    if(!m_video) {
      Radiant::error("AudioTransfer::prepare # No video source");
      m_stopped = true;
      return false;
    }

    Radiant::Guard g(m_video->mutex());

    m_channels = 0;
    m_sampleFmt = Radiant::ASF_INT16;
    int sr = 44100;

    m_video->getAudioParameters( & m_channels, & sr, & m_sampleFmt);

    debugVideoDisplay("AudioTransfer::prepare # chans = %d", m_channels);

    channelsIn = 0;
    channelsOut = m_channels;

    m_frames = 0;
    m_started = true;
    m_stopped = false;
    m_availAudio = 1000000;
    m_videoFrame = (int) m_video->latestFrame() + 1;

    if(m_videoFrame < 0)
      m_videoFrame = 0;

    m_video->getFrame(m_videoFrame - 1, false);

    m_baseTS = 0;
    m_sinceBase = 0;
    m_showFrame = -1;
    m_total = 0;
    m_ending = false;
    m_end = false;
    m_first = true;

    m_startTime = TimeStamp::getTime();
    m_timingBase = m_startTime;

    return true;
  }

  void AudioTransfer::process(float **, float ** out, int n, const Resonant::CallbackTime &)
  {
    Radiant::Guard g2(m_mutex);

    if(!m_video) {
      zero(out, m_channels, n, 0);
      return;
    }

    Radiant::Guard g(m_video->mutex());

    if(!m_video->isFrameAvailable(m_videoFrame)) {
      zero(out, m_channels, n, 0);

      if(m_ending && !m_end) {
        debugVideoDisplay("AudioTransfer::process # END detected.");
        m_end = true;
      }

      debugVideoDisplay("AudioTransfer::process # No frame %d", m_videoFrame);

      return;
    }

    debugVideoDisplay("AudioTransfer::process # %d %d %d %d",
                   m_channels, m_videoFrame, n, m_availAudio);

    const VideoIn::Frame * f = m_video->getFrame(m_videoFrame, false);

    checkEnd(f);

    if(!f)
      return;

    if(m_availAudio > f->m_audioFrames) {
      m_availAudio = f->m_audioFrames;
      debugVideoDisplay("AudioTransfer::process # taking audio %d %d",
                     m_availAudio, m_videoFrame);
      m_baseTS = f->m_audioTS; // - Radiant::TimeStamp::createSecondsD(f->m_audioFrames / 44100.0f);
    }

    int take  = Nimble::Math::Min(n, m_availAudio);
    int taken = 0;

    if(take) {

      int index = f->m_audioFrames - m_availAudio;

      const float * src = & f->m_audio[index * m_channels];

      deInterleave(out, src, m_channels, take, 0);
    }

    taken += take;
    m_availAudio -= take;
    n -= take;
    m_sinceBase += take;

    // Take new data from the next visual frame(s)
    while(n) {

      m_videoFrame++;

      debugVideoDisplay("AudioTransfer::process # To new frame %d", m_videoFrame);

      if(!m_video->isFrameAvailable(m_videoFrame)) {
        debugVideoDisplay("AudioTransfer::process # NOT ENOUGH DECODED : RETURN");
        m_availAudio = 1000000000;
        m_first = true;
        break;
      }

      f = m_video->getFrame(m_videoFrame, false);

      checkEnd(f);

      if(f->m_type == VideoIn::FRAME_IGNORE) {
        debugVideoDisplay("AudioTransfer::process # Ignoring one");
        continue;
      }

      m_availAudio = f->m_audioFrames;

      if(m_availAudio) {
        m_baseTS = f->m_audioTS; // - Radiant::TimeStamp::createSecondsD(f->m_audioFrames / 44100.0f);
        m_sinceBase = 0;
      }

      take = Nimble::Math::Min(n, m_availAudio);

      if(!take) {
        debugVideoDisplay("AudioTransfer::process # Jumping over frame");
        continue;
      }

      debugVideoDisplay("AudioTransfer::process # Got new i = %d a = %d %lf", m_videoFrame,
            m_availAudio, f->m_audioTS.secondsD());

      deInterleave(out, & f->m_audio[0], m_channels, take, taken);

      n -= take;
      m_availAudio -= take;
      taken += take;
      m_sinceBase += take;
    }

    m_total += taken;

    zero(out, m_channels, n, taken);

    m_showTime =
        m_baseTS + TimeStamp::createSecondsD(m_sinceBase / 44100.0 - m_audioLatency);

    m_showFrame = m_video->selectFrame(m_showFrame, m_showTime);

    m_timingBase = Radiant::TimeStamp::getTime();
    // if(m_videoFrame < m_showFrame)
    // m_videoFrame = m_showFrame;
    /*
    if(!taken) {
      for(int i = 0; i < m_channels; i++) {
    bzero(out[i], sizeof(float) * n);
      }
    }
    */
    debug("AudioTransfer::process # EXIT %d %d (%lf, %lf)",
          m_showFrame, m_total, m_showTime.secondsD(), m_baseTS.secondsD());
  }

  bool AudioTransfer::stop()
  {
    m_stopped = true;
    return true;
  }

  unsigned AudioTransfer::videoFrame()
  {
    Radiant::Guard g2(m_mutex);


    float dt = m_timingBase.sinceSecondsD();

    if(dt > 0.03f) {
      // info("AudioTransfer::videoFrame # adjusting for %f", dt);
      m_showFrame = m_video->selectFrame(m_showFrame,
                                         m_showTime + Radiant::TimeStamp::createSecondsD(dt));
    }

    debugVideoDisplay("AudioTransfer::videoFrame # %d", m_showFrame);
    return m_showFrame;
  }

  void AudioTransfer::forgetVideo()
  {
    debugVideoDisplay("AudioTransfer::forgetVideo");

    Radiant::Guard g2(m_mutex);

    m_video = 0;
    m_ending = true;
  }

  void AudioTransfer::deInterleave(float ** dest, const float * src,
                                   int chans, int frames, int offset)
  {
    assert(frames >= 0);

    debugVideoDisplay("AudioTransfer::deInterleave # %d %d %d", chans, frames, offset);

    const float gain = m_gain;

    for(int c = 0; c < chans; c++) {
      float * d = dest[c] + offset;
      const float * s = src + c;
      for(int f = 0; f < frames; f++) {
        *d = *s * gain;
        d++;
        s += chans;
      }
    }
  }

  void AudioTransfer::zero(float ** dest, int chans, int frames, int offset)
  {
    assert(frames >= 0);

    for(int c = 0; c < chans; c++) {
      float * d = dest[c] + offset;
      for(int f = 0; f < frames; f++) {
        *d++ = 0;
      }
    }
  }

  void AudioTransfer::checkEnd(const VideoIn::Frame * f)
  {
    if(!f) {
      m_ending = true;
      debugVideoDisplay("ShowGL::checkEnd # At end 1");
    }
    else {
      double runtime = m_video->runtimeSeconds();
      if(runtime > 0.6)
        runtime -= 0.5;
      if(f->m_absolute.secondsD() > runtime) {
        m_ending = true;
        debugVideoDisplay("ShowGL::checkEnd # At end 2 %lf %lf", f->m_absolute.secondsD(), runtime);
      }
    }
  }

}