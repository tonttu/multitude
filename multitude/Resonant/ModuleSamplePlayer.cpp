/* COPYRIGHT
 *
 * This file is part of Resonant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Resonant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#include "ModuleSamplePlayer.hpp"

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
    m_d = new Internal();
  }

  ModuleSamplePlayer::Sample::~Sample()
  {
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

    uint block = 1000;

    unsigned pos = 0;

    while(pos < m_d->m_info.frames) {
      uint get = Nimble::Math::Min((uint) (m_d->m_info.frames - pos), block);
      uint n = get * m_d->m_info.channels;

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

  bool ModuleSamplePlayer::SampleVoice::synthesize(float ** out, int n)
  {
    if(m_state != PLAYING) {
      //printf(":"); fflush(0);
      return m_state == WAITING_FOR_SAMPLE;
    }

    unsigned avail = m_sample->available(m_position);

    if((int) avail > n)
      avail = n;

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
        // debug("ModuleSamplePlayer::SampleVoice::synthesize # rewind");
        m_position = 0;
        m_dpos = 0.0f;
        more = 1;
      }
      else {
        // debug("ModuleSamplePlayer::SampleVoice::synthesize # done");
        m_sample = 0;
        m_state = INACTIVE;
      }
    }

    return more != 0;
  }

  void ModuleSamplePlayer::SampleVoice::init
      (Sample * sample, Radiant::BinaryData * data)
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
        debug("ModuleSamplePlayer::SampleVoice::init # got %s", name);
      }

      name[0] = '\0';
      if(!data->readString(name, buflen)) {
        error("ModuleSamplePlayer::SampleVoice::init # Error reading parameter");
        break;
      }
    }

    m_dpos = 0.0;

    m_state = sample ? PLAYING : WAITING_FOR_SAMPLE;

    debug("ModuleSamplePlayer::SampleVoice::init # %p Playing gain = %.3f "
          "rp = %.3f, ss = %d, ts = %d", this, m_gain, m_relPitch,
          m_sampleChannel, m_targetChannel);
  }

  void ModuleSamplePlayer::SampleVoice::setSample(Sample * s)
  {
    if(m_state != WAITING_FOR_SAMPLE) {

      fatal("ModuleSamplePlayer::SampleVoice::setSample # Wrong state %p",
            this);
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
    debug("ModuleSamplePlayer::BGLoader::addLoadable # %s %p",
          filename, waiting);

    for(int i = 0; i < BINS; i++) {
      if(m_loads[i].m_name == filename) {
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

      debug("ModuleSamplePlayer::BGLoader::childLoop # once");

      for(int i = 0; i < BINS; i++) {
        LoadItem & it = m_loads[i];

        if(!it.m_free) {

          debug("ModuleSamplePlayer::BGLoader::childLoop # Something");

          Sample * s = new Sample();

          bool good = true;

          if(!s->load(it.m_name.str(), it.m_name.str())) {
            error("ModuleSamplePlayer::BGLoader::childLoop # Could not load "
                  "\"%s\"", it.m_name.str());
            good = false;
          }
          else if(!m_host->addSample(s)) {
            error("ModuleSamplePlayer::BGLoader::childLoop # Could not add "
                  "\"%s\"", it.m_name.str());
            good = false;
          }

          if(!good) {
            delete s;
            for(int j = 0; j < LoadItem::WAITING_COUNT; i++) {
              SampleVoice * voice = it.m_waiting[i];
              if(!voice)
                break;
              voice->loadFailed();
            }
          }
          else {
            debug("ModuleSamplePlayer::BGLoader::childLoop # Loaded "
                  "\"%s\"", it.m_name.str());

            for(int j = 0; j < LoadItem::WAITING_COUNT; j++) {
              SampleVoice * voice = it.m_waiting[j];
              if(!voice)
                break;

              debug("ModuleSamplePlayer::BGLoader::childLoop # Delivering "
                    "\"%s\" to %p", it.m_name.str(), voice);

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
      m_active(0)
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

    if(channelsOut != (int) m_channels) {
      edit = true;
      channelsOut = m_channels;
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
        Radiant::trace(Radiant::DEBUG, "ModuleSamplePlayer::control # No sample \"%s\"", buf);

        m_loader->addLoadable(buf, & voice);

        // return;
      }

      voice.init(sampleind >= 0 ?
                 m_samples[sampleind].ptr() : 0, data);
      m_active++;

      debug("ModuleSamplePlayer::control # Started sample %s (%d/%d)",
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
    uint i;

    // First zero the outputs
    for(i = 0; i < m_channels; i++)
      bzero(out[i], n * 4);

    // Then fill the outputs with audio
    for(i = 0; i < m_active; ) {
      if(!m_voiceptrs[i]->synthesize(out, n))
        dropVoice(i);
      else
        i++;
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
      (const char * directory, float gain)
  {
    using Radiant::Directory;
    Directory dir(directory, Directory::Files);

    int n = 0;

    for(int i = 0; i < dir.count(); i++) {

      std::string file = dir.fileNameWithPath(i);

      n++;

      SF_INFO info;
      SNDFILE * sndf = sf_open(file.c_str(), SFM_READ, & info);

      if(!sndf) {
        Radiant::debug("ModuleSamplePlayer::playSample # failed to load '%s'",
                       file.c_str());
        continue;
      }

      sf_close(sndf);

      for(int c = 0; c < info.channels; c++) {
        playSample(file.c_str(), gain, 1.0f, c, c, true);
      }
    }

    debug("ModuleSamplePlayer::createAmbientBackground # %d samples", n);
  }


  void ModuleSamplePlayer::playSample(const char * filename,
                                      float gain,
                                      float relpitch,
                                      int targetChannel,
                                      int samplechannel,
                                      bool loop)
  {

    SF_INFO info;
    SNDFILE * sndf = sf_open(filename, SFM_READ, & info);

    if(!sndf) {
      Radiant::debug("ModuleSamplePlayer::playSample # failed to load '%s'",
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
      Sample * s = m_samples[i].ptr();

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
      Radiant::RefPtr<Sample> s(new Sample());
      if(s->load((*it).m_filename.c_str(), (*it).m_name.c_str()))
        m_samples.push_back(s);
    }
  }

  bool ModuleSamplePlayer::addSample(Sample * s)
  {
    for(unsigned i = 0; i < m_samples.size(); i++) {
      if(!m_samples[i].ptr()) {
        m_samples[i] = s;
        return true;
      }
      debug("ModuleSamplePlayer::addSample # m_samples[%u] = %p",
            i, m_samples[i].ptr());
    }

    return false;
  }

  void ModuleSamplePlayer::dropVoice(unsigned i)
  {
    // trace("ModuleSamplePlayer::dropVoice # %d", i);
    assert((uint) i < m_active);
    m_active--;
    m_voiceptrs[i]->clear();
    for( ; i < m_active; i++) {
      m_voiceptrs[i] = m_voiceptrs[i + 1];
    }
    m_voiceptrs[m_active] = 0;
  }
}

