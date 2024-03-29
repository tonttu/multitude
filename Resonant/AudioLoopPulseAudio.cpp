/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "AudioLoopPulseAudio.hpp"
#include "DSPNetwork.hpp"
#include "ModuleOutCollect.hpp"

#include <pulse/pulseaudio.h>

#include <Radiant/Sleep.hpp>

namespace Resonant
{
  class AudioLoopPulseAudio::D
  {
  public:
    PulseAudioContextPtr m_context;
    pa_stream * m_outputStream = nullptr;

    DSPNetwork & m_dsp;
    std::shared_ptr<ModuleOutCollect> m_collect;

    int m_channelCount = 0;
    bool m_running = false;
    long m_onReadyListener = -1;
    bool m_underflow = false;

  public:
    D(DSPNetwork & dsp, const std::shared_ptr<ModuleOutCollect> & collect);

    void start(int samplerate, int channels);
    void stop();
    void callback(std::size_t foo);
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  AudioLoopPulseAudio::D::D(DSPNetwork & dsp, const std::shared_ptr<ModuleOutCollect> & collect)
    : m_dsp(dsp)
    , m_collect(collect)
  {
  }

  void AudioLoopPulseAudio::D::start(int samplerate, int channels)
  {
    stop();

    pa_sample_spec ss { PA_SAMPLE_FLOAT32, uint32_t(samplerate), uint8_t(channels) };
    pa_channel_map map;
    bool validMap = pa_channel_map_init_auto(&map, channels, PA_CHANNEL_MAP_DEFAULT);
    if (!validMap)
      validMap = pa_channel_map_init_auto(&map, channels, PA_CHANNEL_MAP_ALSA);
    if (!validMap) {
      map.channels = channels;
      map.map[0] = PA_CHANNEL_POSITION_FRONT_LEFT;
      map.map[1] = PA_CHANNEL_POSITION_FRONT_RIGHT;
      map.map[2] = PA_CHANNEL_POSITION_REAR_LEFT;
      map.map[3] = PA_CHANNEL_POSITION_REAR_RIGHT;
      map.map[4] = PA_CHANNEL_POSITION_FRONT_CENTER;
      map.map[5] = PA_CHANNEL_POSITION_LFE;
      map.map[6] = PA_CHANNEL_POSITION_SIDE_LEFT;
      map.map[7] = PA_CHANNEL_POSITION_SIDE_RIGHT;
      for (unsigned int i = 8; i < PA_CHANNELS_MAX; ++i)
        map.map[i] = pa_channel_position(PA_CHANNEL_POSITION_AUX0 + i);
    }

    m_outputStream = pa_stream_new(m_context->paContext(), "Cornerstone AudioLoop",
                                   &ss, &map);

    if (!m_outputStream) {
      Radiant::error("Failed to open PulseAudio stream with %d channels: %s", channels,
                     pa_strerror(pa_context_errno(m_context->paContext())));
      return;
    }

    m_underflow = false;
    pa_stream_set_write_callback(m_outputStream, [] (pa_stream *, std::size_t bytes, void * ptr) {
      auto d = static_cast<AudioLoopPulseAudio::D*>(ptr);
      d->callback(bytes);
    }, this);

    pa_stream_set_underflow_callback(m_outputStream, [] (pa_stream *, void * ptr) {
      auto d = static_cast<AudioLoopPulseAudio::D*>(ptr);
      d->m_underflow = true;
    }, this);

    m_channelCount = channels;

    pa_buffer_attr attr;
    attr.maxlength = Resonant::Module::MAX_CYCLE * sizeof(float) * channels;
    attr.tlength = Resonant::Module::MAX_CYCLE * sizeof(float) * channels;
    attr.prebuf = -1;
    attr.minreq = -1;
    attr.fragsize = -1;

    pa_cvolume volume;
    volume.channels = channels;
    for (int c = 0; c < channels; ++c)
      volume.values[c] = PA_VOLUME_NORM;

    auto flags = pa_stream_flags_t(PA_STREAM_INTERPOLATE_TIMING | PA_STREAM_AUTO_TIMING_UPDATE |
        PA_STREAM_ADJUST_LATENCY | PA_STREAM_START_UNMUTED);

    pa_stream_connect_playback(m_outputStream, nullptr, &attr, flags, &volume, nullptr);
  }

  void AudioLoopPulseAudio::D::stop()
  {
    if (m_outputStream) {
      pa_stream_disconnect(m_outputStream);
      // pa_stream_disconnect is asynchronous, wait up to 1 second for streams
      // to finish. Without this we might delete the context while some
      // callbacks are still running.
      const int maxWaitMs = 1000;
      const int steps = 10;
      const int sleepMs = maxWaitMs / (steps*10/2);

      for (int i = 0; i < steps; ++i) {
        pa_stream_state_t state = pa_stream_get_state(m_outputStream);
        if (state == PA_STREAM_FAILED || state == PA_STREAM_TERMINATED) {
          break;
        }
        Radiant::Sleep::sleepMs(i*sleepMs);
      }
      pa_stream_unref(m_outputStream);
      m_outputStream = nullptr;
    }
  }

  void AudioLoopPulseAudio::D::callback(std::size_t bytes)
  {
    Radiant::TimeStamp time = Radiant::TimeStamp::currentTime();
    double latency = 0.03;

    pa_usec_t streamLatency;
    int neg;
    if (pa_stream_get_latency(m_outputStream, &streamLatency, &neg) == PA_OK)
      latency = streamLatency / 1000000.0;
    time += Radiant::TimeStamp::createSeconds(latency);

    float * buffer = nullptr;
    pa_stream_begin_write(m_outputStream, reinterpret_cast<void**>(&buffer), &bytes);
    int frames = bytes / sizeof(float) / m_channelCount;

    m_collect->setInterleavedBuffer(buffer);
    CallbackTime::CallbackFlags flags;
    if (m_underflow) {
      m_underflow = false;
      flags |= CallbackTime::FLAG_BUFFER_UNDERFLOW;
    }
    m_dsp.doCycle(frames, CallbackTime(time, latency, flags));
    m_collect->setInterleavedBuffer(nullptr);

    pa_stream_write(m_outputStream, buffer, bytes, nullptr, 0l, PA_SEEK_RELATIVE);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  AudioLoopPulseAudio::AudioLoopPulseAudio(DSPNetwork & dsp, const std::shared_ptr<ModuleOutCollect> & collect)
    : m_d(new D(dsp, collect))
  {
  }

  AudioLoopPulseAudio::~AudioLoopPulseAudio()
  {
    stop();
  }

  bool AudioLoopPulseAudio::start(int samplerate, int channels)
  {
    m_d->m_context = PulseAudioContext::create("Cornerstone AudioLoop");
    m_d->m_context->start();

    std::weak_ptr<D> weak(m_d);
    m_d->m_onReadyListener = m_d->m_context->onReady([weak, samplerate, channels] {
      if (auto d = weak.lock()) {
        d->start(samplerate, channels);
      }
    });

    m_d->m_running = true;
    return true;
  }

  bool AudioLoopPulseAudio::stop()
  {
    m_d->stop();
    if (m_d->m_context) {
      m_d->m_context->removeOnReadyListener(m_d->m_onReadyListener);
      m_d->m_context.reset();
    }
    m_d->m_running = false;
    return true;
  }

  bool AudioLoopPulseAudio::isRunning() const
  {
    return m_d->m_running;
  }

  std::size_t AudioLoopPulseAudio::outChannels() const
  {
    return m_d->m_channelCount;
  }

} // namespace Resonant
