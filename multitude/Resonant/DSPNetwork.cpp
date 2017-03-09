/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "DSPNetwork.hpp"
#include "Resonant.hpp"

#ifdef CORNERSTONE_ENABLE_PORT_AUDIO
#include "AudioLoopPortAudio.hpp"
#endif

#ifdef CORNERSTONE_ENABLE_PULSE
#include "AudioLoopPulseAudio.hpp"
#endif

#include "ModulePanner.hpp"
#include "ModuleOutCollect.hpp"
#include "ModuleSamplePlayer.hpp"

#include <Radiant/Trace.hpp>

#include <algorithm>
#include <typeinfo>
#include <cstdio>

namespace Resonant {

  DSPNetwork::Item::Item()
    : m_module(0),
      m_compiled(false),
      m_done(false),
      m_usePanner(true),
      m_targetChannel(-1)
  {}

  DSPNetwork::Item::~Item()
  {}

  void DSPNetwork::Item::resetModule()
  {
    m_module.reset();
  }

  void DSPNetwork::Item::eraseInput(const Connection & c)
  {
    std::vector<Connection>::iterator it =
      std::find(m_inputs.begin(), m_inputs.end(), c);

    if(it != m_inputs.end())
      m_inputs.erase(it);
  }

  void DSPNetwork::Item::eraseInputs(const QByteArray & moduleId)
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

  void DSPNetwork::Item::removeInputsFrom(const QByteArray & id)
  {
    for(std::list<NewConnection>::iterator it = m_connections.begin();
    it != m_connections.end(); ) {

      if(id == it->m_sourceId) {
        it = m_connections.erase(it);
      }
      else
        ++it;
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
    m_doneCount(0),
    m_itemMutex(true)
  {
    m_collect = std::make_shared<ModuleOutCollect>(this);
    m_collect->setId("outcollect");

    ItemPtr tmp(new Item());
    tmp->m_module = m_collect;

    m_newItems.push_back(tmp);
  }

  DSPNetwork::~DSPNetwork()
  {
    if (m_audioLoop) {
      m_audioLoop->stop();
    }

    for(size_t i = 0; i < m_buffers.size(); i++)
      m_buffers[i].clear();
  }

  bool DSPNetwork::start(AudioLoopBackend backend)
  {
    Radiant::Guard g(m_startupMutex);

    debugResonant("DSPNetwork::start # %p", this);

    if(isRunning())
      return false;

    // m_continue = true;

    int channels = 2;
    if (auto outchannels = getenv("RESONANT_OUTCHANNELS")) {
      channels = atoi(outchannels);
    }

    if (backend == AUDIO_LOOP_PULSE_AUDIO) {
#ifdef CORNERSTONE_ENABLE_PULSE
      m_audioLoop.reset(new AudioLoopPulseAudio(*this, m_collect));
#else
      Radiant::error("DSPNetwork::start # PulseAudio backend was requested but it wasn't included in this build");
      return false;
#endif
    } else {
#ifdef CORNERSTONE_ENABLE_PORT_AUDIO
      m_audioLoop.reset(new AudioLoopPortAudio(*this, m_collect));
#else
      Radiant::error("DSPNetwork::start # PortAudio backend was requested but it wasn't included in this build");
      return false;
#endif
    }
    return m_audioLoop->start(44100, channels);
  }

  void DSPNetwork::addModule(ItemPtr item)
  {
    debugResonant("DSPNetwork::addModule # %p", this);

    Radiant::Guard g( m_newMutex);

    m_newItems.push_back(item);
  }

  void DSPNetwork::markDone(ItemPtr i)
  {
    /// Calling overloaded markDone here, will search itself from items...
    assert(i->module());
    markDone(i->module());
  }

  void DSPNetwork::markDone(ModulePtr module)
  {
    // The order of the guards matters. DSPNetwork::doCycle() locks the
    // mutexes in this order. Since this function is in the public API it can
    // be called any time and if the mutexes are locked in different order a
    // deadlock may occur.
    Radiant::Guard g2( m_itemMutex);
    Radiant::Guard g1( m_newMutex);
    ItemPtr it = findItemUnsafe(module->id());

    if(it) {
      it->m_done = true;
      m_doneCount++;
    } else {
      bool found = false;
      for(auto it = m_newItems.begin(); it != m_newItems.end();) {
        ItemPtr item = *it;
        if(module == item->module()) {
          found = true;
          item->m_module = nullptr;
          it = m_newItems.erase(it);
        } else {
          ++it;
        }
      }

      if (!found) {
        Radiant::error("DSPNetwork::markDone # Failed for \"%s\"", module->id().data());
      }
    }
  }

  void DSPNetwork::send(Radiant::BinaryData & control)
  {
    debugResonant("DSPNetwork::send # %p", this);

    Radiant::Guard g( m_inMutex);
    m_incoming.append(control);
  }

  std::shared_ptr<ModuleSamplePlayer> DSPNetwork::samplePlayer()
  {
    if (!audioLoop()) {
      return nullptr;
    }
    ModulePtr m = findModule("sampleplayer");

    if(m)
      return std::dynamic_pointer_cast<ModuleSamplePlayer>(m);

    auto item = std::make_shared<Resonant::DSPNetwork::Item>();
    auto player = std::make_shared<Resonant::ModuleSamplePlayer>();
    item->setModule(player);
    player->setId("sampleplayer");
    item->setUsePanner(false);

    Radiant::BinaryData control;
    control.writeInt32(static_cast<int32_t> (audioLoop()->outChannels()));
    control.rewind();

    player->eventProcess("channels", control);

    addModule(item);

    return player;
  }

  ModuleSamplePlayer * DSPNetwork::javascriptSamplePlayer()
  {
    return samplePlayer().get();
  }

  void DSPNetwork::dumpInfo(FILE *f)
  {
    Radiant::info("DSPNetwork::dumpInfo # %p", f);
    Radiant::BinaryData control;

    control.writeString("/self/dump_info");
    control.writeInt64((int64_t) f);

    send(control);
  }

  void DSPNetwork::doCycle(int framesPerBuffer, const CallbackTime & time)
  {
    const int cycle = framesPerBuffer;

    std::vector<ModulePtr> modulesToDelete;

    {
      Radiant::Guard g(m_itemMutex);

      checkNewItems();
      checkNewControl();

      for(iterator it = m_items.begin(); it != m_items.end(); ++it) {
        ItemPtr & item = (*it);
        /*
           Module * m = item.m_module;
           trace("DSPNetwork::doCycle # Processing %p %s", m, typeid(*m).name());
           */
        item->process(cycle, time);
      }

      checkDoneItems(modulesToDelete);
    }

    // Deleting modulesToDelete automatically deletes items in modulesToDelete
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
        Radiant::error("DSPNetwork::checkNewControl # Could not read string at %d", m_incopy.pos());
        continue;
      }

      if(strncmp(buf, "/self/", 6) == 0) {
        const char * name = buf + 6;
        if(strcmp(name, "dump_info") == 0) {
          FILE * f = (FILE *) m_incopy.readInt64();
          doDumpInfo(f);
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
    if(!m_newMutex.tryLock())
      return;

    Radiant::ReleaseGuard g( m_newMutex);

    if(m_newItems.empty()) {
      return;
    }
    else {
      debugResonant("DSPNetwork::checkNewItems # Now %d items, adding %d, buffer memory %ld byes",
           (int) m_items.size(), (int) m_newItems.size(),
           countBufferBytes());
    }

    char buf[128];

    while(m_newItems.size()) {

      debugResonant("DSPNetwork::checkNewItems # Next ");

      ItemPtr item = m_newItems.front();
      checkValidId(item);
      m_newItems.pop_front();
      m_items.push_front(item);
      auto & module = *item->m_module;
      const char * type = typeid(module).name();

      if(!compile(item, 0)) {
        Radiant::error("DSPNetwork::checkNewItems # Could not add module %s", type);
        m_items.pop_front();
      }
      else {
        debugResonant("DSPNetwork::checkNewItems # Added a new module %s", type);

        if(item->m_module == m_collect)
          continue;

        const QByteArray & id = item->m_module->id();

        int mchans = (int) item->m_outs.size();
        int tchan  = item->m_targetChannel;
        size_t outchans = m_collect->channels(); // hardware output channels

        if(m_panner && item->usePanner()) {
          //info("Adding %d inputs to the panner", mchans);

          ItemPtr oi = findItemUnsafe(m_panner->id());
          for(int i = 0; i < mchans; i++) {

            Connection conn;
            conn.setModuleId(id);
            conn.m_channel = i % mchans;
            oi->m_inputs.push_back(conn);

            m_controlData.rewind();
            snprintf(buf, sizeof(buf), "%s-%d", id.data(), i);
            m_controlData.writeString(buf);
            m_controlData.rewind();

            m_panner->eventProcess("addsource", m_controlData);
          }
          compile(oi);

          continue;
        }

        std::shared_ptr<ModulePanner> panner = std::dynamic_pointer_cast<ModulePanner>(item->m_module);

        if(panner) {
          m_panner = panner;
        }


        ItemPtr oi = findItemUnsafe(m_collect->id());

        if(!oi)
          Radiant::fatal("DSPNetwork::checkNewItems # No collector \"%s\"",
                           m_collect->id().data());

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
            m_collect->eventProcess("newmapping", m_controlData);
          }
          compile(oi);
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
            m_collect->eventProcess("newmapping", m_controlData);
          }
          compile(oi);
          debugResonant("DSPNetwork::checkNewItems # Compiled out collector");
        }
      }
    }

  }

  void DSPNetwork::checkDoneItems(std::vector<ModulePtr> & modulesToDelete)
  {
    if(!m_newMutex.tryLock())
      return;

    {
      Radiant::ReleaseGuard g( m_newMutex);

      if(!m_doneCount)
        return;

      char buf[128];

      for(iterator it = m_items.begin(); it != m_items.end(); ) {

        ItemPtr item = (*it);

        if(item->m_done) {

          for(unsigned i = 0; i < item->m_outs.size() && m_panner; i++) {

            m_controlData.rewind();
            snprintf(buf, sizeof(buf), "%s-%d", item->m_module->id().data(), i);
            m_controlData.writeString(buf);
            m_controlData.rewind();

            m_panner->eventProcess("removesource", m_controlData);

            ItemPtr oi = findItemUnsafe(m_panner->id());
            oi->eraseInputs(item->m_module->id());
          }

          uncompile(item);

          debugResonant("DSPNetwork::checkDoneItems # Stopped %p (%ld bufferbytes)",
                        item->m_module.get(), countBufferBytes());

          item->m_module->stop();

          modulesToDelete.push_back(item->m_module);
          item->m_module = nullptr;

          it = m_items.erase(it);
        }
        else
          ++it;
      }
      m_doneCount = 0;
    }
  }

  void DSPNetwork::deliverControl(const QByteArray & moduleid,
                                  const QByteArray & commandid,
                                  Radiant::BinaryData & data)
  {
    debugResonant("DSPNetwork::deliverControl # %p %s %s %d", this, moduleid.data(), commandid.data(),
          data.total());

    for(iterator it = m_items.begin(); it != m_items.end(); ++it) {
      ModulePtr m = (*it)->module();
      if(m->id() == moduleid) {
        m->eventProcess(commandid, data);
        return;
      }
    }
    Radiant::error("DSPNetwork::deliverControl # No module \"%s\"", moduleid.data());
  }


  bool DSPNetwork::uncompile(ItemPtr item)
  {
    ModulePtr m = item->m_module;

    if(m == m_collect)
      return true;

    int mchans = (int) item->m_outs.size();
    // int outchans = 2; // hardware output channels

    if(mchans) {
      ItemPtr oi = findItemUnsafe(m_collect->id());
      assert(oi);

      m_controlData.rewind();
      m_controlData.writeString(m->id());
      m_controlData.rewind();
      m_collect->eventProcess("removemappings", m_controlData);

      oi->removeInputsFrom(m->id());

      compile(oi);
      debugResonant("DSPNetwork::uncompile # uncompiled \"%s\"", m->id().data());
    }

    return true;
  }

  bool DSPNetwork::compile(ItemPtr item)
  {
    int i = 0;

    for(iterator it = m_items.begin(); it != m_items.end(); ++it) {
      if((*it) ==  item)
        return compile(item, i);
      i++;
    }

    Radiant::error("DSPNetwork::compile # Failed to find something to compile");

    return false;
  }

  bool DSPNetwork::compile(ItemPtr item, int location)
  {
    int i = 0;
    int ins = (int) item->m_inputs.size();
    int outs = ins;

    std::list<NewConnection>::iterator conit;

    for(conit = item->m_connections.begin(); conit != item->m_connections.end(); ++conit) {
      NewConnection & nc = *conit;
      if(nc.m_targetId == item->m_module->id()) {
        item->m_inputs.push_back(Connection(nc.m_sourceId,
                       nc.m_sourceChannel));
        debugResonant("Item[%d].m_inputs[%d] = [%s,%d]", location, i,
            nc.m_sourceId.data(), nc.m_sourceChannel);
      }
      i++;
    }

    item->m_module->prepare(ins, outs);

    if(ins != (int) item->m_inputs.size()) {
      Radiant::fatal("DSPNetwork::compile # input size mismatch %d != %d",
        ins, (int) item->m_inputs.size());
    }

    item->m_ins.resize(ins);

    if(!item->m_ins.empty())
      memset( & item->m_ins[0], 0, item->m_ins.size() * sizeof(float *));

    if(item->m_outs.size() > (unsigned) outs) {
      item->m_outs.resize(outs);
    }
    else {
      while(item->m_outs.size() < (unsigned) outs) {
        item->m_outs.push_back(0);
      }
    }

    for(i = 0; i < ins; i++) {
      Connection & conn = item->m_inputs[i];
      float * ptr = findOutput(conn.m_moduleId, conn.m_channel);
      item->m_ins[i] = ptr;
      debugResonant("Item[%d].m_ins[%d] = %p from %s:%d", location, i, ptr,
            conn.m_moduleId.data(), conn.m_channel);
    }

    for(i = 0; i < outs; i++) {
      if(item->m_outs[i] == 0) {
        Buf & b = findFreeBuf(location);
        item->m_outs[i] = b.m_data;
        debugResonant("Item[%d].m_outs[%d] = %p", location, i, b.m_data);
      }
    }

    item->m_compiled = true;

    ModulePtr m = item->m_module;
    auto & module = *m;

    debugResonant("DSPNetwork::compile # compiled %p %s", m.get(), typeid(module).name());

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
    std::advance(it, location);

    float * ptr = m_buffers[channel].m_data;

    if((*it)->findInOutput(ptr) >= 0)
      return false;

    iterator other = it;

    for(size_t i = 0; i < m_items.size(); i++) {
      ++other;
      if(other == m_items.end())
        other = m_items.begin();

      ItemPtr item = *other;

      if(item->findInInput(ptr) >= 0)
        return false;
      else if(item->findInOutput(ptr) >= 0)
        return false;
    }

    return true;
  }

  DSPNetwork::ItemPtr DSPNetwork::findItem(const QByteArray & id)
  {
    Radiant::Guard g(m_itemMutex);
    return findItemUnsafe(id);
  }

  DSPNetwork::ItemPtr DSPNetwork::findItemUnsafe(const QByteArray & id)
  {
    for(iterator it = m_items.begin(); it != m_items.end(); ++it) {
      ItemPtr item = (*it);
      if(item->m_module->id() == id) {
        return item;
      }
    }
    return ItemPtr();
  }

  void DSPNetwork::checkValidId(ItemPtr item)
  {
    char buf[32];
    int index = 0;

    ModulePtr m = item->m_module;

    if(m->id().isEmpty()) {
      sprintf(buf, "%p", m.get());
      m->setId(buf);
      index++;
    }

    while(findItemUnsafe(m->id())) {
      if(!index)
        sprintf(buf, "%p", m.get());
      else
        sprintf(buf, "%p-%.4d", m.get(), index);
      m->setId(buf);
      index++;
    }
  }

  ModulePtr DSPNetwork::findModule(const QByteArray & id)
  {
    ItemPtr item = findItem(id);

    if(!item) {
      Radiant::Guard g(m_newMutex);
      for (ItemPtr & item: m_newItems)
        if (item->m_module->id() == id)
          return item->module();

      return 0;
    }

    return item->m_module;
  }

  float * DSPNetwork::findOutput(const QByteArray & id, int channel)
  {
    ItemPtr item = findItemUnsafe(id);

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

  void DSPNetwork::doDumpInfo(FILE *f)
  {
    if(!f)
      f = stdout;

    // fprintf(f, "DSPNetwork %p on frame %ld\n", this, m_frames);

    int index = 0;
    for(container::iterator it = m_items.begin(); it != m_items.end(); ++it) {
      ItemPtr item = *it;
      auto & module = *item->m_module;

      fprintf(f, "  DSP ITEM [%d] %s %s %p\n",
              index, item->m_module->id().data(), typeid(module).name(), item->m_module.get());

      for(size_t i = 0; i < item->m_ins.size(); i++) {
        fprintf(f, "    INPUT PTR [%d] %p\n", (int) i, item->m_ins[i]);
      }
      for(size_t i = 0; i < item->m_outs.size(); i++) {
        fprintf(f, "    OUTPUT PTR [%d] %p\n", (int) i, item->m_outs[i]);
      }

      index++;
    }
  }

}

DEFINE_SINGLETON(Resonant::DSPNetwork);
