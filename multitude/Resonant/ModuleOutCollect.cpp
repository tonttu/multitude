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

#include "ModuleOutCollect.hpp"
#include "Resonant.hpp"

#include "DSPNetwork.hpp"

#include <Nimble/Random.hpp>

#include <Radiant/BinaryData.hpp>
#include <Radiant/Trace.hpp>

#include <algorithm>

#include <strings.h>

namespace Resonant {

  using Radiant::debug;
  using Radiant::info;
  using Radiant::error;

  ModuleOutCollect::ModuleOutCollect(Application * a , DSPNetwork * host)
    : Module(a),
    m_subwooferChannel(-1),
    m_host(host)
  {
    /* A hack to get the subwoofer channel information in without message passing etc. */
    const char * subenv = getenv("RESONANT_SUBWOOFER");
    if(subenv) {
      m_subwooferChannel = atoi(subenv);
      info("ModuleOutCollect::ModuleOutCollect # Subwoofer channel set to %d",
           m_subwooferChannel);
    }
  }

  ModuleOutCollect::~ModuleOutCollect()
  {}

  bool ModuleOutCollect::prepare(int & channelsIn, int & channelsOut)
  {
    channelsOut = 0;

    channelsIn = (int) m_map.size();
    m_channels = m_host->outChannels();

    /* For debugging purposes you can override (=expand) the number of
       output channels. */
    const char * forcechans = getenv("RESONANT_FORCE_CHANNELS");
    if(forcechans) {
      m_channels =  atoi(forcechans);
      Radiant::info("ModuleOutCollect::prepare # forcing channel count to %ld",
                    m_channels);
    }

    assert(m_channels != 0);

    m_interleaved.resize(m_channels * MAX_CYCLE);

    debugResonant("ModuleOutCollect::prepare # %d", (int) m_channels);

    return true;
  }

  void ModuleOutCollect::processMessage(const QString & id, Radiant::BinaryData & control)
  {
    bool ok = true;
    Move tmp;

    ok = control.readString(tmp.sourceId);

    // info("ModuleOutCollect::control # Now %d sources in the map", (int) m_map.size());

    if(id == "subwooferchannel") {
      m_subwooferChannel = control.readInt32();
    }
    if(id == "removemappings") {
      // Remove all the mappings that match the given input.

      for(iterator it = m_map.begin(); it != m_map.end(); ) {
        if(tmp.sourceId == it->sourceId) {
          debugResonant("ModuleOutCollect::control # dropping connection to %s:%d",
                        tmp.sourceId.toUtf8().data(), (*it).from);
          it = m_map.erase(it);
        }
        else
          it++;
      }
    }
    else {
      tmp.from = control.readInt32( & ok);
      tmp.to   = control.readInt32( & ok);

      debugResonant("ModuleOutCollect::control # %s", id.toUtf8().data());

      if(!ok) {
        error("ModuleOutCollect::control # Could not parse control # %s",
              tmp.sourceId.toUtf8().data());
        return;
      }
      else if(id == "newmapping") {
        m_map.push_back(tmp);
        debugResonant("ModuleOutCollect::control # newmapping %s %d -> %d",
                      tmp.sourceId.toUtf8().data(), tmp.from, tmp.to);
      }
      else if(id == "removemapping") {
        iterator it = std::find(m_map.begin(), m_map.end(), tmp);

        if(it != m_map.end()) {
          m_map.erase(it);
        }
        else
          error("ModuleOutCollect::control # Could not erase mapping # %s:%d -> %d",
                tmp.sourceId.toUtf8().data(), tmp.from, tmp.to);
      }
      else {
        error("ModuleOutCollect::control # No param \"%s\"", id.toUtf8().data());
      }
    }
  }

  void ModuleOutCollect::process(float ** in, float **, int n)
  {
    size_t chans = m_channels;

    assert(m_interleaved.size() >= (n * chans));

    // Set to zero
    if(!m_interleaved.empty())
      bzero( & m_interleaved[0], sizeof(float) * n * chans);

    for(size_t i = 0; i < m_map.size(); i++) {

      int to = m_map[i].to;

      const float * src = in[i];
      const float * sentinel = src + n;

      if(!src)
        continue; // Should output a warning ;-)

      float * dest = & m_interleaved[to];

      /* if(i < 2)
        info("ModuleOutCollect::process # %p %d %f", src, i, src[0]);
      */

      while(src < sentinel) {
        *dest += * src;

        src++;
        dest += chans;
      }

      if(m_subwooferChannel >= 0 && m_subwooferChannel < static_cast<int> (chans)) {
        src = in[i];
        dest = & m_interleaved[m_subwooferChannel];

        while(src < sentinel) {
          *dest += * src;

          src++;
          dest += chans;
        }
      }

    }

    /*
    static Nimble::RandomUniform __r;

    uint nn = chans * n;
    for(uint i = 0; i < nn; i++)
      m_interleaved[i] = __r.rand11();
    */
  }


}
