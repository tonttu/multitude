/* COPYRIGHT
 */

#include "ModuleSamplePlayer.hpp"
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

  using namespace Radiant;

  class ModuleSamplePlayer::Sample::Internal
  {
  public:
    Internal()
    { bzero( & m_info, sizeof(m_info)); };

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

    bzero(&m_d->m_info, sizeof(m_d->m_info));

    SNDFILE * sndf = sf_open(filename, SFM_READ, & m_d->m_info);

    if(!sndf)
      return false;

    m_data.resize(m_d->m_info.channels * m_d->m_info.frames);
    if(!m_data.empty())
      bzero( & m_data[0], m_data.size() * sizeof(float));

    size_t block = 1000;

    size_t pos = 0;

    while(pos < static_cast<size_t> (m_d->m_info.frames)) {
      size_t get = Nimble::Math::Min((size_t) (m_d->m_info.frames - pos), block);
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
      error("ModuleSamplePlayer::SampleVoice::synthesize # channel count exceeded for %s "
            "%ld >= %ld", m_sample->name().c_str(),
            m_targetChannel, host->channels());
      m_state = INACTIVE;
      return false;
    }

    float * b1 = out[m_targetChannel];
    float gain = m_gain;
    float pitch = m_relPitch;

    bool more;

    int chans = m_sample->channels();

    if(pitch == 1.0f) {
      const float * src = m_sample->buf(m_position) + m_sampleChannel;

      for(unsigned i = 0; i < avail; i++) {
        *b1++ += *src * gain;
        src += chans;
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
      }

      m_dpos = dpos;

      more = dpos < dmax;
    }

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
      (std::shared_ptr<Sample> sample, Radiant::BinaryData * data)
  {
    m_sample = sample;
    m_position = 0;

    m_gain = 1.0f;
    m_relPitch = 1.0f;
    m_sampleChannel = 0;
    m_targetChannel = 0;
    m_dpos = 0.0f;
    m_loop = false;

    const int buflen = 64;
    char name[buflen] = { '\0' };

    if(!data->readString(name, buflen)) {
      error("ModuleSamplePlayer::SampleVoice::init # Invalid beginning");

      return;
    }

    while(name[0] != '\0' && strcmp(name, "end") != 0) {

      bool ok = true;

      if(strcmp(name, "gain") == 0)
        m_gain = data->readFloat32( & ok);
      else if(strcmp(name, "relpitch") == 0)
        m_relPitch = data->readFloat32( & ok);
      else if(strcmp(name, "samplechannel") == 0)
        m_sampleChannel = data->readInt32( & ok);
      else if(strcmp(name, "targetchannel") == 0)
        m_targetChannel = data->readInt32( & ok);
      else if(strcmp(name, "loop") == 0)
        m_loop = (data->readInt32( & ok) != 0);
      else if(strcmp(name, "time") == 0)
        m_startTime = data->readTimeStamp( & ok);
      else {
        error("ModuleSamplePlayer::SampleVoice::init # Invalid parameter \"%s\"",
              name);
        break;
      }

      if(!ok) {
        error("ModuleSamplePlayer::SampleVoice::init # Error parsing value for %s",
              name);
      }
      else {
        debugResonant("ModuleSamplePlayer::SampleVoice::init # got %s", name);
      }

      name[0] = '\0';
      if(!data->readString(name, buflen)) {
        error("ModuleSamplePlayer::SampleVoice::init # Error reading parameter");
        break;
      }
    }

    m_dpos = 0.0;

    m_state = sample ? PLAYING : WAITING_FOR_SAMPLE;

    debugResonant("ModuleSamplePlayer::SampleVoice::init # %p Playing gain = %.3f "
          "rp = %.3f, ss = %ld, ts = %ld", this, m_gain, m_relPitch,
          m_sampleChannel, m_targetChannel);
  }

  void ModuleSamplePlayer::SampleVoice::setSample(std::shared_ptr<Sample> s)
  {
    if(m_state != WAITING_FOR_SAMPLE) {

      error("ModuleSamplePlayer::SampleVoice::setSample # Wrong state %p %d",
            this, (int) m_state);
    }
    m_sample = s;
    m_state = PLAYING;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ModuleSamplePlayer::LoadItem::LoadItem()
      : m_free(true)
  {
    bzero(m_waiting, sizeof(m_waiting));
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
            error("ModuleSamplePlayer::BGLoader::childLoop # Could not load "
                  "\"%s\"", it.m_name.c_str());
            good = false;
          }
          else if(!m_host->addSample(s)) {
            error("ModuleSamplePlayer::BGLoader::childLoop # Could not add "
                  "\"%s\"", it.m_name.c_str());
            good = false;
          }

          if(!good) {
            for(int j = 0; j < LoadItem::WAITING_COUNT; i++) {
              SampleVoice * voice = it.m_waiting[i];
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

      m_mutex.lock();
      m_cond.wait(m_mutex);
      m_mutex.unlock();
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  ModuleSamplePlayer::ModuleSamplePlayer(Application * a)
      : Module(a),
      m_channels(1),
      m_active(0),
      m_masterGain(1.0f)
  {
    m_voices.resize(256);
    m_voiceptrs.resize(m_voices.size());
    if(!m_voiceptrs.empty())
      bzero( & m_voiceptrs[0], m_voiceptrs.size() * sizeof(SampleVoice *));
    m_samples.resize(2048);

    m_loader = new BGLoader(this);
  }

  ModuleSamplePlayer::~ModuleSamplePlayer()
  {}

  bool ModuleSamplePlayer::prepare(int & channelsIn, int & channelsOut)
  {
    bool edit = false;

    if(channelsIn) {
      edit = true;
      channelsIn = 0;
    }

    if(channelsOut != static_cast<int> (m_channels)) {
      edit = true;
      channelsOut = static_cast<int> (m_channels);
    }

    return true;
  }

  void ModuleSamplePlayer::processMessage(const char * id, Radiant::BinaryData * data)
  {
    const int bufsize = 256;
    char buf[bufsize];

    bool ok = true;

    if(strcmp(id, "playsample") == 0) {
      int voiceind = findFreeVoice();

      if(voiceind < 0) {
        error("ModuleSamplePlayer::control # Out of polyphony");
        return;
      }

      ok = data->readString(buf, bufsize);

      if(!ok) {
        error("ModuleSamplePlayer::control # Could not get sample name ");
        return;
      }

      SampleVoice & voice = m_voices[voiceind];
      m_voiceptrs[m_active] = & voice;

      int sampleind = findSample(buf);

      if(sampleind < 0) {
        debugResonant("ModuleSamplePlayer::control # No sample \"%s\"", buf);

        m_loader->addLoadable(buf, & voice);

        // return;
      }

      voice.init(sampleind >= 0 ?
                 m_samples[sampleind] : std::shared_ptr<Sample>(), data);
      m_active++;

      debugResonant("ModuleSamplePlayer::control # Started sample %s (%d/%ld)",
            buf, voiceind, m_active);
      // assert(voiceind < (int) m_active);

    }
    else if(strcmp(id, "channels") == 0) {
      m_channels = data->readInt32( & ok);
    }
    else
      error("ModuleSamplePlayer::control # Unknown message \"%s\"", id);

    if(!ok) {
      error("ModuleSamplePlayer::control # When processing \"%s\"", id);
    }
  }

  void ModuleSamplePlayer::process(float ** , float ** out, int n)
  {
    size_t i;
    m_time = Radiant::TimeStamp::getTime();

    // First zero the outputs
    for(i = 0; i < m_channels; i++)
      bzero(out[i], n * 4);

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
       info("ModuleSamplePlayer::process # %d %p %f", i, out[i], *out[i]);
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
    Radiant::Directory dir(directory, Directory::Files);

    int n = 0;

    if(static_cast<size_t> (fillchannels) > channels())
      fillchannels = static_cast<int> (channels());

    for(int i = 0; i < dir.count(); i++) {

      std::string file = dir.fileNameWithPath(i);

      std::string suf = Radiant::FileUtils::suffixLowerCase(file);

      if(suf == "mp3") {
        std::string wavname(file);
        strcpy( & wavname[wavname.size() - 3], "wav");

        // If the wav file already exists, then ignore the mp3 entry.
        /* To make this better, we could compare the timestamps...*/
        if(Radiant::FileUtils::fileReadable(wavname))
          continue;

        char command[128];

		/// @todo These should probably be documented somewhere
#ifdef WIN32
        sprintf(command, "madplay.exe %s -o wave:%s", file.c_str(), wavname.c_str());
#else
        sprintf(command, "mpg123 %s --wav %s", file.c_str(), wavname.c_str());
#endif
        info("Performing mp3 -> wav conversion with [%s]", command);
        system(command);
        file = wavname;
      }

      n++;

      SF_INFO info;
      SNDFILE * sndf = sf_open(file.c_str(), SFM_READ, & info);

      if(!sndf) {
        debugResonant("ModuleSamplePlayer::playSample # failed to load '%s'",
                       file.c_str());
        continue;
      }

      sf_close(sndf);


      // Start everything in 7 seconds
      Radiant::TimeStamp startTime = Radiant::TimeStamp::getTime() +
                                     Radiant::TimeStamp::createSecondsD(delay);

      for(int c = 0; c < fillchannels; c++) {
        playSample(file.c_str(), gain, 1.0f, (c+i) % channels(), c % info.channels, true, startTime);
      }
    }

    debugResonant("ModuleSamplePlayer::createAmbientBackground # %d samples", n);
  }


  void ModuleSamplePlayer::playSample(const char * filename,
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
      return;
    }

    sf_close(sndf);

    Radiant::BinaryData control;
    control.writeString(std::string(id()) + "/playsample");

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

    // Select the target channel for the sample
    control.writeString("time");
    control.writeTimeStamp(time);

    // Finish parameters
    control.writeString("end");

    // Send the control message to the sample player.
    DSPNetwork::instance()->send(control);

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

    for(std::list<SampleInfo>::iterator it = m_sampleList.begin();
    it != m_sampleList.end(); it++) {
      std::shared_ptr<Sample> s(new Sample());
      if(s->load((*it).m_filename.c_str(), (*it).m_name.c_str()))
        m_samples.push_back(s);
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
}

