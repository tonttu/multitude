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

#include "DSPNetwork.hpp"
#include "Resonant.hpp"
#include "AudioLoop_private.hpp"

#include "ModulePanner.hpp"
#include "ModuleOutCollect.hpp"
#include "ModuleSamplePlayer.hpp"

#include <Radiant/Trace.hpp>

#include <strings.h>

#include <algorithm>
#include <typeinfo>
#include <cstdio>

#include <portaudio.h>

namespace Resonant {

  using namespace Radiant;

  DSPNetwork::Item::Item()
    : m_module(0),
      m_compiled(false),
      m_done(false),
      m_usePanner(true),
      m_targetChannel(-1)
  {}

  DSPNetwork::Item::~Item()
  {}

  void DSPNetwork::Item::deleteModule()
  {
    if(m_module) {

    }
    delete m_module;
    m_module = 0;
  }

  void DSPNetwork::Item::eraseInput(const Connection & c)
  {
    std::vector<Connection>::iterator it =
      std::find(m_inputs.begin(), m_inputs.end(), c);

    if(it != m_inputs.end())
      m_inputs.erase(it);
  }

  void DSPNetwork::Item::eraseInputs(const QString & moduleId)
  {
    for(unsigned i = 0; i < m_inputs.size(); ) {

      if(moduleId == m_inputs[i].m_moduleId) {
        m_inputs.erase(m_inputs.begin() + i);
        m_ins.erase(m_ins.begin() + i);
      }
      else
        i++;
    }
  }

  int DSPNetwork::Item::findInInput(float * ptr) const
  {
    for(size_t i = 0; i < m_ins.size(); i++)
      if(m_ins[i] == ptr)
        return static_cast<int> (i);
    return -1;
  }

  int DSPNetwork::Item::findInOutput(float * ptr) const
  {
    for(size_t i = 0; i < m_outs.size(); i++)
      if(m_outs[i] == ptr)
        return (int) i;
    return -1;
  }

  void DSPNetwork::Item::removeInputsFrom(const QString & id)
  {
    for(std::list<NewConnection>::iterator it = m_connections.begin();
    it != m_connections.end(); ) {

      if(id == it->m_sourceId) {
        it = m_connections.erase(it);
      }
      else
        it++;
    }

    for(unsigned i = 0; i < m_inputs.size(); ) {
      if(id == m_inputs[i].m_moduleId) {
        m_inputs.erase(m_inputs.begin() + i);
        m_ins.erase(m_ins.begin() + i);
      }
      else
        i++;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  DSPNetwork::DSPNetwork()
    : // m_continue(false),
    m_panner(0),
    m_frames(0),
    m_doneCount(0),
    m_syncinfo()
  {
    m_collect = new ModuleOutCollect(0, this);
    m_collect->setId("outcollect");

    Item tmp;
    tmp.m_module = m_collect;

    m_newItems.push_back(tmp);
  }

  DSPNetwork::~DSPNetwork()
  {
    stop();

    for(size_t i = 0; i < m_buffers.size(); i++)
      m_buffers[i].clear();

    for(container::iterator i = m_items.begin(); i != m_items.end(); i++)
      i->deleteModule();
  }

  bool DSPNetwork::start(const QString & device)
  {
    Radiant::Guard g(m_startupMutex);

    debugResonant("DSPNetwork::start # %p", this);

    if(isRunning())
      return false;

    m_devName = device;

    // m_continue = true;

    return startReadWrite(44100, 8);
  }


  void DSPNetwork::addModule(Item & i)
  {
    debugResonant("DSPNetwork::addModule # %p", this);

    Radiant::Guard g( m_newMutex);

    m_newItems.push_back(i);
  }

  void DSPNetwork::markDone(Item & i)
  {
    assert(i.module());
    markDone(*i.module());
  }

  void DSPNetwork::markDone(Module & m)
  {
    Radiant::Guard g( m_newMutex);
    Item * it = findItem(m.id());

    if(it) {
      it->m_done = true;
      m_doneCount++;
    }
    else
      error("DSPNetwork::markDone # Failed for \"%s\"", m.id().toUtf8().data());
  }

  void DSPNetwork::send(Radiant::BinaryData & control)
  {
    debugResonant("DSPNetwork::send # %p", this);

    Radiant::Guard g( m_inMutex);
    m_incoming.append(control);
  }

  ModuleSamplePlayer * DSPNetwork::samplePlayer()
  {
    Module * m = findModule("sampleplayer");

    if(m)
      return dynamic_cast<ModuleSamplePlayer *>(m);

    Resonant::DSPNetwork::Item item;
    Resonant::ModuleSamplePlayer * player = new Resonant::ModuleSamplePlayer(0);
    item.setModule(player);
    player->setId("sampleplayer");
    item.setUsePanner(false);

    Radiant::BinaryData control;
    control.writeInt32(static_cast<int32_t> (outChannels()));
    control.rewind();

    player->processMessage("channels", control);

    addModule(item);

    return player;
  }

  void DSPNetwork::dumpInfo(FILE *f)
  {
    info("DSPNetwork::dumpInfo # %p", f);
    Radiant::BinaryData control;

    control.writeString("/self/dump_info");
    control.writeInt64((int64_t) f);

    send(control);
  }

  int DSPNetwork::callback(const void *in, void *out,
                           unsigned long framesPerBuffer, int streamnum,
                           const PaStreamCallbackTimeInfo & time,
                           unsigned long flags)
  {
    (void) in;

    size_t streams = m_d->m_streams.size();

    Radiant::TimeStamp outputTime;
    double latency;

    const bool outputError = (flags & paOutputUnderflow) || (flags & paOutputOverflow);

    if(flags & paOutputUnderflow) {
      static int s_underflowWarnings = 0;
      if(++s_underflowWarnings == 20) {
        Radiant::warning("DSPNetwork::callback # Too many output underflow warnings, ignoring them in future");
      } else if(s_underflowWarnings < 20) {
        Radiant::warning("DSPNetwork::callback # output underflow");
      }
    }
    if(flags & paOutputOverflow) {
      Radiant::warning("DSPNetwork::callback # output overflow");
    }

    // We either have multiple streams or have a broken implementation
    // (Linux/Pulseaudio). In that case we just generate the timing information
    // ourselves. In this case we also guess that the latency is ~30 ms
    // (on Linux/Alsa it seems to be ~20-30 ms when having some cheap hw)
    if(streams > 1 || time.outputBufferDacTime == 0) {
      latency = 0.030;
      /// @todo shouldn't hardcode 44100
      if(m_syncinfo.baseTime == 0 /*|| m_syncinfo.framesProcessed > 44100 * 5*/ ||
         outputError) {
        m_syncinfo.baseTime = Radiant::TimeStamp(Radiant::TimeStamp::getTime()) +
            Radiant::TimeStamp::createSecondsD(latency);
        outputTime = m_syncinfo.baseTime;
        m_syncinfo.framesProcessed = 0;
      } else {
        outputTime = m_syncinfo.baseTime + Radiant::TimeStamp::createSecondsD(
              m_syncinfo.framesProcessed / 44100.0);
      }
    } else {
      latency = time.outputBufferDacTime - time.currentTime;
      if(m_syncinfo.baseTime == 0 || m_syncinfo.framesProcessed > 44100 * 60 ||
         outputError) {
        m_syncinfo.baseTime = Radiant::TimeStamp(Radiant::TimeStamp::getTime()) +
            Radiant::TimeStamp::createSecondsD(latency - time.outputBufferDacTime);
        m_syncinfo.framesProcessed = 0;
      }
      outputTime = m_syncinfo.baseTime + Radiant::TimeStamp::
          createSecondsD(time.outputBufferDacTime);
    }
    m_syncinfo.framesProcessed += framesPerBuffer;

    // Radiant::info("Latency: %.2lf ms, Diff from getTime: %.2lf ms",
    //               latency*1000.0, (Radiant::TimeStamp(Radiant::TimeStamp::getTime()).secondsD()-outputTime.secondsD()+latency)*1000.0);

    /// Here we assume that every stream (== audio device) is running in its
    /// own separate thread, that is, this callback is called from multiple
    /// different threads at the same time, one for each audio device.
    /// The first thread is responsible for filling the buffer
    /// (m_collect->interleaved()) by calling doCycle. This thread first waits
    /// until all other threads have finished processing the previous data,
    /// then runs the next cycle and informs everyone else that they can continue
    /// running from the barrier.
    /// We also assume, that framesPerBuffer is somewhat constant in different
    /// threads at the same time.
    if(streams == 1) {
      doCycle(framesPerBuffer, CallbackTime(outputTime, latency, flags));
    } else if(streamnum == 0) {
      m_d->m_sem.acquire(static_cast<int> (streams));
      doCycle(framesPerBuffer, CallbackTime(outputTime, latency, flags));
      for (size_t i = 1; i < streams; ++i)
        m_d->m_streams[i].m_barrier->release();
    } else {
      m_d->m_streams[streamnum].m_barrier->acquire();
    }

    int outChannels = m_d->m_streams[streamnum].outParams.channelCount;

    const float * res = m_collect->interleaved();
    if(res != 0) {
      for (Channels::iterator it = m_d->m_channels.begin(); it != m_d->m_channels.end(); ++it) {
        if (streamnum != it->second.device) continue;
        int from = it->first;
        int to = it->second.channel;

        const float * data = res + from;
        float* target = (float*)out;
        target += to;

        size_t chans_from = m_collect->channels();

        for (size_t i = 0; i < framesPerBuffer; ++i) {
          *target = *data;
          target += outChannels;
          data += chans_from;
        }
      }
      //memcpy(out, res, 4 * framesPerBuffer * outChannels);
    }
    else {
      error("DSPNetwork::callback # No data to play");
      bzero(out, 4 * framesPerBuffer * outChannels);
    }
    if(streams > 1) m_d->m_sem.release();

    m_frames += framesPerBuffer;

    if(m_frames < 40000) {
      debugResonant("DSPNetwork::callback # %lu", framesPerBuffer);
    }

    return paContinue;
  }

  void DSPNetwork::doCycle(int framesPerBuffer, const CallbackTime & time)
  {
    const int cycle = framesPerBuffer;

    checkNewItems();
    checkNewControl();

    for(iterator it = m_items.begin(); it != m_items.end(); it++) {
      Item & item = (*it);
      /*
         Module * m = item.m_module;
         trace("DSPNetwork::doCycle # Processing %p %s", m, typeid(*m).name());
         */
      item.process(cycle, time);
    }

    checkDoneItems();

  }

  void DSPNetwork::checkNewControl()
  {
    {
      Radiant::Guard g( m_inMutex);
      m_incopy = m_incoming;
      m_incoming.rewind();
    }

    int sentinel = m_incopy.pos();

    m_incopy.rewind();

    while(m_incopy.pos() < sentinel) {
      char buf[512];

      std::string id;
      id.reserve(512);
      buf[0] = 0;

      if(!m_incopy.readString(buf, 512)) {
        error("DSPNetwork::checkNewControl # Could not read string");
        continue;
      }

      if(strncmp(buf, "/self/", 6) == 0) {
        const char * name = buf + 6;
        if(strcmp(name, "dump_info") == 0) {
          FILE * f = (FILE *) m_incopy.readInt64();
          duDumpInfo(f);
        }
      }

      const char * slash = strchr(buf, '/');
      const char * command;

      if(!slash) {
        id = buf;
        command = 0;
      }
      else {
        id.assign(buf, slash - buf);
        command = slash + 1;
      }

      deliverControl(id.c_str(), command, m_incopy);
    }
  }

  void DSPNetwork::checkNewItems()
  {
    if(!m_newItems.empty()) {
      debugResonant("DSPNetwork::checkNewItems # Now %d items, adding %d, buffer memory %ld byes",
           (int) m_items.size(), (int) m_newItems.size(),
           countBufferBytes());
    }

    char buf[128];

    while(m_newItems.size()) {

      debugResonant("DSPNetwork::checkNewItems # Next ");

      if(!m_newMutex.tryLock())
        return;

      Radiant::ReleaseGuard g( m_newMutex);

      Item item = m_newItems.front();
      checkValidId(item);
      m_newItems.pop_front();
      m_items.push_front(item);
      const char * type = typeid(* item.m_module).name();

      Item * itptr = & (*m_items.begin());

      if(!compile(*itptr, 0)) {
        error("DSPNetwork::checkNewItems # Could not add module %s", type);
        m_items.pop_front();
      }
      else {
        debugResonant("DSPNetwork::checkNewItems # Added a new module %s", type);

        if(itptr->m_module == m_collect)
          continue;

        const QString & id = itptr->m_module->id();

        int mchans = (int) itptr->m_outs.size();
        int tchan  = itptr->m_targetChannel;
        size_t outchans = m_collect->channels(); // hardware output channels

        if(m_panner && itptr->usePanner()) {
          //info("Adding %d inputs to the panner", mchans);

          Item * oi = findItem(m_panner->id());
          for(int i = 0; i < mchans; i++) {

            Connection conn;
            conn.setModuleId(id);
            conn.m_channel = i % mchans;
            oi->m_inputs.push_back(conn);

            m_controlData.rewind();
            snprintf(buf, sizeof(buf), "%s-%d", id.toUtf8().data(), i);
            m_controlData.writeString(buf);
            m_controlData.rewind();

            m_panner->processMessage("addsource", m_controlData);
          }
          compile( * oi);

          continue;
        }

        ModulePanner * panner = dynamic_cast<ModulePanner *>(itptr->m_module);

        if(panner) {
          m_panner = panner;
        }


        Item * oi = findItem(m_collect->id());

        if(!oi)
          Radiant::fatal("DSPNetwork::checkNewItems # No collector \"%s\"",
                           m_collect->id().toUtf8().data());

        if(mchans && tchan >= 0) {

          for(int i = 0; i < mchans; i++) {
            Connection conn;
            conn.setModuleId(id);
            conn.m_channel = i;
            oi->m_inputs.push_back(conn);

            m_controlData.rewind();
            m_controlData.writeString(id); // Source id
            m_controlData.writeInt32(i);// Source module output channel
            m_controlData.writeInt32(i + tchan); // Target channels
            m_controlData.rewind();
            m_collect->processMessage("newmapping", m_controlData);
          }
          compile( * oi);
          debugResonant("DSPNetwork::checkNewItems # Compiled out collector");
        }
        else if(mchans) {

          /* Heuristically add mappings for the new module, so that it is
             heard. Realistically, this behavior should be overridable
             as needed, now one cannot really make too clever DSP
             networks.
             */

          for(size_t i = 0; i < outchans; i++) {
            Connection conn;
            conn.setModuleId(id);
            conn.m_channel = i % mchans;
            oi->m_inputs.push_back(conn);

            m_controlData.rewind();
            m_controlData.writeString(id); // Source id
            m_controlData.writeInt32(i % mchans);// Source module output channel
            m_controlData.writeInt32(i % int(outchans)); // Target channels
            m_controlData.rewind();
            m_collect->processMessage("newmapping", m_controlData);
          }
          compile( * oi);
          debugResonant("DSPNetwork::checkNewItems # Compiled out collector");
        }
      }
    }

  }

  void DSPNetwork::checkDoneItems()
  {
    if(!m_newMutex.tryLock())
      return;

    Radiant::ReleaseGuard g( m_newMutex);

    if(!m_doneCount)
      return;

    char buf[128];

    for(iterator it = m_items.begin(); it != m_items.end(); ) {

      Item & item = (*it);

      if(item.m_done) {

        for(unsigned i = 0; i < item.m_outs.size() && m_panner; i++) {

          m_controlData.rewind();
          snprintf(buf, sizeof(buf), "%s-%d", item.m_module->id().toUtf8().data(), i);
          m_controlData.writeString(buf);
          m_controlData.rewind();

          m_panner->processMessage("removesource", m_controlData);

          Item * oi = findItem(m_panner->id());
          oi->eraseInputs(item.m_module->id());
        }

        uncompile(item);

        debugResonant("DSPNetwork::checkDoneItems # Stopped %p (%ld bufferbytes)",
             item.m_module, countBufferBytes());

        item.m_module->stop();
        item.deleteModule();

        iterator tmp = it;
        it++;
        m_items.erase(tmp);
      }
      else
        it++;
    }

    m_doneCount = 0;
  }

  void DSPNetwork::deliverControl(const QString & moduleid,
      const char * commandid,
      Radiant::BinaryData & data)
  {
    debugResonant("DSPNetwork::deliverControl # %p %s %s %d", this, moduleid.toUtf8().data(), commandid,
          data.total());

    for(iterator it = m_items.begin(); it != m_items.end(); it++) {
      Module * m = (*it).m_module;
      if(m->id() == moduleid) {
        m->processMessage(commandid, data);
        return;
      }
    }
    error("DSPNetwork::deliverControl # No module \"%s\"", moduleid.toUtf8().data());
  }


  bool DSPNetwork::uncompile(Item & item)
  {
    Module * m = item.m_module;

    if(m == m_collect)
      return true;

    int mchans = (int) item.m_outs.size();
    // int outchans = 2; // hardware output channels

    if(mchans) {
      Item * oi = findItem(m_collect->id());

      if(!oi)
        Radiant::trace(FATAL, "DSPNetwork::checkNewItems # No collector \"%s\"",
            m_collect->id().toUtf8().data());

      m_controlData.rewind();
      m_controlData.writeString(m->id());
      m_controlData.rewind();
      m_collect->processMessage("removemappings", m_controlData);

      oi->removeInputsFrom(m->id());

      compile( * oi);
      debugResonant("DSPNetwork::uncompile # uncompiled \"%s\"", m->id().toUtf8().data());
    }

    return true;
  }

  bool DSPNetwork::compile(Item & item)
  {
    int i = 0;

    for(iterator it = m_items.begin(); it != m_items.end(); it++) {
      if(& (*it) == & item)
        return compile(item, i);
      i++;
    }

    Radiant::error("DSPNetwork::compile # Failed to find something to compile");

    return false;
  }

  bool DSPNetwork::compile(Item & item, int location)
  {
    int i = 0;
    int ins = (int) item.m_inputs.size();
    int outs = ins;

    std::list<NewConnection>::iterator conit;

    std::list<Item *> affected;

    for(conit = item.m_connections.begin(); conit != item.m_connections.end();
        conit++) {
      NewConnection & nc = *conit;
      if(nc.m_targetId == item.m_module->id()) {
        item.m_inputs.push_back(Connection(nc.m_sourceId,
                       nc.m_sourceChannel));
        debugResonant("Item[%d].m_inputs[%d] = [%s,%d]", location, i,
            nc.m_sourceId.toUtf8().data(), nc.m_sourceChannel);
      }
      i++;
    }

    item.m_module->prepare(ins, outs);

    if(ins != (int) item.m_inputs.size()) {
      fatal("DSPNetwork::compile # input size mismatch %d != %d",
        ins, (int) item.m_inputs.size());
    }

    item.m_ins.resize(ins);

    if(!item.m_ins.empty())
      bzero( & item.m_ins[0], item.m_ins.size() * sizeof(float *));

    if(item.m_outs.size() > (unsigned) outs) {
      item.m_outs.resize(outs);
    }
    else {
      while(item.m_outs.size() < (unsigned) outs) {
        item.m_outs.push_back(0);
      }
    }

    for(i = 0; i < ins; i++) {
      Connection & conn = item.m_inputs[i];
      float * ptr = findOutput(conn.m_moduleId, conn.m_channel);
      item.m_ins[i] = ptr;
      debugResonant("Item[%d].m_ins[%d] = %p from %s:%d", location, i, ptr,
            conn.m_moduleId.toUtf8().data(), conn.m_channel);
    }

    for(i = 0; i < outs; i++) {
      if(item.m_outs[i] == 0) {
        Buf & b = findFreeBuf(location);
        item.m_outs[i] = b.m_data;
        debugResonant("Item[%d].m_outs[%d] = %p", location, i, b.m_data);
      }
    }

    item.m_compiled = true;

    Module * m = item.m_module;

    debugResonant("DSPNetwork::compile # compiled %p %s", m, typeid(*m).name());

    return true;
  }

  DSPNetwork::Buf & DSPNetwork::findFreeBuf(int location)
  {
    size_t s = m_buffers.size();

    for(size_t i = 0; i < s; i++) {
      if(bufIsFree((int) i, location)) {
        debugResonant("DSPNetwork::findFreeBuf # Found %d -> %lu", location, i);
        return m_buffers[i];
      }
    }

    m_buffers.resize(s + 1);

    m_buffers[s].init();

    debugResonant("DSPNetwork::findFreeBuf # Created %d -> %lu", location, s);

    return m_buffers[s];
  }

  bool DSPNetwork::bufIsFree(int channel, int location)
  {
    iterator it = m_items.begin();
    int i;

    for(i = 0; i <= location; i++)
      it++;

    float * ptr = m_buffers[channel].m_data;

    if((*it).findInOutput(ptr) >= 0)
      return false;

    iterator other = it;

    for(size_t i = 0; i < m_items.size(); i++) {
      other++;
      if(other == m_items.end())
        other = m_items.begin();

      Item & item = (*other);

      if(item.findInInput(ptr) >= 0)
        return false;
      else if(item.findInOutput(ptr) >= 0)
        return false;
    }

    return true;
  }

  DSPNetwork::Item * DSPNetwork::findItem(const QString & id)
  {
    for(iterator it = m_items.begin(); it != m_items.end(); it++) {
      Item & item = (*it);
      if(item.m_module->id() == id) {
        return & item;
      }
    }
    return 0;
  }

  void DSPNetwork::checkValidId(Item & it)
  {
    char buf[32];
    int index = 0;

    Module * m = it.m_module;

    if(m->id().isEmpty()) {
      sprintf(buf, "%p", m);
      m->setId(buf);
      index++;
    }

    while(findItem(m->id())) {
      if(!index)
        sprintf(buf, "%p", m);
      else
        sprintf(buf, "%p-%.4d", m, index);
      m->setId(buf);
      index++;
    }
  }

  Module * DSPNetwork::findModule(const QString & id)
  {
    Item * item = findItem(id);

    if(!item)
      return 0;

    return item->m_module;
  }

  float * DSPNetwork::findOutput(const QString & id, int channel)
  {
    Item * item = findItem(id);

    if(!item)
      return 0;

    if(item->m_outs.size() >= (size_t) channel)
      return item->m_outs[channel];
    return 0;
  }

  long DSPNetwork::countBufferBytes()
  {
    long bytes = 0;

    for(unsigned i = 0; i < m_buffers.size(); i++) {
      bytes += m_buffers[i].m_size * sizeof(float);
      bytes += sizeof(Buf);
    }

    return bytes;
  }

  void DSPNetwork::duDumpInfo(FILE *f)
  {
    if(!f)
      f = stdout;

    fprintf(f, "DSPNetwork %p on frame %ld\n", this, m_frames);

    int index = 0;
    for(container::iterator it = m_items.begin(); it != m_items.end(); it++) {
      Item & item = *it;

      fprintf(f, "  DSP ITEM [%d] %s %s %p\n",
              index, item.m_module->id().toUtf8().data(), typeid(*item.m_module).name(), item.m_module);

      for(size_t i = 0; i < item.m_ins.size(); i++) {
        fprintf(f, "    INPUT PTR [%d] %p\n", (int) i, item.m_ins[i]);
      }
      for(size_t i = 0; i < item.m_outs.size(); i++) {
        fprintf(f, "    OUTPUT PTR [%d] %p\n", (int) i, item.m_outs[i]);
      }

      index++;
    }
  }

}

DEFINE_SINGLETON(Resonant::DSPNetwork);
