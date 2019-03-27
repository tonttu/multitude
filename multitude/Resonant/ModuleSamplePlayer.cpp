/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "ModuleSamplePlayer.hpp"

#include "ModulePanner.hpp"
#include "Resonant.hpp"

#include "DSPNetwork.hpp"
#include "AudioFileHandler.hpp"

#include <Nimble/Math.hpp>

#include <Radiant/BinaryData.hpp>
#include <Radiant/Directory.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

#include <sndfile.h>
#include <cassert>

namespace Resonant {


  ModuleSamplePlayer::NoteInfoInternal::NoteInfoInternal()
    : m_status(NOTE_PLAYING)
    , m_noteId(-1)
    , m_sampleLengthSeconds(0.0f)
    , m_playHeadPosition(0.0f)
  {
  }


  ModuleSamplePlayer::NoteInfo::NoteInfo()
  {}

  ModuleSamplePlayer::NoteInfo::~NoteInfo()
  {}

  ModuleSamplePlayer::NoteStatus ModuleSamplePlayer::NoteInfo::status() const
  {
    if(m_info)
      return m_info->m_status;

    return NOTE_FINISHED;
  }

  int ModuleSamplePlayer::NoteInfo::noteId() const
  {
    if(m_info)
      return m_info->m_noteId;

    return -1;
  }

  float ModuleSamplePlayer::NoteInfo::sampleLengthSeconds() const
  {
    if(m_info)
      return m_info->m_sampleLengthSeconds;

    return 0.0f;
  }

  float ModuleSamplePlayer::NoteInfo::playHeadSeconds() const
  {
    if(m_info)
      return m_info->m_playHeadPosition;

    return 0.0f;
  }

  void ModuleSamplePlayer::NoteInfo::init(int id)
  {
    if(!m_info)
      m_info = NoteInfoInternalPtr(new NoteInfoInternal());

    m_info->m_noteId = id;
    m_info->m_status = NOTE_PLAYING;
  }

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  QString ModuleSamplePlayer::NoteParameters::fileName() const
  {
    return m_fileName;
  }

  void ModuleSamplePlayer::NoteParameters::setFileName(const QString &fileName)
  {
    m_fileName = fileName;
  }

  float ModuleSamplePlayer::NoteParameters::samplePlayhead() const
  {
    return m_samplePlayhead;
  }

  void ModuleSamplePlayer::NoteParameters::setSamplePlayhead(float samplePlayhead)
  {
    m_samplePlayhead = samplePlayhead;
  }

  Radiant::TimeStamp ModuleSamplePlayer::NoteParameters::playbackTime() const
  {
    return m_playbackTime;
  }

  void ModuleSamplePlayer::NoteParameters::setPlaybackTime(const Radiant::TimeStamp &playbackTime)
  {
    m_playbackTime = playbackTime;
  }

  bool ModuleSamplePlayer::NoteParameters::loop() const
  {
    return m_loop;
  }

  void ModuleSamplePlayer::NoteParameters::setLoop(bool loop)
  {
    m_loop = loop;
  }

  int ModuleSamplePlayer::NoteParameters::sampleChannel() const
  {
    return m_sampleChannel;
  }

  void ModuleSamplePlayer::NoteParameters::setSampleChannel(int sampleChannel)
  {
    m_sampleChannel = sampleChannel;
  }

  int ModuleSamplePlayer::NoteParameters::targetChannel() const
  {
    return m_targetChannel;
  }

  void ModuleSamplePlayer::NoteParameters::setTargetChannel(int targetChanggel)
  {
    m_targetChannel = targetChanggel;
  }

  float ModuleSamplePlayer::NoteParameters::relativePitch() const
  {
    return m_relativePitch;
  }

  void ModuleSamplePlayer::NoteParameters::setRelativePitch(float relativePitch)
  {
    m_relativePitch = relativePitch;
  }

  float ModuleSamplePlayer::NoteParameters::gain() const
  {
    return m_gain;
  }

  void ModuleSamplePlayer::NoteParameters::setGain(float gain)
  {
    m_gain = gain;
  }

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////

  class ModuleSamplePlayer::Sample::Internal
  {
  public:
    Internal()
    { memset( & m_info, 0, sizeof(m_info)); };

    SF_INFO m_info;
  };

  ModuleSamplePlayer::Sample::Sample()
  {
    //info("ModuleSamplePlayer::Sample::Sample # %p", this);
    m_d = new Internal();
  }

  ModuleSamplePlayer::Sample::~Sample()
  {
    //info("ModuleSamplePlayer::Sample::~Sample # %p", this);

    delete m_d;
  }

  const float * ModuleSamplePlayer::Sample::buf(unsigned i) const
  {
    return & m_data[i * m_d->m_info.channels];
  }

  bool ModuleSamplePlayer::Sample::load(const char * filename,
                                        const char * name)
  {
    if(!Radiant::FileUtils::fileReadable(filename))
      return false;

    m_name = name;

    memset(&m_d->m_info, 0, sizeof(m_d->m_info));

    SNDFILE * sndf = AudioFileHandler::open(filename, SFM_READ, & m_d->m_info);

    if(!sndf)
      return false;

    m_data.resize(m_d->m_info.channels * m_d->m_info.frames);
    if(!m_data.empty())
      memset( & m_data[0], 0, m_data.size() * sizeof(float));

    size_t block = 1000;

    size_t pos = 0;

    while(pos < static_cast<size_t> (m_d->m_info.frames)) {
      size_t get = std::min((size_t) (m_d->m_info.frames - pos), block);
      size_t n = get * m_d->m_info.channels;

      sf_read_float(sndf, & m_data[pos * m_d->m_info.channels], n);

      pos += get;
    }

    sf_close(sndf);

    Radiant::debug
        ("ModuleSamplePlayer::Sample::load # %s from %s %d frames %d channels",
         filename, name, (int) m_d->m_info.frames, (int) m_d->m_info.channels);

    return true;
  }

  unsigned ModuleSamplePlayer::Sample::available(unsigned pos) const
  {
    // pos /=m_d->m_info.channels;
    return (pos < m_d->m_info.frames) ? m_d->m_info.frames - pos : 0;
  }

  unsigned ModuleSamplePlayer::Sample::channels() const
  {
    return m_d->m_info.channels;
  }

  unsigned ModuleSamplePlayer::Sample::frames() const
  {
    return m_d->m_info.frames;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  bool ModuleSamplePlayer::SampleVoice::synthesize(float ** out, int n, ModuleSamplePlayer * host)
  {
    if(m_startTime > host->time()) {
      return true;
    } else if(m_state != PLAYING) {
      // printf(":"); fflush(0);
      return m_state == WAITING_FOR_SAMPLE;
    }

    unsigned avail = m_sample->available(m_position);

    if((int) avail > n)
      avail = n;

    // Radiant::info("avail = %u", avail);

    if(m_targetChannel >= host->channels()) {
      Radiant::error("ModuleSamplePlayer::SampleVoice::synthesize # channel count exceeded for %s "
                     "%ld >= %ld", m_sample->name().toUtf8().data(),
                     m_targetChannel, host->channels());
      m_state = INACTIVE;
      return false;
    }

    float * b1 = out[m_targetChannel];
    Nimble::Rampd gain = m_gain;
    Nimble::Rampd pitch = m_relPitch;

    bool more;

    int chans = m_sample->channels();
    int sampleChannel = chans == 1 ? 0 : m_sampleChannel;
    float onePerChans = 1.f / chans;

    if(avail == 0) {
      more = false;
    }
    else if(pitch == 1.0f && !pitch.left()) {
      // downmix all channels to mono
      if (sampleChannel == -1) {
        const float * src = m_sample->buf(m_position);

        for (unsigned i = 0; i < avail; i++) {
          float mix = 0;
          for (const float * end = src + chans; src < end; ++src)
            mix += *src;
          *b1++ += mix * onePerChans * gain;
          gain.update();
        }
      } else {
        const float * src = m_sample->buf(m_position) + sampleChannel;

        for(unsigned i = 0; i < avail; i++) {
          *b1++ += *src * gain;
          src += chans;
          gain.update();
        }
      }

      m_position += avail;
      m_dpos = m_position;

      more = (int) avail == n;
    }
    else {
      double dpos = m_dpos;
      double dmax = m_sample->frames() - 1;

      if (sampleChannel == -1) {
        const float * src = m_sample->buf(0);

        for (int i = 0 ; i < n && dpos < dmax; i++) {
          int base = (int) dpos;
          double w2 = dpos - (double) base;

          float mixa = 0.f, mixb = 0.f;
          for (int ch = 0; ch < chans; ++ch) {
            mixa += src[base * chans + ch];
            mixb += src[(base+1) * chans + ch];
          }

          *b1++ += gain * onePerChans *
              float((mixa * (1.0 - w2) + mixb * w2));
          dpos += pitch;
          gain.update();
          pitch.update();
        }
      } else {
        const float * src = m_sample->buf(0) + sampleChannel;

        for(int i = 0 ; i < n && dpos < dmax; i++) {

          int base = (int) dpos;
          double w2 = dpos - (double) base;
          *b1++ += gain *
              float((src[base * chans] * (1.0 - w2) + src[(base+1) * chans] * w2));
          dpos += pitch;
          gain.update();
          pitch.update();
        }
      }

      m_dpos = dpos;
      m_position = dpos;

      m_relPitch = pitch;
      more = dpos < dmax;
    }

    m_gain = gain;

    if(m_finishCounter > 0 && (m_finishCounter - n) <= 0) {

      if(m_autoRestartAfterStop) {
        m_finishCounter = -1;
        m_autoRestartAfterStop = false;
        m_position = m_startPosition;
        m_dpos = m_position;
        m_gain = 0.0f;
        m_gain.setTarget(m_startGain, m_startFadeInDurationSamples);
        // Radiant::info("ModuleSamplePlayer::SampleVoice::synthesize # Restart");
      }
      else {

        more = 0;
        // m_loop = false;
      }
    }

    m_finishCounter -= n;

    if(!more) {
      if(m_loop && !m_stopped) {
        // debugResonant("ModuleSamplePlayer::SampleVoice::synthesize # rewind");
        m_position = 0;
        m_dpos = 0.0f;
        more = 1;
      }
      else {
        // debugResonant("ModuleSamplePlayer::SampleVoice::synthesize # done");
        m_sample.reset();
        m_state = INACTIVE;
      }
    }

    if(m_info) {
      m_info->m_playHeadPosition = m_dpos / 44100.0;
      /*
      Radiant::info("Playhead at %d %f %lf %d", (int) m_position, m_info->m_playHeadPosition, m_dpos,
                    (int) m_sample ? m_sample->frames() : 0u);
                    */
    }

    return more != 0;
  }

  void ModuleSamplePlayer::SampleVoice::init
  (ModuleSamplePlayer * host, std::shared_ptr<Sample> sample, Radiant::BinaryData &data)
  {
    m_sample = sample;
    m_position = 0;

    m_gain = 1.0f;
    m_relPitch = 1.0f;
    m_sampleChannel = 0;
    m_targetChannel = 0;
    m_dpos = 0.0f;
    m_noteId = 0;
    m_loop = false;

    const int buflen = 64;
    char name[buflen + 1] = { '\0' };

    if(!data.readString(name, buflen)) {
      Radiant::error("ModuleSamplePlayer::SampleVoice::init # Invalid beginning");

      return;
    }

    while(name[0] != '\0' && strcmp(name, "end") != 0) {

      bool ok = true;

      if(strcmp(name, "gain") == 0)
        m_gain = data.readFloat32( & ok);
      else if(strcmp(name, "relpitch") == 0)
        m_relPitch = data.readFloat32( & ok);
      else if(strcmp(name, "samplechannel") == 0)
        m_sampleChannel = data.readInt32( & ok);
      else if(strcmp(name, "targetchannel") == 0)
        m_targetChannel = data.readInt32( & ok);
      else if(strcmp(name, "location") == 0) {
        Nimble::Vector2 loc = data.readVector2Float32( & ok);
        if(ok)
          m_targetChannel = host->locationToChannel(loc);
      }
      else if(strcmp(name, "loop") == 0)
        m_loop = (data.readInt32( & ok) != 0);
      else if(strcmp(name, "time") == 0)
        m_startTime = data.readTimeStamp( & ok);
      else if(strcmp(name, "note-id") == 0)
        m_noteId = data.readInt32(& ok);
      else if(strcmp(name, "playhead-seconds") == 0) {
        float seconds = data.readFloat32( & ok);
        m_position = seconds * 44100.0;
        m_dpos = m_position;
      }
      else {
        Radiant::error("ModuleSamplePlayer::SampleVoice::init # Invalid parameter \"%s\"",
                       name);
        break;
      }

      if(!ok) {
        Radiant::error("ModuleSamplePlayer::SampleVoice::init # Error parsing value for %s",
                       name);
      }
      else {
        debugResonant("ModuleSamplePlayer::SampleVoice::init # got %s", name);
      }

      name[0] = '\0';
      if(!data.readString(name, buflen)) {
        Radiant::error("ModuleSamplePlayer::SampleVoice::init # Error reading parameter");
        break;
      }
    }

    if(m_noteId > 0) {
      Radiant::Guard g(host->m_mutex);
      auto it = host->m_infos.find(m_noteId);
      if(it != host->m_infos.end()) {
        m_info = it->second;
      }
      else {
        m_info = NoteInfoInternalPtr(new NoteInfoInternal());
        m_info->m_noteId = m_noteId;
        host->m_infos[m_noteId] = m_info;
      }
    }

    if(m_startTime > host->time())
      m_state = WAITING_FOR_SAMPLE;
    else
      m_state = sample ? PLAYING : WAITING_FOR_SAMPLE;

    if(m_info) {
      m_info->m_playHeadPosition = m_dpos;
      if(sample)
        m_info->m_sampleLengthSeconds = sample->frames() / 44100.0f;
    }

    m_startGain = m_gain;

    debugResonant("ModuleSamplePlayer::SampleVoice::init # %p Playing gain = %.3f "
                  "rp = %.3f, ss = %ld, ts = %ld", this, m_gain.value(), m_relPitch.value(),
                  m_sampleChannel, m_targetChannel);
  }

  void ModuleSamplePlayer::SampleVoice::processMessage(ModuleSamplePlayer * /*host*/, const QByteArray &parameter,
                                                       Radiant::BinaryData &data)
  {
    // Radiant::info("ModuleSamplePlayer::SampleVoice::processMessage # %s", parameter.data());

    if(parameter == "control") {

      const int buflen = 64;
      char name[buflen + 1] = { '\0' };

      if(!data.readString(name, buflen)) {
        Radiant::error("ModuleSamplePlayer::SampleVoice::processMessage # Invalid beginning");

        return;
      }

      float gain = -1.0f;
      float interpolationTimeSeconds = -1.0f;
      float relPitch = -1.0f;
      float playheadSeconds = -1.0f;

      int loop = -1;

      bool ok = true;

      while(name[0] != '\0' && strcmp(name, "end") != 0 && ok) {

        if(strcmp(name, "gain") == 0) {
          gain = data.readFloat32( & ok);
        }
        else if(strcmp(name, "relative-pitch") == 0)
          relPitch = data.readFloat32( & ok);
        else if(strcmp(name, "interpolation-time") == 0)
          interpolationTimeSeconds = data.readFloat32( & ok);
        else if(strcmp(name, "loop") == 0)
          loop = (data.readInt32( & ok) != 0);
        else if(strcmp(name, "playhead-seconds") == 0)
          playheadSeconds = data.readFloat32( & ok);
        else {
          ok = false;
        }

        if(!ok)
          Radiant::error("ModuleSamplePlayer::SampleVoice::processMessage # Control # Invalid parameter \"%s\"",
                         name);
        else
          ok = data.readString(name, buflen);
      }

      /*
      Radiant::info("ModuleSamplePlayer::SampleVoice::processMessage # Control %s %f %f %f %f %d",
                    ok ? "OK" : "FAIL", gain, relPitch, interpolationTimeSeconds, playheadSeconds,
                    (int) loop);
      */
      if(!ok)
        return;

      if(interpolationTimeSeconds < 0.0f)
        interpolationTimeSeconds = 0.01f;

      /// @todo Do not hard-code the sample-rate
      float sampleRate = 44100.0f;

      int interpolationSamples = interpolationTimeSeconds * sampleRate;

      if(gain >= 0.0f) {
        m_gain.setTarget(gain, interpolationSamples);
        m_startGain = gain;
      }
      if(relPitch >= 0.0f) {
        m_relPitch.setTarget(relPitch, interpolationSamples);
      }
      if(loop >= 0) {
        m_loop = loop != 0;
      }
      if(playheadSeconds >= 0.0f) {

        // Half of the time goes to fade-out, the other half to fade-in
        interpolationSamples /= 2;

        m_startPosition = playheadSeconds * sampleRate;
        m_autoRestartAfterStop = true;
        m_startFadeInDurationSamples = interpolationSamples;

        m_gain.setTarget(0, interpolationSamples);
        m_finishCounter = interpolationSamples;
      }
    }
  }

  void ModuleSamplePlayer::SampleVoice::setSample(std::shared_ptr<Sample> s)
  {
    if(m_state != WAITING_FOR_SAMPLE) {

      Radiant::error("ModuleSamplePlayer::SampleVoice::setSample # Wrong state %p %d",
                     this, (int) m_state);
    }
    m_sample = s;
    m_state = PLAYING;

    if(m_info)
      m_info->m_sampleLengthSeconds = s->frames() / 44100.0f;
  }

  void ModuleSamplePlayer::SampleVoice::stop(float fadeTime, float sampleRate)
  {
    m_gain.setTarget(0, fadeTime * sampleRate);
    m_finishCounter = fadeTime * sampleRate;
    m_stopped = true;
  }

  void ModuleSamplePlayer::SampleVoice::scanDataToEnd(Radiant::BinaryData &data)
  {
    QByteArray buf;
    while(data.pos() < data.total()) {
      data.readString(buf);
      if(buf == "end")
        return;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ModuleSamplePlayer::LoadItem::LoadItem()
    : m_free(true)
  {
    memset(m_waiting, 0, sizeof(m_waiting));
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ModuleSamplePlayer::BGLoader::BGLoader(ModuleSamplePlayer * host)
    : m_host(host)
  {
    m_continue = true;
    run();
  }

  ModuleSamplePlayer::BGLoader::~BGLoader()
  {
    if(isRunning()) {
      m_continue = false;
      m_cond.wakeOne(m_mutex);
      waitEnd();
    }
  }

  bool ModuleSamplePlayer::BGLoader::addLoadable(const char * filename,
                                                 SampleVoice * waiting)
  {
    debugResonant("ModuleSamplePlayer::BGLoader::addLoadable # %s %p",
                  filename, waiting);

    for(int i = 0; i < BINS; i++) {
      if(m_loads[i].m_name == std::string(filename)) {
        return m_loads[i].addWaiting(waiting);
      }
    }

    for(int i = 0; i < BINS; i++) {
      if(m_loads[i].m_free) {
        m_loads[i].init(filename, waiting);
        break;
      }
    }

    m_cond.wakeAll(m_mutex);

    return false;
  }

  void ModuleSamplePlayer::BGLoader::childLoop()
  {
    while(m_continue) {

      debugResonant("ModuleSamplePlayer::BGLoader::childLoop # once");

      for(int i = 0; i < BINS; i++) {
        LoadItem & it = m_loads[i];

        if(!it.m_free) {

          debugResonant("ModuleSamplePlayer::BGLoader::childLoop # Something");

          std::shared_ptr<Sample> s(new Sample);

          bool good = true;

          if(!s->load(it.m_name.c_str(), it.m_name.c_str())) {
            Radiant::error("ModuleSamplePlayer::BGLoader::childLoop # Could not load "
                           "\"%s\"", it.m_name.c_str());
            good = false;
          }
          else if(!m_host->addSample(s)) {
            Radiant::error("ModuleSamplePlayer::BGLoader::childLoop # Could not add "
                           "\"%s\"", it.m_name.c_str());
            good = false;
          }

          if(!good) {
            for(int j = 0; j < LoadItem::WAITING_COUNT; j++) {
              SampleVoice * voice = it.m_waiting[j];
              if(!voice)
                break;
              voice->loadFailed();
            }
          }
          else {
            debugResonant("ModuleSamplePlayer::BGLoader::childLoop # Loaded "
                          "\"%s\"", it.m_name.c_str());

            for(int j = 0; j < LoadItem::WAITING_COUNT; j++) {
              SampleVoice * voice = it.m_waiting[j];
              if(!voice)
                break;

              debugResonant("ModuleSamplePlayer::BGLoader::childLoop # Delivering "
                            "\"%s\" to %p", it.m_name.c_str(), voice);

              voice->setSample(s);
            }
          }

          it.m_free = true;
        }
      }

      Radiant::Guard g(m_mutex);
      m_cond.wait(m_mutex);
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ModuleSamplePlayer::ModuleSamplePlayer()
    : m_channels(1)
    , m_active(0)
    , m_masterGain(1.0f)
    , m_userNoteIdCounter(1)
  {
    m_voices.resize(256);
    m_voiceptrs.resize(m_voices.size());
    if(!m_voiceptrs.empty())
      memset( & m_voiceptrs[0], 0, m_voiceptrs.size() * sizeof(SampleVoice *));
    m_samples.resize(2048);

    m_loader = new BGLoader(this);
  }

  ModuleSamplePlayer::~ModuleSamplePlayer()
  {}

  bool ModuleSamplePlayer::prepare(int & channelsIn, int & channelsOut)
  {
    //bool edit = false;

    if(channelsIn) {
      //edit = true;
      channelsIn = 0;
    }

    if(channelsOut != static_cast<int> (m_channels)) {
      //edit = true;
      channelsOut = static_cast<int> (m_channels);
    }

    return true;
  }

  void ModuleSamplePlayer::eventProcess(const QByteArray & id, Radiant::BinaryData & data)
  {
    const int bufsize = 256;
    char buf[bufsize];

    bool ok = true;

    // Radiant::info("ModuleSamplePlayer::eventProcess # %s", id.data());

    if(id == "playsample" || id == "playsample-at-location") {
      int voiceind = findFreeVoice();

      if(voiceind < 0) {
        Radiant::error("ModuleSamplePlayer::eventProcess # Out of polyphony");
        return;
      }

      ok = data.readString(buf, bufsize);

      if(!ok) {
        Radiant::error("ModuleSamplePlayer::eventProcess # Could not get sample name ");
        return;
      }

      SampleVoice & voice = m_voices[voiceind];
      m_voiceptrs[m_active] = & voice;

      int sampleind = findSample(buf);

      if(sampleind < 0) {
        debugResonant("ModuleSamplePlayer::eventProcess # No sample \"%s\"", buf);

        m_loader->addLoadable(buf, & voice);

        // return;
      }

      voice.init(this, sampleind >= 0 ?
                   m_samples[sampleind] : std::shared_ptr<Sample>(), data);
      m_active++;

      debugResonant("ModuleSamplePlayer::eventProcess # Started sample %s (%d/%ld)",
                    buf, voiceind, m_active);
      // assert(voiceind < (int) m_active);

    }
    else if(id == "stop-sample") {

      stopSampleInternal(data);

    }
    else if(id.startsWith("voice/")) {

      int slashLocation = id.indexOf('/', 6);

      if(slashLocation < 0) {
        Radiant::error("ModuleSamplePlayer::eventProcess # Bad voice command %s", id.data());
        return;
      }

      QByteArray voiceIdStr(id.data() + 6, slashLocation - 6);

      int voiceId = voiceIdStr.toInt();
      if(voiceId)
        controlSample(voiceId, id.data() + slashLocation + 1, data);

    }
    else if(id == "channels") {
      m_channels = data.readInt32( & ok);
    }
    else
      Radiant::error("ModuleSamplePlayer::eventProcess # Unknown message \"%s\"", id.data());

    if(!ok) {
      Radiant::error("ModuleSamplePlayer::eventProcess # When processing \"%s\"", id.data());
    }

    // Radiant::info("ModuleSamplePlayer::eventProcess # %s EXIT", id.data());
  }

  void ModuleSamplePlayer::process(float ** , float ** out, int n, const CallbackTime &)
  {
    size_t i;
    m_time = Radiant::TimeStamp::currentTime();

    // First zero the outputs
    for(i = 0; i < m_channels; i++)
      memset(out[i], 0, n * 4);

    // Then fill the outputs with audio
    for(i = 0; i < m_active; ) {
      if(!m_voiceptrs[i]->synthesize(out, n, this))
        dropVoice(i);
      else
        i++;
    }

    for(i = 0; i < m_channels; i++) {
      float * ptr = out[i];
      for(float * sentinel = ptr + n; ptr < sentinel; ptr++)
        *ptr *= m_masterGain;
    }


    /* for(i = 0; i < m_channels; i++)
       Radiant::info("ModuleSamplePlayer::process # %d %p %f", i, out[i], *out[i]);
    */

  }

  bool ModuleSamplePlayer::stop()
  {
    for(auto it : m_infos) {
      it.second->m_status = NOTE_FINISHED;
    }

    m_infos.clear();

    while(m_active > 0) {
      dropVoice(m_active - 1);
    }

    return true;
  }

  bool ModuleSamplePlayer::addSample(const char * filename, const char * name)
  {
    if(!Radiant::FileUtils::fileReadable(filename))
      return false;

    SampleInfo sv;
    sv.m_name = name;
    sv.m_filename = filename;
    m_sampleList.push_back(sv);

    return true;
  }

  void ModuleSamplePlayer::createAmbientBackground
  (const char * directory, float gain, int fillchannels, float delay)
  {
    Radiant::Directory dir(directory, Radiant::Directory::FILES);

    int n = 0;

    if(static_cast<size_t> (fillchannels) > channels())
      fillchannels = static_cast<int> (channels());

    for(int i = 0; i < dir.count(); i++) {

      QString file = dir.fileNameWithPath(i);

      QString suf = Radiant::FileUtils::suffixLowerCase(file);

      if(suf == "mp3") {
        std::string wavname(file.toStdString());
        strcpy( & wavname[wavname.size() - 3], "wav");

        // If the wav file already exists, then ignore the mp3 entry.
        /* To make this better, we could compare the timestamps...*/
        if(Radiant::FileUtils::fileReadable(QString::fromStdString(wavname)))
          continue;

        char command[128];

        /// @todo These should probably be documented somewhere
#ifdef WIN32
        sprintf(command, "madplay.exe %s -o wave:%s", file.toUtf8().data(), wavname.c_str());
#else
        sprintf(command, "mpg123 %s --wav %s", file.toUtf8().data(), wavname.c_str());
#endif
        Radiant::info("Performing mp3 -> wav conversion with [%s]", command);
#ifdef RADIANT_LINUX
        int err = Radiant::FileUtils::runInShell(command);
#else
        int err = system(command);
#endif
        if(err != 0)
          Radiant::error("ModuleSamplePlayer::createAmbientBackground # '%s' failed", command);

        file = QString::fromStdString(wavname);
      }

      n++;

      SF_INFO info;
      SNDFILE * sndf = AudioFileHandler::open(file, SFM_READ, & info);

      if(!sndf) {
        debugResonant("ModuleSamplePlayer::createAmbientBackground # failed to load '%s'",
                      file.toUtf8().data());
        continue;
      }

      sf_close(sndf);


      // Start everything in 7 seconds
      Radiant::TimeStamp startTime = Radiant::TimeStamp::currentTime() +
          Radiant::TimeStamp::createSeconds(delay);

      for(int c = 0; c < fillchannels; c++) {
        playSample(file.toUtf8().data(), gain, 1.0f, (c+i) % channels(), c % info.channels, true, startTime);
      }
    }

    debugResonant("ModuleSamplePlayer::createAmbientBackground # %d samples", n);
  }


  ModuleSamplePlayer::NoteInfo ModuleSamplePlayer::playSample(const char * filename,
                                                              float gain,
                                                              float relpitch,
                                                              int targetChannel,
                                                              int samplechannel,
                                                              bool loop,
                                                              Radiant::TimeStamp time)
  {
    NoteParameters parameters(QString::fromUtf8(filename));

    parameters.setGain(gain);
    parameters.setRelativePitch(relpitch);
    parameters.setTargetChannel(targetChannel);
    parameters.setSampleChannel(samplechannel);
    parameters.setLoop(loop);
    parameters.setPlaybackTime(time);

    return playSample(parameters);
  }

  ModuleSamplePlayer::NoteInfo ModuleSamplePlayer::playSample(const ModuleSamplePlayer::NoteParameters &parameters)
  {
    QByteArray fileName8(parameters.fileName().toLocal8Bit());

    SF_INFO info;
    SNDFILE * sndf = AudioFileHandler::open(fileName8.data(), SFM_READ, &info);

    if(!sndf) {
      Radiant::error("ModuleSamplePlayer::playSample # failed to load '%s'",
                     fileName8.data());
      return NoteInfo();
    }

    sf_close(sndf);

    int noteId;
    NoteInfo noteInfo;
    {
      Radiant::Guard g(m_mutex);
      noteId = m_userNoteIdCounter++;
      noteInfo.init(noteId);
      m_infos[noteId] = noteInfo.m_info;
    }

    Radiant::BinaryData control;
    control.writeString(id() + "/playsample");

    control.writeString(parameters.fileName());

    control.writeString("gain");
    control.writeFloat32(parameters.gain());

    // Relative pitch
    control.writeString("relpitch");
    control.writeFloat32(parameters.relativePitch() * info.samplerate / 44100.0f);

    // Infinite looping;
    control.writeString("loop");
    control.writeInt32(parameters.loop());

    // Select a channel from the sample
    control.writeString("samplechannel");
    control.writeInt32(parameters.sampleChannel());

    // Select the target channel for the sample
    control.writeString("targetchannel");
    control.writeInt32(parameters.targetChannel());

    control.writeString("time");
    control.writeTimeStamp(parameters.playbackTime());

    control.writeString("note-id");
    control.writeInt64(noteId);

    control.writeString("playhead-seconds");
    control.writeFloat32(parameters.samplePlayhead());

    // Finish parameters
    control.writeString("end");

    // Send the control message to the sample player.
    DSPNetwork::instance()->send(control);

    return noteInfo;
  }

  ModuleSamplePlayer::NoteInfo ModuleSamplePlayer::playSampleAtLocation(const char *filename, float gain, float relpitch,
                                                                        Nimble::Vector2 location, int sampleChannel,
                                                                        bool loop, Radiant::TimeStamp time)
  {
    SF_INFO info;
    SNDFILE * sndf = AudioFileHandler::open(filename, SFM_READ, & info);

    if(!sndf) {
      Radiant::error("ModuleSamplePlayer::playSampleAtLocation # failed to load '%s'",
                     filename);
      return NoteInfo();
    }

    sf_close(sndf);

    NoteInfo noteInfo;
    int noteId;
    {
      Radiant::Guard g(m_mutex);
      noteId = m_userNoteIdCounter++;
      noteInfo.init(noteId);
      m_infos[noteId] = noteInfo.m_info;
    }

    Radiant::BinaryData control;
    control.writeString(id() + "/playsample-at-location");

    control.writeString(filename);

    control.writeString("gain");
    control.writeFloat32(gain);

    // Relative pitch
    control.writeString("relpitch");
    control.writeFloat32(relpitch * info.samplerate / 44100.0f);

    // Infinite looping;
    control.writeString("loop");
    control.writeInt32(loop);

    // Select a channel from the sample
    control.writeString("samplechannel");
    control.writeInt32(sampleChannel);

    // Select the target channel for the sample
    control.writeString("location");
    control.writeVector2Float32(location);

    control.writeString("time");
    control.writeTimeStamp(time);

    control.writeString("note-id");
    control.writeInt64(noteId);

    // Finish parameters
    control.writeString("end");

    // Send the control message to the sample player.
    DSPNetwork::instance()->send(control);

    return noteInfo;
  }

  void ModuleSamplePlayer::stopSample(int noteId)
  {
    if(noteId <= 0) {
      Radiant::error("ModuleSamplePlayer::stopSample # Invalid note id %d (value should be greater than zero)",
                     noteId);
      return;
    }

    Radiant::BinaryData control;
    control.writeString(id() + "/stop-sample");

    control.writeString("note-id");
    control.writeInt64(noteId);

    // Finish parameters
    control.writeString("end");

    // Send the control message to the sample player.
    DSPNetwork::instance()->send(control);
  }

  void ModuleSamplePlayer::setSampleGain(const ModuleSamplePlayer::NoteInfo & info, float gain, float interpolationTimeSeconds)
  {
    if(!info.isPlaying()) return;

    Radiant::BinaryData control;
    char buf[64];
    sprintf(buf, "%d", info.noteId());
    control.writeString(id() + "/voice/" + QByteArray(buf) + "/control");

    control.writeString("gain");
    // Radiant::info("ModuleSamplePlayer::setSampleGain # Writing gain at %d", control.pos());
    control.writeFloat32(gain);

    control.writeString("interpolation-time");
    control.writeFloat32(interpolationTimeSeconds);

    // Finish parameters
    control.writeString("end");

    // Send the control message to the sample player.
    DSPNetwork::instance()->send(control);
  }

  void ModuleSamplePlayer::setSampleRelativePitch(const ModuleSamplePlayer::NoteInfo &info, float relativePitch, float interpolationTimeSeconds)
  {
    if(!info.isPlaying()) return;

    Radiant::BinaryData control;
    char buf[64];
    sprintf(buf, "%d", info.noteId());
    control.writeString(id() + "/voice/" + QByteArray(buf) + "/control");

    control.writeString("relative-pitch");
    control.writeFloat32(relativePitch);

    control.writeString("interpolation-time");
    control.writeFloat32(interpolationTimeSeconds);

    // Finish parameters
    control.writeString("end");

    // Send the control message to the sample player.
    DSPNetwork::instance()->send(control);
  }

  void ModuleSamplePlayer::setSamplePlayHead(const ModuleSamplePlayer::NoteInfo &info, float playHeadTimeSeconds, float interpolationTimeSeconds)
  {
    if(!info.isPlaying()) return;

    Radiant::BinaryData control;
    char buf[64];
    sprintf(buf, "%d", info.noteId());
    control.writeString(id() + "/voice/" + QByteArray(buf) + "/control");

    control.writeString("playhead-seconds");
    control.writeFloat32(playHeadTimeSeconds);

    control.writeString("interpolation-time");
    control.writeFloat32(interpolationTimeSeconds);

    // Finish parameters
    control.writeString("end");

    // Send the control message to the sample player.
    DSPNetwork::instance()->send(control);
  }

  void ModuleSamplePlayer::setSampleLooping(const ModuleSamplePlayer::NoteInfo &info, bool looping)
  {
    if(!info.isPlaying()) return;

    Radiant::BinaryData control;
    char buf[64];
    sprintf(buf, "%d", info.noteId());
    control.writeString(id() + "/voice/" + QByteArray(buf) + "/control");

    control.writeString("loop");
    control.writeInt32(looping);

    // Finish parameters
    control.writeString("end");

    // Send the control message to the sample player.
    DSPNetwork::instance()->send(control);
  }

  int ModuleSamplePlayer::locationToChannel(Nimble::Vector2 location)
  {
    DSPNetwork::ItemPtr item = DSPNetwork::instance()->findItem("panner");
    if(!item) {
      debugResonant("ModuleSamplePlayer::locationToChannel # Failed to find a panner");
      return 0;
    }

    std::shared_ptr<Resonant::ModulePanner> pan =
        std::dynamic_pointer_cast<Resonant::ModulePanner>(item->module());

    if(!pan) {
      Radiant::error("ModuleSamplePlayer::locationToChannel # Failed to cast a panner");
      return 0;
    }

    return pan->locationToChannel(location);
  }

  int ModuleSamplePlayer::findFreeVoice()
  {
    for(unsigned i = 0; i < m_voices.size(); i++) {
      if(!m_voices[i].isActive())
        return i;
    }
    return -1;
  }

  int ModuleSamplePlayer::findSample(const char * name)
  {
    for(unsigned i = 0; i < m_samples.size(); i++) {
      Sample * s = m_samples[i].get();

      if(s)
        if(s->name() == name)
          return i;
    }

    return -1;
  }

  void ModuleSamplePlayer::loadSamples()
  {
    m_samples.clear();

    for(std::list<SampleInfo>::iterator it = m_sampleList.begin(); it != m_sampleList.end(); ++it) {
      std::shared_ptr<Sample> s(new Sample());
      if(s->load((*it).m_filename.toUtf8().data(), (*it).m_name.toUtf8().data()))
        m_samples.push_back(s);
    }
  }

  void ModuleSamplePlayer::stopSampleInternal(Radiant::BinaryData &data)
  {
    int noteId = 0;
    float fadeTime = 0.02f;


    const int buflen = 64;
    char name[buflen + 1] = { '\0' };

    if(!data.readString(name, buflen)) {
      Radiant::error("ModuleSamplePlayer::SampleVoice::stopSampleInternal # Invalid beginning");

      return;
    }

    bool ok = true;

    while(name[0] != '\0' && strcmp(name, "end") != 0 && ok) {

      if(strcmp(name, "note-id") == 0)
        noteId = data.readInt32( & ok);
      else if(strcmp(name, "fade-time") == 0)
        fadeTime = data.readFloat32( & ok);

      ok = data.readString(name, buflen);
    }

    if(noteId > 0) {
      SampleVoice * voice = findVoiceForNoteId(noteId);
      if(voice) {
        voice->stop(fadeTime);
      }
    }
  }

  void ModuleSamplePlayer::controlSample(int voiceId, const QByteArray &parameter, Radiant::BinaryData &data)
  {
    SampleVoice * voice = findVoiceForNoteId(voiceId);
    if(voice) {
      voice->processMessage(this, parameter, data);
    }
    else {
      SampleVoice::scanDataToEnd(data);
      // Radiant::error("ModuleSamplePlayer::controlSample # No voice %d", voiceId);
    }
  }

  bool ModuleSamplePlayer::addSample(std::shared_ptr<Sample> s)
  {
    for(unsigned i = 0; i < m_samples.size(); i++) {
      if(!m_samples[i]) {
        m_samples[i] = s;
        return true;
      }
      debugResonant("ModuleSamplePlayer::addSample # m_samples[%u] = %p",
                    i, m_samples[i].get());
    }

    return false;
  }

  void ModuleSamplePlayer::dropVoice(size_t i)
  {
    // trace("ModuleSamplePlayer::dropVoice # %d", i);
    assert( i < m_active);
    m_active--;
    NoteInfoInternalPtr noteInfo = m_voiceptrs[i]->info();
    m_voiceptrs[i]->clear();
    for( ; i < m_active; i++) {
      m_voiceptrs[i] = m_voiceptrs[i + 1];
    }
    m_voiceptrs[m_active] = 0;

    if(noteInfo) {
      Radiant::Guard g(m_mutex);
      auto it = m_infos.find(noteInfo->m_noteId);
      if(it != m_infos.end()) {
        m_infos.erase(it);
      }

      noteInfo->m_status = NOTE_FINISHED;
    }
  }

  ModuleSamplePlayer::SampleVoice * ModuleSamplePlayer::findVoiceForNoteId(int noteId)
  {
    for(size_t i = 0; i < m_active; i++) {
      SampleVoice * voice = m_voiceptrs[i];
      if(voice->noteId() == noteId)
        return voice;
    }
    return 0;
  }

  }

