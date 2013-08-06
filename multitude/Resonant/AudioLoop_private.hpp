/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef AUDIOLOOPPRIV_HPP
#define AUDIOLOOPPRIV_HPP

#include "AudioLoop.hpp"

#include <Radiant/Platform.hpp>
#include <Radiant/Mutex.hpp>
#include <Radiant/Semaphore.hpp>

#include <portaudio.h>

#include <vector>
#include <map>
#include <list>
#include <set>

/// @cond
namespace Resonant {

  struct Stream {
    Stream() : stream(0), streamInfo(0), startTime(0) {
      memset( &inParams,  0, sizeof(inParams));
      memset( &outParams, 0, sizeof(outParams));
    }
    PaStreamParameters inParams;
    PaStreamParameters outParams;
    PaStream * stream;
    const PaStreamInfo * streamInfo;
    PaTime startTime;
    Radiant::Semaphore * m_barrier;
  };
  // Channel c from device d
  struct Channel {
    Channel(int d = -1, int c = -1) : device(d), channel(c) {}
    int device;
    int channel;
  };
  typedef std::map<int, Channel> Channels;

  class AudioLoop::AudioLoopInternal
  {
    public:
      AudioLoopInternal() {}

      static int paCallback(const void *in, void *out,
          unsigned long framesPerBuffer,
          const PaStreamCallbackTimeInfo* time,
          PaStreamCallbackFlags status,
          void * self);

      static void paFinished(void * self);
      void collect(int stream, const void *in, void *out, unsigned long framesPerBuffer);

      /// Maps [input channel] -> [device, channel]
      Channels m_channels;
      std::vector<Stream> m_streams;

      typedef std::pair<AudioLoop*, int> Callback;
      typedef std::list<Callback> CallbackList;
      CallbackList cb;

      std::set<int> m_written;
      std::vector<void*> m_streamBuffers;

      Radiant::Semaphore m_sem;
  };
}
/// @endcond

#endif // AUDIOLOOPPRIV_HPP
