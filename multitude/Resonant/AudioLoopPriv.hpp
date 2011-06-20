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

#include <strings.h>

namespace Resonant {

  struct Stream {
    Stream() : stream(0), streamInfo(0), startTime(0) {
      bzero( &inParams,  sizeof(inParams));
      bzero( &outParams, sizeof(outParams));
    }
    PaStreamParameters inParams;
    PaStreamParameters outParams;
    PaStream * stream;
    const PaStreamInfo * streamInfo;
    PaTime startTime;
    Radiant::Semaphore * m_barrier;
  };
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

      Channels m_channels;
      std::vector<Stream> m_streams;
      std::list<std::pair<AudioLoop*, int> > cb;
      std::set<int> m_written;
      std::vector<void*> m_streamBuffers;

#ifdef RADIANT_WIN32
      static __declspec(thread) int s_currentStream;
#else
      static __thread int s_currentStream;
#endif
      Radiant::Semaphore m_sem;
  };
}

#endif // AUDIOLOOPPRIV_HPP
