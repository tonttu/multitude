#include "PulseAudioSource.hpp"
#include "PulseAudioContext.hpp"
#include "AudioLoopPulseAudio.hpp"

#include <Radiant/Timer.hpp>

#include <pulse/pulseaudio.h>

namespace Resonant
{
  class PulseAudioSource::D
  {
  public:
    void capture(const float * data, std::size_t totalSamples);

    bool prepareContext(QString * errorMsg, double timeoutSecs);

  public:
    PulseAudioContextPtr m_context;
    pa_stream * m_stream = nullptr;

    ModuleBufferPlayerPtr m_module = std::make_shared<ModuleBufferPlayer>();
  };

  void PulseAudioSource::D::capture(const float * data, std::size_t totalSamples)
  {
    auto & buffers = m_module->buffers();
    const int channels = m_module->channelCount();
    int samples = totalSamples / channels;

    // Convert interleaved data to separate buffers
    for (int c = 0; c < channels; ++c) {
      int remaining = samples;
      const float * src = data + c;

      while (remaining > 0) {
        auto writer = buffers[c].write(remaining);
        if (writer.size() == 0) break;

        for (float * out = writer.data(), * end = out + writer.size(); out != end; ++out) {
          *out = *src;
          src += channels;
        }
        remaining -= writer.size();
      }

#ifdef BUFFER_WARNINGS
      if (remaining > 0 && c == 0) {
        Radiant::warning("PulseAudioSource::D::capture # Buffer overflow (%d samples)",
                         samples);
      }
#endif
    }
  }

  bool PulseAudioSource::D::prepareContext(QString * errorMsg, double timeoutSecs)
  {
    if (!m_context) {
      m_context = PulseAudioContext::create("Cornerstone PulseAudioSource");
      m_context->start();
    }

    if (!m_context->waitForReady(timeoutSecs)) {
      if (errorMsg) {
        *errorMsg = "Timeout when connection to PulseAudio daemon";
        return false;
      }
    }

    return true;
  }


  PulseAudioSource::PulseAudioSource()
    : m_d(new D())
  {
  }

  PulseAudioSource::~PulseAudioSource()
  {
    if (m_d) {
      close();
    }
  }

  PulseAudioSource::PulseAudioSource(PulseAudioSource && src)
    : m_d(std::move(src.m_d))
  {
  }

  PulseAudioSource & PulseAudioSource::operator=(PulseAudioSource && src)
  {
    std::swap(m_d, src.m_d);
    return *this;
  }

  bool PulseAudioSource::open(const QString & sourceName, QString * errorMsg,
                              double timeoutSecs, const QString & uiName)
  {
    Radiant::Timer timer;
    if (!m_d->prepareContext(errorMsg, timeoutSecs)) {
      return false;
    }

    PulseAudioContext::Lock lock(*m_d->m_context);

    /// @todo shouldn't hardcode channel count
    pa_sample_spec ss { PA_SAMPLE_FLOAT32, 44100, 2 };
    m_d->m_stream = pa_stream_new(m_d->m_context->paContext(),
                                  (uiName.isNull() ? "Capture " + sourceName : uiName).toUtf8().data(),
                                  &ss, nullptr);

    pa_stream_set_read_callback(m_d->m_stream, [] (pa_stream * s, size_t, void * ptr) {
      auto d = static_cast<PulseAudioSource::D*>(ptr);
      const void * data = nullptr;
      std::size_t bytes = 0;
      pa_stream_peek(s, &data, &bytes);
      if (data) {
        d->capture(static_cast<const float*>(data), bytes / sizeof(float));
        pa_stream_drop(s);
      }
    }, m_d.get());

    m_d->m_module->setChannelCount(ss.channels);

    pa_stream_connect_record(m_d->m_stream, sourceName.toUtf8().data(), nullptr, PA_STREAM_NOFLAGS);

    return true;
  }

  QList<SourceInfo> PulseAudioSource::sources(QString * errorMsg, double timeoutSecs)
  {
    QList<SourceInfo> srcs;

    Radiant::Timer timer;
    if (!m_d->prepareContext(errorMsg, timeoutSecs)) {
      return srcs;
    }

    auto op = m_d->m_context->listSources([&srcs] (const pa_source_info * info, bool eol) {
      if (eol) {
        return;
      }

      SourceInfo src;
      src.name = info->name;

      if (auto card = pa_proplist_gets(info->proplist, "alsa.card")) {
        src.alsaCard = QString(card).toInt();
      }
      if (auto name = pa_proplist_gets(info->proplist, "alsa.name")) {
        src.alsaName = name;
      }

      srcs << src;
    });

    if (!op->waitForFinished(timeoutSecs - timer.time())) {
      if (errorMsg) {
        *errorMsg = "listSources timed out";
      }
      op->cancel();
    }

    return srcs;
  }

  void PulseAudioSource::close()
  {
    if (m_d->m_stream) {
      PulseAudioContext::Lock lock(*m_d->m_context);
      pa_stream_disconnect(m_d->m_stream);
      pa_stream_unref(m_d->m_stream);
      m_d->m_stream = nullptr;
    }
    m_d->m_context.reset();
  }

  ModuleBufferPlayerPtr PulseAudioSource::module() const
  {
    return m_d->m_module;
  }
} // namespace Resonant
