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

#include "ModulePanner.hpp"

#include <Radiant/BinaryData.hpp>
#include <Radiant/Trace.hpp>

#include <Valuable/DOMElement.hpp>
#include <Valuable/DOMDocument.hpp>

#include <assert.h>
#include <string.h>
#include <strings.h>

namespace Resonant {

  using namespace Nimble;
  using namespace Radiant;

  ModulePanner::ModulePanner(Application * a)
      : Module(a),
      m_outChannels(8),
      m_maxRadius(this, "max-radius", 1500)
  {
    setName("pan2d");

  }

  ModulePanner::~ModulePanner()
  {}

  Valuable::DOMElement ModulePanner::serializeXML(Valuable::DOMDocument * doc)
  {
    Valuable::DOMElement e = Module::serializeXML(doc);

    return e;
  }


  bool ModulePanner::readElement(Valuable::DOMElement element)
  {
    if(element.getTagName() == "speaker") {
      LoudSpeaker * ls = new LoudSpeaker;
      bool ok = ls->deserializeXML(element);

      if(ok) {
        m_speakers.push_back(ls);
        return true;
      }
    }

    return false;
  }

  bool ModulePanner::prepare(int & channelsIn, int & channelsOut)
  {
    (void) channelsIn;

    channelsOut = m_speakers.size();

    return true;
  }

  void ModulePanner::processMessage(const char * id,
                                    Radiant::BinaryData * data)
  {
    debug("ModulePanner::control # %s", id);

    bool ok = true;

    if(strcmp(id, "channels") == 0) {
      int n = data->readInt32( & ok);
      if(ok)
        m_outChannels = n;
    }
    else if(strcmp(id, "fullhdstereo") == 0) {
      makeFullHDStereo();
    }
    else if(strcmp(id, "addsource") == 0) {
      Source s;
      data->readString(s.m_id);
      m_sources.push_back(s);
    }
    else if(strcmp(id, "removesource") == 0) {
      std::string id;
      data->readString(id);
      removeSource(id);
    }
    else if(strcmp(id, "setsourcelocation") == 0) {
      std::string id;
      data->readString(id);
      Nimble::Vector2 loc = data->readVector2Float32( & ok);

      if(ok) {
        setSourceLocation(id, loc);
      }
      else {
        error("ModulePanner::control # %s # Could not read source location",
              id.c_str());
      }
    }
    else {
      error("ModulePanner::control # Unknown command %s", id);
    }
  }

  void ModulePanner::process(float ** in, float ** out, int n)
  {
    int bufferbytes = n * sizeof(float);

    // Zero the output channels
    for(unsigned i = 0; i < m_speakers.size(); i++) {
      bzero(out[i], bufferbytes);
    }

    for(int i = 0; i < (int) m_sources.size(); i++) {

      Source & s = *m_sources[i];

      for(int j = 0; j < PIPES_PER_SOURCE; j++) {

        Pipe & p = s.m_pipes[j];

        if(p.done())
          continue;

        const float * src = in[i];

        float * dest = out[p.m_to];
        float * sentinel = dest + n;

        if(p.m_ramp.left()) {

          for( ; dest < sentinel; dest++, src++) {
            *dest += (*src * p.m_ramp.value());
            p.m_ramp.update();
          }
        }
        else {
          float v = p.m_ramp.value();
          for( ; dest < sentinel; dest++, src++) {
            *dest += (*src * v);
          }
        }

        /* debug("ModulePanner::process # source %d, pipe %d, gain = %f "
              "in = %p %f out = %f",
              i, j, p.m_ramp.value(), in[i], *in[i], out[p.m_to][0]);
        */
      }
    }
  }

  void ModulePanner::makeFullHDStereo()
  {
    m_speakers.clear();

    LoudSpeaker * ls = new LoudSpeaker;

    ls->m_location = Vector2(0, 540);
    m_speakers.push_back(ls);

    ls = new LoudSpeaker;

    ls->m_location = Vector2(1920, 540);
    m_speakers.push_back(ls);

    m_maxRadius = 1200;
  }

  void ModulePanner::setSpeaker(unsigned i, Nimble::Vector2 location)
  {
    assert(i < 100000);

    if(m_speakers.size() <= i)
      m_speakers.resize(i + 1);

    LoudSpeaker * ls = new LoudSpeaker;

    ls->m_location = location;
    m_speakers[i] = ls;
  }

  void ModulePanner::setSpeaker(unsigned i, float x, float y)
  {
    setSpeaker(i, Nimble::Vector2(x, y));
  }

  void ModulePanner::setSourceLocation(const std::string & id,
                                       Nimble::Vector2 location)
  {
    debug("ModulePanner::setSourceLocation # %s [%f %f]", id.c_str(),
          location.x, location.y);

    Source * s = 0;

    for(unsigned i = 0; i < m_sources.size(); i++) {
      Source & s2 = * m_sources[i];
      if(s2.m_id == id) {
        s = & s2;
      }
    }

    if(!s) {
      error("ModulePanner::setSourceLocation # id \"%s\" is not known",
            id.c_str());
      return;
    }

    s->m_location = location;

    int interpSamples = 2000;

    for(unsigned i = 0; i < m_speakers.size(); i++) {
      LoudSpeaker * ls = m_speakers[i].ptr();

      if(!ls)
        continue;

      float d = (location - ls->m_location.asVector()).length();
      float rel = d / m_maxRadius;

      float inv = 1.0f - rel;
      float gain = Nimble::Math::Min(inv * 2.0f, 1.0f);

      if(gain <= 0.0000001f) {

        // Silence that output:
        for(unsigned j = 0; j < PIPES_PER_SOURCE; j++) {
          Pipe & p = s->m_pipes[j];
          if(p.m_to == i && p.m_ramp.target() > 0.0001f) {
            p.m_ramp.setTarget(0.0f, interpSamples);
            debug("ModulePanner::setSourceLocation # Silencing %u", i);

          }
        }
      }
      else {
        bool found = false;

        // Find existing pipe:
        for(unsigned j = 0; j < PIPES_PER_SOURCE && !found; j++) {
          Pipe & p = s->m_pipes[j];

          debug("Checking %u: %u %f -> %f", j, p.m_to,
                p.m_ramp.value(), p.m_ramp.target());

          if(p.m_to == i && p.m_ramp.target() > 0.0001f) {
            debug("ModulePanner::setSourceLocation # Adjusting %u", j);
            p.m_ramp.setTarget(gain, interpSamples);
            found = true;
          }
        }

        if(!found) {

          // Pick up a new pipe:
          for(unsigned j = 0; j < PIPES_PER_SOURCE && !found; j++) {
            Pipe & p = s->m_pipes[j];
            if(p.done()) {
              debug("ModulePanner::setSourceLocation # "
                    "Starting %u towards %u", j, i);
              p.m_to = i;
              p.m_ramp.setTarget(gain, interpSamples);
              found = true;
            }
          }
        }

        if(!found) {
          error("Could not allocate pipe for a moving source");
        }
      }
    }
  }

  void ModulePanner::removeSource(const std::string & id)
  {

    for(Sources::iterator it = m_sources.begin();
    it != m_sources.end(); it++) {
      Source & s = * (*it);
      if(s.m_id == id) {
        m_sources.erase(it);
        debug("ModulePanner::removeSource # Removed source %s, now %u",
              id.c_str(), m_sources.size());
        return;
      }
    }

    error("ModulePanner::removeSource # No such source: \"%s\"", id.c_str());
  }

}
