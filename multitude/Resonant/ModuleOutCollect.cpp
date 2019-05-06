/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ModuleOutCollect.hpp"

#include "Resonant.hpp"

#include "DSPNetwork.hpp"

#include <Nimble/Random.hpp>

#include <Radiant/BinaryData.hpp>
#include <Radiant/Trace.hpp>

#include <algorithm>

namespace Resonant {

  using Radiant::debug;
  using Radiant::info;
  using Radiant::error;

  ModuleOutCollect::ModuleOutCollect(DSPNetwork * host)
    : m_subwooferChannel(-1)
    , m_compressOutput(true) // By default compress the output, we do not want clipping to happen
    , m_host(host)
  {
    /* A hack to get the subwoofer channel information in without message passing etc. */
    const char * subenv = getenv("RESONANT_SUBWOOFER");
    if(subenv) {
      m_subwooferChannel = atoi(subenv);
      Radiant::info("ModuleOutCollect::ModuleOutCollect # Subwoofer channel set to %d",
           m_subwooferChannel);
    }
  }

  ModuleOutCollect::~ModuleOutCollect()
  {}

  bool ModuleOutCollect::prepare(int & channelsIn, int & channelsOut)
  {
    channelsOut = 0;

    channelsIn = (int) m_map.size();
    m_channels = m_host->audioLoop()->outChannels();

    /* For debugging purposes you can override (=expand) the number of
       output channels. */
    const char * forcechans = getenv("RESONANT_FORCE_CHANNELS");
    if(forcechans) {
      m_channels =  atoi(forcechans);
      Radiant::info("ModuleOutCollect::prepare # forcing channel count to %ld",
                    (long) m_channels);
    }

    assert(m_channels != 0);

    m_lastSample.resize(m_channels);

    debugResonant("ModuleOutCollect::prepare # %d", (int) m_channels);

    return true;
  }

  void ModuleOutCollect::eventProcess(const QByteArray & id, Radiant::BinaryData & control)
  {
    bool ok = true;
    Move tmp;

    ok = control.readString(tmp.sourceId);

    // Radiant::info("ModuleOutCollect::control # Now %d sources in the map", (int) m_map.size());

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
          ++it;
      }
    }
    else {
      tmp.from = control.readInt32( & ok);
      tmp.to   = control.readInt32( & ok);

      debugResonant("ModuleOutCollect::control # %s", id.data());

      if(!ok) {
        Radiant::error("ModuleOutCollect::control # Could not parse control # %s",
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
          Radiant::error("ModuleOutCollect::control # Could not erase mapping # %s:%d -> %d",
                tmp.sourceId.toUtf8().data(), tmp.from, tmp.to);
      }
      else {
        Radiant::error("ModuleOutCollect::control # No param \"%s\"", id.data());
      }
    }
  }

  void ModuleOutCollect::process(float ** in, float **, int n, const CallbackTime &)
  {
    size_t chans = m_channels;

    // Set to zero
    if(m_interleaved)
      memset(m_interleaved, 0, sizeof(float) * n * chans);
    else
      return;

    for(size_t i = 0; i < m_map.size(); i++) {

      int to = m_map[i].to;

      const float * src = in[i];
      const float * sentinel = src + n;

      if(!src)
        continue; // Should output a warning ;-)

      float * dest = & m_interleaved[to];

      /* if(i < 2)
        Radiant::info("ModuleOutCollect::process # %p %d %f", src, i, src[0]);
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

    if(m_compressOutput) {

      if(m_limiters.size() != chans) {
        m_limiters.resize(chans);
        for(size_t chan = 0; chan < chans; chan++) {
          m_limiters[chan].prepare(0.0f, 40);
        }
      }

      for(size_t chan = 0; chan < chans; chan++) {
        float * dest = & m_interleaved[chan];

        ChannelLimiter & limiter = m_limiters[chan];

        for(float * sentinel = dest + n * chans; dest < sentinel; dest += chans) {
          *dest = limiter.putGet(*dest, 0.f, 30, 20000);
        }
      }
    }

    const float * lastSrc = m_interleaved + (n-1) * chans;
    memcpy(m_lastSample.data(), lastSrc, sizeof(float) * chans);
  }

}
