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

#include <Nimble/Math.hpp>

#include <Radiant/BinaryData.hpp>
#include <Radiant/Directory.hpp>
#include <Radiant/FileUtils.hpp>
#include <Radiant/Trace.hpp>

#include <sndfile.h>
#include <cassert>

namespace Resonant {

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

    SNDFILE * sndf = sf_open(filename, SFM_READ, & m_d->m_info);

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
    if(m_state != PLAYING || m_startTime > host->time()) {
      //printf(":"); fflush(0);
      return m_state == WAITING_FOR_SAMPLE;
    }

    unsigned avail = m_sample->available(m_position);

    if((int) avail > n)
      avail = n;

    if(m_targetChannel >= host->channels()) {
      Radiant::error("ModuleSamplePlayer::SampleVoice::synthesize # channel count exceeded for %s "
            "%ld >= %ld", m_sample->name().toUtf8().data(),
            m_targetChannel, host->channels());
      m_state = INACTIVE;
      return false;
    }

    float * b1 = out[m_targetChannel];
    Nimble::Rampd gain = m_gain;
    float pitch = m_relPitch;

    bool more;

    int chans = m_sample->channels();

    if(pitch == 1.0f) {
      const float * src = m_sample->buf(m_position) + m_sampleChannel;

      for(unsigned i = 0; i < avail; i++) {
        *b1++ += *src * gain;
        src += chans;
        gain.update();
      }

      m_position += avail;

      more = (int) avail == n;
    }
    else {
      double dpos = m_dpos;
      double dmax = m_sample->frames() - 1;

      const float * src = m_sample->buf(0) + m_sampleChannel;

      for(int i = 0 ; i < n && dpos < dmax; i++) {

        int base = (int) dpos;
        double w2 = dpos - (double) base;
        *b1++ += gain *
                 float((src[base * chans] * (1.0 - w2) + src[(base+1) * chans] * w2));
        dpos += pitch;
        gain.update();
      }

      m_dpos = dpos;

      m_gain = gain;
      more = dpos < dmax;
    }

    if(m_finishCounter > 0 && (m_finishCounter - n) <= 0) {
      more = 0;
      m_loop = false;
    }
    m_finishCounter -= n;

    if(!more) {
      if(m_loop) {
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

    m_dpos = 0.0;

    m_state = sample ? PLAYING : WAITING_FOR_SAMPLE;

    debugResonant("ModuleSamplePlayer::SampleVoice::init # %p Playing gain = %.3f "
          "rp = %.3f, ss = %ld, ts = %ld", this, m_gain.value(), m_relPitch,
          m_sampleChannel, m_targetChannel);
  }

  void ModuleSamplePlayer::SampleVoice::setSample(std::shared_ptr<Sample> s)
  {
    if(m_state != WAITING_FOR_SAMPLE) {

      Radiant::error("ModuleSamplePlayer::SampleVoice::setSample # Wrong state %p %d",
            this, (int) m_state);
    }
    m_sample = s;
    m_state = PLAYING;
  }

  void ModuleSamplePlayer::SampleVoice::stop(float fadeTime, float sampleRate)
  {
    m_gain.setTarget(0, fadeTime * sampleRate);
    m_finishCounter = fadeTime * sampleRate;
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
        int err = system(command);
        if(err != 0)
          Radiant::error("ModuleSamplePlayer::createAmbientBackground # '%s' failed", command);

        file = QString::fromStdString(wavname);
      }

      n++;

      SF_INFO info;
      SNDFILE * sndf = sf_open(file.toUtf8().data(), SFM_READ, & info);

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


  int ModuleSamplePlayer::playSample(const char * filename,
                                      float gain,
                                      float relpitch,
                                      int targetChannel,
                                      int samplechannel,
                                      bool loop,
                                      Radiant::TimeStamp time)
  {

    SF_INFO info;
    SNDFILE * sndf = sf_open(filename, SFM_READ, & info);

    if(!sndf) {
      Radiant::error("ModuleSamplePlayer::playSample # failed to load '%s'",
                     filename);
      return 0;
    }

    sf_close(sndf);

    int noteId;
    {
      Radiant::Guard g(m_mutex);
      noteId = m_userNoteIdCounter++;
    }

    Radiant::BinaryData control;
    control.writeString(id() + "/playsample");

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
    control.writeInt32(samplechannel);

    // Select the target channel for the sample
    control.writeString("targetchannel");
    control.writeInt32(targetChannel);

    control.writeString("time");
    control.writeTimeStamp(time);

    control.writeString("note-id");
    control.writeInt64(noteId);

    // Finish parameters
    control.writeString("end");

    // Send the control message to the sample player.
    DSPNetwork::instance()->send(control);

    return noteId;
  }

  int ModuleSamplePlayer::playSampleAtLocation(const char *filename, float gain, float relpitch,
                                               Nimble::Vector2 location, int sampleChannel,
                                               bool loop, Radiant::TimeStamp time)
  {
    SF_INFO info;
    SNDFILE * sndf = sf_open(filename, SFM_READ, & info);

    if(!sndf) {
      Radiant::error("ModuleSamplePlayer::playSample # failed to load '%s'",
                     filename);
      return 0;
    }

    sf_close(sndf);

    int noteId;
    {
      Radiant::Guard g(m_mutex);
      noteId = m_userNoteIdCounter++;
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

    return noteId;
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

  int ModuleSamplePlayer::locationToChannel(Nimble::Vector2 location)
  {
    DSPNetwork::Item * item = DSPNetwork::instance()->findItem("panner");
    if(!item) {
      Radiant::error("ModuleSamplePlayer::locationToChannel # Failed to find a panner");
      return 0;
    }

    Resonant::ModulePanner * pan = dynamic_cast<Resonant::ModulePanner *>(item->module());

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
      Radiant::error("ModuleSamplePlayer::SampleVoice::init # Invalid beginning");

      return;
    }

    bool ok = true;

    while(name[0] != '\0' && strcmp(name, "end") != 0 && ok) {

      bool ok = true;

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
    m_voiceptrs[i]->clear();
    for( ; i < m_active; i++) {
      m_voiceptrs[i] = m_voiceptrs[i + 1];
    }
    m_voiceptrs[m_active] = 0;
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

