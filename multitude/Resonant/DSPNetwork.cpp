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

#include "ModulePanner.hpp"
#include "ModuleOutCollect.hpp"
#include "ModuleSamplePlayer.hpp"

#include <Radiant/FixedStr.hpp>
#include <Radiant/Trace.hpp>

#include <algorithm>
#include <typeinfo>

#include <portaudio.h>


namespace Resonant {



  using namespace Radiant;

  DSPNetwork::Item::Item()
    : m_module(0),
      m_compiled(false),
      m_done(false),
      m_targetChannel(-1)
  {

  }

  DSPNetwork::Item::~Item()
  {}

  void DSPNetwork::Item::eraseInput(const Connection & c)
  {
    std::vector<Connection>::iterator it =
      std::find(m_inputs.begin(), m_inputs.end(), c);

    if(it != m_inputs.end())
      m_inputs.erase(it);
  }

  void DSPNetwork::Item::eraseInputs(const std::string & moduleId)
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
    for(uint i = 0; i < m_ins.size(); i++)
      if(m_ins[i] == ptr)
        return i;
    return -1;
  }

  int DSPNetwork::Item::findInOutput(float * ptr) const
  {
    for(uint i = 0; i < m_outs.size(); i++)
      if(m_outs[i] == ptr)
        return i;
    return -1;
  }

  void DSPNetwork::Item::removeInputsFrom(const char * id)
  {
    for(std::list<NewConnection>::iterator it = m_connections.begin();
    it != m_connections.end(); ) {

      if(strcmp(id, (*it).m_sourceId) == 0) {
    it = m_connections.erase(it);
      }
      else
    it++;
    }

    for(unsigned i = 0; i < m_inputs.size(); ) {
      if(strcmp(id, m_inputs[i].m_moduleId) == 0) {
    m_inputs.erase(m_inputs.begin() + i);
    m_ins.erase(m_ins.begin() + i);
      }
      else
    i++;
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////


  DSPNetwork * DSPNetwork::m_instance = 0;

  DSPNetwork::DSPNetwork()
    : // m_continue(false),
    m_panner(0),
    m_doneCount(0)
  {
    m_devName[0] = 0;
    m_collect = new ModuleOutCollect(0, this);
    m_collect->setId("outcollect");

    Item tmp;
    tmp.m_module = m_collect;

    m_newItems.push_back(tmp);

    if(!m_instance)
      m_instance = this;
  }

  DSPNetwork::~DSPNetwork()
  {
    if(m_instance == this)
      m_instance = 0;

    for(uint i = 0; i < m_buffers.size(); i++)
      m_buffers[i].clear();

    delete m_collect;
    m_collect = 0;
  }

  bool DSPNetwork::start(const char * device)
  {
    if(isRunning())
      return false;

    if(device)
      strcpy(m_devName, device);
    else
      m_devName[0] = '\0';

    // m_continue = true;

    return startReadWrite(44100, 2);
  }


  void DSPNetwork::addModule(Item & i)
  {
    Radiant::Guard g( & m_newMutex);

    m_newItems.push_back(i);
  }

  void DSPNetwork::markDone(Item & i)
  {
    Radiant::Guard g( & m_newMutex);
    Item * it = findItem(i.m_module->id());

    if(it) {
      it->m_done = true;
      m_doneCount++;
    }
    else
      error("DSPNetwork::markDone # Failed for \"%s\"", i.m_module->id());
  }

  void DSPNetwork::send(Radiant::BinaryData & control)
  {
    Radiant::Guard g( & m_inMutex);
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

    Radiant::BinaryData control;
    control.writeInt32(outChannels());
    control.rewind();

    player->processMessage("channels", & control);

    addModule(item);

    return player;
  }

  DSPNetwork * DSPNetwork::instance()
  {
    if(!m_instance)
      return 0;

    if(!m_instance->isRunning()) {
      debug("DSPNetwork::instance # Initializing DSP...");
      if(!m_instance->start())
        Radiant::error("DSPNetwork::instance # failed to initialize sound device");
    }
    return m_instance;
  }

  int DSPNetwork::callback(const void *in, void *out,
      unsigned long framesPerBuffer)
  {
    (void) in;

    doCycle(framesPerBuffer);
    const float * res = m_collect->interleaved();
    assert(res != 0);
    memcpy(out, res, 4 * framesPerBuffer * outChannels());

    return paContinue;
  }

  void DSPNetwork::doCycle(int framesPerBuffer)
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
      item.process(cycle);
    }

    checkDoneItems();

  }

  void DSPNetwork::checkNewControl()
  {
    {
      Radiant::Guard g( & m_inMutex);
      m_incopy = m_incoming;
      m_incoming.rewind();
    }

    int sentinel = m_incopy.pos();

    m_incopy.rewind();

    char buf[512];

    FixedStrT<512> id;

    while(m_incopy.pos() < sentinel) {
      buf[0] = 0;

      if(!m_incopy.readString(buf, 512)) {
        error("DSPNetwork::checkNewControl # Could not read string");
        continue;
      }

      const char * slash = strchr(buf, '/');
      const char * command;

      if(!slash) {
        id = buf;
        command = 0;
      }
      else {
        id.copyn(buf, slash - buf);
        command = slash + 1;
      }

      deliverControl(id, command, m_incopy);
    }
  }

  void DSPNetwork::checkNewItems()
  {
    if(m_newItems.size()) {
      debug("DSPNetwork::checkNewItems # Now %d items, adding %d, buffer memory %ld byes",
           (int) m_items.size(), (int) m_newItems.size(),
           countBufferBytes());
    }

    char buf[128];

    while(m_newItems.size()) {

      debug("DSPNetwork::checkNewItems # Next ");

      if(!m_newMutex.tryLock())
        return;

      Radiant::ReleaseGuard g( & m_newMutex);

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
        debug("DSPNetwork::checkNewItems # Added a new module %s", type);

        if(itptr->m_module == m_collect)
          continue;

        const char * id = itptr->m_module->id();

        int mchans = itptr->m_outs.size();
        int tchan  = itptr->m_targetChannel;
        int outchans = m_collect->channels(); // hardware output channels

        if(m_panner) {
          info("Adding %d inputs to the panner", mchans);

          Item * oi = findItem(m_panner->id());
          for(int i = 0; i < mchans; i++) {

            Connection conn;
            conn.setModuleId(id);
            conn.m_channel = i % mchans;
            oi->m_inputs.push_back(conn);

            m_controlData.rewind();
            sprintf(buf, "%s-%d", id, i);
            m_controlData.writeString(buf);
            m_controlData.rewind();

            m_panner->processMessage("addsource", & m_controlData);
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
                           m_collect->id());

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
            m_collect->processMessage("newmapping", & m_controlData);
          }
          compile( * oi);
          debug("DSPNetwork::checkNewItems # Compiled out collector");
        }
        else if(mchans) {

          /* Heuristically add mappings for the new module, so that it is
             heard. Realistically, this behavior should be overridable
             as needed, now one cannot really make too clever DSP
             networks.
             */

          for(int i = 0; i < outchans; i++) {
            Connection conn;
            conn.setModuleId(id);
            conn.m_channel = i % mchans;
            oi->m_inputs.push_back(conn);

            m_controlData.rewind();
            m_controlData.writeString(id); // Source id
            m_controlData.writeInt32(i % mchans);// Source module output channel
            m_controlData.writeInt32(i % outchans); // Target channels
            m_controlData.rewind();
            m_collect->processMessage("newmapping", & m_controlData);
          }
          compile( * oi);
          debug("DSPNetwork::checkNewItems # Compiled out collector");
        }
      }
    }

  }

  void DSPNetwork::checkDoneItems()
  {
    if(!m_newMutex.tryLock())
      return;

    Radiant::ReleaseGuard g( & m_newMutex);

    if(!m_doneCount)
      return;

    char buf[128];

    for(iterator it = m_items.begin(); it != m_items.end(); ) {

      Item & item = (*it);

      if(item.m_done) {

        for(unsigned i = 0; i < item.m_outs.size() && m_panner; i++) {

          m_controlData.rewind();
          sprintf(buf, "%s-%d", item.m_module->id(), i);
          m_controlData.writeString(buf);
          m_controlData.rewind();

          m_panner->processMessage("removesource", & m_controlData);

          Item * oi = findItem(m_panner->id());
          oi->eraseInputs(item.m_module->id());
        }

        uncompile(item);

        debug("DSPNetwork::checkDoneItems # Stopped %p (%ld bufferbytes)",
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

  void DSPNetwork::deliverControl(const char * moduleid,
      const char * commandid,
      Radiant::BinaryData & data)
  {
    for(iterator it = m_items.begin(); it != m_items.end(); it++) {
      Module * m = (*it).m_module;
      if(strcmp(m->id(), moduleid) == 0) {
        m->processMessage(commandid, & data);
        return;
      }
    }
    error("DSPNetwork::deliverControl # No module \"%s\"", moduleid);
  }


  bool DSPNetwork::uncompile(Item & item)
  {
    Module * m = item.m_module;

    if(m == m_collect)
      return true;

    int mchans = item.m_outs.size();
    // int outchans = 2; // hardware output channels
    const char * id = m->id();

    if(mchans) {
      Item * oi = findItem(m_collect->id());

      if(!oi)
        Radiant::trace(FATAL, "DSPNetwork::checkNewItems # No collector \"%s\"",
            m_collect->id());

      m_controlData.rewind();
      m_controlData.writeString(id);
      m_controlData.rewind();
      m_collect->processMessage("removemappings", & m_controlData);

      oi->removeInputsFrom(id);

      compile( * oi);
      debug("DSPNetwork::uncompile # uncompiled \"%s\"", id);
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
    int ins = item.m_inputs.size();
    int outs = ins;

    std::list<NewConnection>::iterator conit;

    std::list<Item *> affected;

    for(conit = item.m_connections.begin(); conit != item.m_connections.end();
        conit++) {
      NewConnection & nc = *conit;
      if(strcmp(nc.m_targetId, item.m_module->id()) == 0) {
        item.m_inputs.push_back(Connection(nc.m_sourceId,
                       nc.m_sourceChannel));
        debug("Item[%d].m_inputs[%d] = [%d,%d]", location, i,
            nc.m_sourceId, nc.m_sourceChannel);
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
      debug("Item[%d].m_ins[%d] = %p from %s:%d", location, i, ptr,
            conn.m_moduleId, conn.m_channel);
    }

    for(i = 0; i < outs; i++) {
      if(item.m_outs[i] == 0) {
        Buf & b = findFreeBuf(location);
        item.m_outs[i] = b.m_data;
        debug("Item[%d].m_outs[%d] = %p", location, i, b.m_data);
      }
    }

    item.m_compiled = true;

    Module * m = item.m_module;

    debug("DSPNetwork::compile # compiled %p %s", m, typeid(*m).name());

    return true;
  }

  DSPNetwork::Buf & DSPNetwork::findFreeBuf(int location)
  {
    uint s = m_buffers.size();

    for(uint i = 0; i < s; i++) {
      if(bufIsFree(i, location)) {
        debug("DSPNetwork::findFreeBuf # Found %d -> %u", location, i);
        return m_buffers[i];
      }
    }

    m_buffers.resize(s + 1);

    m_buffers[s].init();

    debug("DSPNetwork::findFreeBuf # Created %d -> %u", location, s);

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

    for(uint i = 0; i < m_items.size(); i++) {
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

  DSPNetwork::Item * DSPNetwork::findItem(const char * id)
  {
    for(iterator it = m_items.begin(); it != m_items.end(); it++) {
      Item & item = (*it);
      if(strcmp(item.m_module->id(), id) == 0) {
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

    if(strlen(m->id()) == 0) {
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

  Module * DSPNetwork::findModule(const char * id)
  {
    Item * item = findItem(id);

    if(!item)
      return 0;

    return item->m_module;
  }

  float * DSPNetwork::findOutput(const char * id, int channel)
  {
    Item * item = findItem(id);

    if(!item)
      return 0;

    if(item->m_outs.size() >= (uint) channel)
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

}
