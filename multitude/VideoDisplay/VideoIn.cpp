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

#include "VideoIn.hpp"

#include "AudioTransfer.hpp"

// #include <Radiant/PlatformUtils.hpp>
#include <Radiant/Sleep.hpp>
#include <Radiant/Trace.hpp>
#include <Radiant/VideoInput.hpp>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <map>

namespace VideoDisplay {

  using namespace Radiant;

  static Radiant::MutexStatic __countermutex;
  int    __framecount = 0;

  VideoIn::Frame::Frame()
      : m_audio(0),
      m_allocatedAudio(0),
      m_audioFrames(0),
      m_type(FRAME_INVALID)
  {
    int tmp = 0;
    {
      Radiant::GuardStatic g(__countermutex);
      __framecount++;
      tmp = __framecount;
    }
    debug("VideoIn::Frame::Frame # %p Instance count at %d", this, tmp);
  }

  VideoIn::Frame::~Frame()
  {
    int tmp = 0;
    {
      Radiant::GuardStatic g(__countermutex);
      __framecount--;
      tmp = __framecount;
    }
    debug("VideoIn::Frame::~Frame # %p Instance count at %d", this, tmp);
    m_image.freeMemory();
    if(m_audio)
      free(m_audio);
  }

  void VideoIn::Frame::copyAudio(const void * audio, int channels, int frames,
                                 Radiant::AudioSampleFormat format,
                                 Radiant::TimeStamp ts)
  {
    int n = frames * channels;

    if((m_allocatedAudio < n) ||
       (n < 10000 && m_allocatedAudio > 20000)) {
      /* If there is not enough space we need to allocate memory.
         If there is too muuch space we can free some memory. */
      debug("VideoIn::Frame::copyAudio # %d -> %d", m_allocatedAudio, n);

      free(m_audio);
      m_audio = (float *) malloc(n * sizeof(float));
      m_allocatedAudio = n;
  }

    if(format == Radiant::ASF_INT16) {
      const int16_t * au16 = (const int16_t *) audio;

      for(int i = 0; i < n; i++)
        m_audio[i] = au16[i] * (1.0f / (1 << 16));
    }

    m_audioFrames = frames;
    m_audioTS = ts;
  }

  void VideoIn::Frame::skipAudio(Radiant::TimeStamp amount,
                                 int channels, int samplerate)
  {
    debug("VideoIn::Frame::skipAudio # %lf %d %d", amount.secondsD(), channels,
         samplerate);

    if(amount <= 0)
      return;

    int takeframes = amount.secondsD() * samplerate;

    if(takeframes >= m_audioFrames) {
      m_audioFrames = 0;
      m_audioTS = 0;
      return;
    }
    int takesamples = takeframes * channels;
    int leftsamples = m_audioFrames * channels - takesamples;
    memmove(m_audio, m_audio + takesamples, leftsamples * sizeof(float));

    m_audioFrames -= takeframes;
    m_audioTS += amount;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  static std::map<std::string, VideoIn::VideoInfo> __infos;

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  int VideoIn::m_debug = 1;

  VideoIn::VideoIn()
      : m_decodedFrames(0),
      m_consumedFrames(0),
      m_consumedAuFrames(0),
      m_playing(false),

      m_flags(Radiant::WITH_VIDEO | Radiant::WITH_AUDIO),
      m_channels(0),
      m_sample_rate(44100),
      m_auformat(ASF_INT16),
      m_auBufferSize(0),
      m_auFrameBytes(0),
      m_continue(true),
      m_vmutex(false, false, false),
      m_amutex(false, false, false),
      m_fps(30.0),
      m_done(false),
      m_ending(false),
      m_decoding(true),
      m_atEnd(false),
      m_consumedRequests(0),
      m_queuedRequests(0),
      m_listener(0),
      m_mutex(false, false, false)
  {
    debug("VideoIn::VideoIn # %p", this);
  }

  VideoIn::~VideoIn()
  {
    debug("VideoIn::~VideoIn # %p", this);

    Guard g(mutex());

    if(m_listener)
      m_listener->forgetVideo();

    assert(!isRunning());

    /*
      The code below cannot be used to do anything. We are in the
      destructor, and virtual tables are long gone :-|

    if(isRunning()) {
      m_continue = false;
      m_vcond.wakeAll(m_vmutex);
      waitEnd();
    }
    */
  }

  /** Gets a frame from the video stream to be shown on the screen. */

  VideoIn::Frame * VideoIn::getFrame(int index, bool updateCount)
  {
    if(index < 0)
      return 0;

    /* Radiant::debug("VideoIn::nextFrame # dec = %u cons = %u",
          m_decodedFrames, m_consumedFrames);
    */
    if((int) m_decodedFrames <= index)
      index = m_decodedFrames - 1;

    if(!m_continue && m_decodedFrames <= m_consumedFrames)
      return 0;

    Frame * im = m_frames[index % m_frames.size()].ptr();

    if(!im)
      return 0;

    if(updateCount) {
      m_consumedFrames = index;

      // Signal that one frame was consumed:
      m_vcond.wakeAll();
    }
    else {
      m_consumedAuFrames = index;

      // Signal that one frame was consumed:
      m_vcond.wakeAll();
    }

    im->m_lastUse = Radiant::TimeStamp::getTime();
    m_displayFrameTime = im->m_absolute;

    return im;
  }

  bool VideoIn::init(const char * filename, Radiant::TimeStamp pos, int flags)
  {
    assert(!isRunning());

    m_finalFrames   = (uint) -1;
    m_displayFrameTime = 0;

    m_continue = true;

    m_flags = flags;

    bool ok = open(filename, pos);

    if(!ok) {
      error("VideoIn::start # Could not open file \"%s\"", filename);
      m_continue = false;
      return false;
    }

    run();

    return true;
  }

  bool VideoIn::play(Radiant::TimeStamp pos)
  {
    debug("VideoIn::play");

    if(pos < 0) {
      pos = m_displayFrameTime;
      if(m_atEnd)
        pos = 0;
    }

    pushRequest(Req(START, pos));

    return true;
  }

  void VideoIn::stop()
  {
    debug("VideoIn::stop");

    if(!m_continue && !isRunning())
      return;

    pushRequest(Req(STOP));
    // m_decoding = false;

    /* Wake up the decoder thread that might be (stuck) in the putFrame.*/
    if(m_decodedFrames > 4) {
      m_breakBack = true;
      m_vmutex.lock();

      while(m_consumedFrames < m_decodedFrames - 2
            /* &&m_consumedAuFrames < m_decodedFrames - 2*/) {
        // info("Walkback the decoding process");
        m_decodedFrames--;
      }
      m_vmutex.unlock();
      m_vcond.wakeAll();
    }

  }

  bool VideoIn::seek(Radiant::TimeStamp pos)
  {
    debug("VideoIn::seek");


    pushRequest(Req(SEEK, pos));

    return true;
  }

  void VideoIn::freeUnusedMemory()
  {

    pushRequest(Req(FREE_MEMORY));

  }

  bool VideoIn::atEnd()
  {
    if((m_consumedFrames >= m_finalFrames))
      return true;

    return false;
  }

  int VideoIn::selectFrame(int bottom, Radiant::TimeStamp time) const
  {

    if(bottom < 0)
      bottom = 0;
    int latest = latestFrame();

    int best = latest; // Nimble::Math::Min(latest, startfrom);
    int low = Nimble::Math::Min((int) m_consumedFrames,
                                (int) m_consumedAuFrames);
    if(low < bottom)
      low = bottom;

    // (int) (best - frameRingBufferSize() / 2));
    low = Nimble::Math::Max(low, 0);
    TimeStamp bestdiff = TimeStamp::createSecondsD(10000);

    double close = -1.0;

    for(int i = best; i >= low; i--) {
      const Frame * f = m_frames[i % m_frames.size()].ptr();

      if(!f)
        continue;

      if(f->m_type == FRAME_INVALID ||
         f->m_type == FRAME_IGNORE)
        break;

      TimeStamp diff = Nimble::Math::Abs(f->m_absolute - time);
      if(diff < bestdiff) {
        best = i;
        bestdiff = diff;
        close = f->m_absolute.secondsD();
      }
      else
        break;
    }

    debug("VideoIn::selectFrame # %d (%d %d) (%d %d) %lf %lf",
          best, low, latest, m_consumedFrames, m_consumedAuFrames,
          close, time.secondsD());

    return best;
  }

  void VideoIn::setDebug(int level)
  {
    m_debug = level;
  }

  void VideoIn::toggleDebug()
  {
    m_debug = !m_debug;
  }

  void VideoIn::setAudioListener(AudioTransfer * listener)
  {
    Radiant::Guard g(mutex());

    if(listener)
      assert(m_listener == 0);

    debug("VideoIn::setAudioListener # from %p to %p", m_listener, listener);
    m_listener = listener;
  }

  /** Before we get here the video stream should be opened successfully
      and the ring buffers should be allocated. */

  void VideoIn::childLoop()
  {
    debug("VideoIn::childLoop # ENTRY");

    while(m_continue) {

      m_requestMutex.lock();
      Req req;

      if(m_consumedRequests >= m_queuedRequests) {
        ;
      }
      else {
        req = m_requests[m_consumedRequests % REQUEST_QUEUE_SIZE];
        m_consumedRequests++;
      }
      m_requestMutex.unlock();

      if(req.m_request != NO_REQUEST && req.m_request != FREE_MEMORY)
        debug("VideoIn::childLoop # REQ = %d p = %d",
              (int) req.m_request, (int) playing());

      if(req.m_request == START) {
        m_decoding = true;
        m_atEnd = false;
        videoPlay(req.m_time);
        m_playing = true;
      }
      else if(req.m_request == STOP) {
        videoStop();
        m_playing = false;
      }
      else if(req.m_request == SEEK) {
        if(playing())
          videoPlay(req.m_time);
        else
          videoGetSnapshot(req.m_time);
      }
      else if(req.m_request == FREE_MEMORY) {
        freeFreeableMemory();
      }
      else if(playing())
        videoGetNextFrame();

      Radiant::Sleep::sleepMs(5);
    }

    m_frames.clear();

    debug("VideoIn::childLoop # EXIT");
  }


  VideoIn::Frame * VideoIn::putFrame(const Radiant::VideoImage * im,
                                     FrameType type,
                                     Radiant::TimeStamp show,
                                     Radiant::TimeStamp absolute,
                                     bool immediate)
  {
    assert(m_frames.size() != 0);

    m_vmutex.lock();

    if(immediate && false) {
      // Ignored.
      while((m_decodedFrames - 4) >= m_consumedFrames &&
            (m_decodedFrames - 4) >= m_consumedAuFrames)
        m_decodedFrames--;
    }

    while(((m_decodedFrames + 4) >= (m_consumedFrames + m_frames.size()) ||
           (m_decodedFrames + 4) >= (m_consumedAuFrames + m_frames.size())) &&
          m_continue)
      // m_vcond.wait(1000);
      m_vcond.wait(m_vmutex, 500);
    m_vmutex.unlock();

    if(!m_continue) {
      return 0;
    }

    Radiant::Guard g(mutex());

    RefPtr<Frame> & rf = m_frames[m_decodedFrames % m_frames.size()];

    if(!rf.ptr()) {
      rf = new Frame;
    }

    Frame & f = * rf.ptr();

    f.m_type = type;
    f.m_time = show;
    f.m_absolute = absolute;
    f.m_audioFrames = 0;
    // f.m_audio.clear();
    f.m_audioTS = 0;

    if(type == FRAME_SNAPSHOT)
      m_consumedAuFrames = m_decodedFrames;

    if(!f.m_image.m_planes[0].m_data) {
      f.m_image.allocateMemory(*im);
    }

    bool ok = f.m_image.copyData(*im);

    if(!ok)
      error("VideoIn::putFrame # Radiant::Image::copyData failed");

    m_decodedFrames++;

    // Signal that one frame was produced:
    m_vcond.wakeAll();

    if(m_debug)
      debug("VideoIn::putFrame # %p %u %u %lf",
            & f, m_decodedFrames, m_consumedFrames, absolute.secondsD());

    debug("VideoIn::putFrame # %d", m_decodedFrames);

    f.m_lastUse = Radiant::TimeStamp::getTime();

    return & f;
  }

  void VideoIn::ignorePreviousFrames()
  {
    debug("VideoIn::ignorePreviousFrames # %d", m_decodedFrames);
    for(uint i = m_consumedFrames; (i + 1) < m_decodedFrames; i++) {
      Frame * f = m_frames[i % m_frames.size()].ptr();
      if(f)
        f->m_type = FRAME_IGNORE;
    }
  }

  void VideoIn::freeFreeableMemory()
  {
    Radiant::Guard g(mutex());

    Radiant::TimeStamp limit = Radiant::TimeStamp::getTime() -
                               Radiant::TimeStamp::createSecondsI(10);

    for(unsigned i = 0; i < m_frames.size(); i++) {
      Frame * f = m_frames[i].ptr();

      if(f && (f->m_lastUse < limit)) {
        m_frames[i] = 0;
      }
    }
  }

  void VideoIn::pushRequest(const Req & r)
  {
    if(r.m_request != NO_REQUEST && r.m_request != FREE_MEMORY)
      debug("VideoIn::pushRequest # %d %lf", r.m_request, r.m_time.secondsD());

    Radiant::Guard g( & m_requestMutex);

    if(m_queuedRequests &&
       (m_queuedRequests > m_consumedRequests)) {
      Req & prev = m_requests[(m_queuedRequests-1) % REQUEST_QUEUE_SIZE];

      if(r.m_request == prev.m_request) {
        // override the previous request, so we do not spam the decoder.
        prev.m_time = r.m_time;
        return;
      }
    }

    m_requests[m_queuedRequests % REQUEST_QUEUE_SIZE] = r;
    m_queuedRequests++;
  }
}
