/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#include "ModulePanner.hpp"
#include "Resonant.hpp"

#include <Nimble/Interpolation.hpp>

#include <Radiant/BinaryData.hpp>
#include <Radiant/Trace.hpp>

#include <Valuable/DOMElement.hpp>
#include <Valuable/DOMDocument.hpp>
#include <Valuable/Attribute.hpp>

#include <assert.h>
#include <string.h>

namespace Resonant {

  ModulePanner::ModulePanner(Mode mode)
      : Module(),
      m_speakers(this, "speakers"),
      m_generation(0),
      m_maxRadius(this, "max-radius", 1500),
      m_rectangles(this, "rectangles"),
      m_operatingMode(this, "mode", mode)
  {
    setName("pan2d");

  }

  ModulePanner::~ModulePanner()
  {
    for (size_t i=0; i < m_rectangles->size(); ++i)
      delete (*m_rectangles)[i];
  }

  bool ModulePanner::deserialize(const Valuable::ArchiveElement & element)
  {
    m_rectangles->clear();
    m_speakers->clear();
    bool ok = Module::deserialize(element);
    ++m_generation;

    if(ok && !m_rectangles->empty() && m_speakers->empty()) {
      for(size_t i = 0; i < m_rectangles->size(); ++i)
        addSoundRectangleSpeakers((*m_rectangles)[i]);
    }

    return ok;
  }

  bool ModulePanner::prepare(int & channelsIn, int & channelsOut)
  {
    (void) channelsIn;

    channelsOut = static_cast<int> (m_speakers->size());

    return true;
  }

  void ModulePanner::eventProcess(const QByteArray & id,
                                    Radiant::BinaryData & data)
  {
    debugResonant("ModulePanner::control # %s", id.data());

    bool ok = true;

    if(id == "fullhdstereo") {
      makeFullHDStereo();
    }
    else if(id == "addsource") {
      Source s;
      data.readString(s.m_id);
      m_sources.push_back(s);
    }
    else if(id == "removesource") {
      QByteArray id;
      data.readString(id);
      removeSource(id);
    }
    else if(id == "setsourcelocation") {
      QByteArray id;
      data.readString(id);
      Nimble::Vector2 loc = data.readVector2Float32( & ok);

      if(ok) {
        setSourceLocation(id, loc);
      }
      else {
        Radiant::error("ModulePanner::control # %s # Could not read source location",
              id.data());
      }
    }
    else {
      Radiant::error("ModulePanner::control # Unknown command %s", id.data());
    }
  }

  void ModulePanner::process(float ** in, float ** out, int n, const CallbackTime &)
  {
    int bufferbytes = n * sizeof(float);

    // Zero the output channels
    for(unsigned i = 0; i < m_speakers->size(); i++) {
      memset(out[i], 0, bufferbytes);
    }

    for(int i = 0; i < (int) m_sources.size(); i++) {

      Source & s = *m_sources[i];

      for(int j = 0; j < (int) s.m_pipes.size(); j++) {

        Pipe & p = s.m_pipes[j];

        if(p.isDone())
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

        debugResonant("ModulePanner::process # source %d, pipe %d -> %d, gain = %f "
              "in = %p %f out = %f",
              i, j, p.m_to, p.m_ramp.value(), in[i], *in[i], out[p.m_to][0]);

      }
    }
  }

  void ModulePanner::makeFullHDStereo()
  {
    m_speakers->clear();

    LoudSpeaker * ls = new LoudSpeaker;

    ls->m_location = Nimble::Vector2f(0, 540);
    m_speakers->push_back(std::shared_ptr<LoudSpeaker>(ls));

    ls = new LoudSpeaker;

    ls->m_location = Nimble::Vector2f(1920, 540);
    m_speakers->push_back(std::shared_ptr<LoudSpeaker>(ls));

    m_maxRadius = 1200;
    ++m_generation;
  }

  void ModulePanner::addSoundRectangle(SoundRectangle * r)
  {
    //  Radiant::info("ModuleRectPanner::addSoundRectangle # new rect %d,%d %d,%d", r.location().x, r.location().y, r.size().x, r.size().y);

    m_rectangles->push_back(r);

    addSoundRectangleSpeakers(r);
  }

  void ModulePanner::setMode(Mode mode)
  {
    m_operatingMode = mode;
  }

  ModulePanner::Mode ModulePanner::getMode() const
  {
    return (ModulePanner::Mode)*m_operatingMode;
  }

  int ModulePanner::locationToChannel(Nimble::Vector2 location) const
  {
    int best = -1;
    float bestd = 1000000.0f;
    int i = 0;
    for(auto it = m_speakers->begin(); it != m_speakers->end(); it++, i++) {
      if (!*it) continue;
      const LoudSpeaker & l = **it;
      const float dist = (l.m_location - location).length();
      if(dist < bestd || best == -1) {
        best = i;
        bestd = dist;
      }
    }

    return (best >= 0) ? best : 0;
  }

  void ModulePanner::setSpeaker(unsigned i, Nimble::Vector2 location)
  {
    assert(i < 100000);

    if(m_speakers->size() <= i)
      m_speakers->resize(i + 1);

    LoudSpeaker * ls = new LoudSpeaker;

    ls->m_location = location;
    (*m_speakers)[i].reset(ls);
    ++m_generation;
  }

  void ModulePanner::setSpeaker(unsigned i, float x, float y)
  {
    setSpeaker(i, Nimble::Vector2(x, y));
  }

  void ModulePanner::setSourceLocation(const QByteArray & id,
                                       Nimble::Vector2 location)
  {
    debugResonant("ModulePanner::setSourceLocation # %s [%f %f]", id.data(),
          location.x, location.y);

    Source * s = 0;

    for(unsigned i = 0; i < m_sources.size(); i++) {
      Source & s2 = * m_sources[i];
      if(s2.m_id == id) {
        s = & s2;
      }
    }

    if(!s) {
      Radiant::error("ModulePanner::setSourceLocation # id \"%s\" is not known",
            id.data());
      return;
    }

    if(s->m_location == location && s->m_generation == m_generation)
      return;

    s->m_generation = m_generation;
    s->m_location = location;

    int interpSamples = 2000;

    for(unsigned i = 0; i < m_speakers->size(); i++) {
      LoudSpeaker * ls = (*m_speakers)[i].get();

      if(!ls)
        continue;

      float gain = computeGain(ls, location);

      if(gain <= 0.0000001f) {

        // Silence that output:
        for(unsigned j = 0; j < s->m_pipes.size(); j++) {
          Pipe & p = s->m_pipes[j];
          if(p.m_to == i && p.m_ramp.target() >= 0.0001f) {
            p.m_ramp.setTarget(0.0f, interpSamples);
            debugResonant("ModulePanner::setSourceLocation # Silencing %u", i);

          }
        }
      }
      else {
        bool found = false;

        // Find existing pipe:
        for(unsigned j = 0; j < s->m_pipes.size() && !found; j++) {
          Pipe & p = s->m_pipes[j];

          debugResonant("Checking %u: %u %f -> %f", j, p.m_to,
                p.m_ramp.value(), p.m_ramp.target());

          if(p.m_to == i) {
            debugResonant("ModulePanner::setSourceLocation # Adjusting %u", j);
            p.m_ramp.setTarget(gain, interpSamples);
            found = true;
          }
        }

        if(!found) {

          // Pick up a new pipe:
          for(unsigned j = 0; j <= s->m_pipes.size() && !found; j++) {
            if(j == s->m_pipes.size()) {
              s->m_pipes.resize(j+1);
              debugResonant("ModulePanner::setSourceLocation # pipes resize to %d", j+1);
            }
            Pipe & p = s->m_pipes[j];
            if(p.isDone()) {
              debugResonant("ModulePanner::setSourceLocation # "
                    "Starting %u towards %u", j, i);
              p.m_to = i;
              p.m_ramp.setTarget(gain, interpSamples);
              found = true;
            }
          }
        }

        if(!found) {
          Radiant::error("Could not allocate pipe for a moving source");
        }
      }
    }
  }

  void ModulePanner::removeSource(const QByteArray & id)
  {

    for(Sources::iterator it = m_sources.begin(); it != m_sources.end(); ++it) {
      Source & s = * (*it);
      if(s.m_id == id) {
        m_sources.erase(it);
        debugResonant("ModulePanner::removeSource # Removed source %s, now %lu",
              id.data(), m_sources.size());
        return;
      }
    }

    Radiant::error("ModulePanner::removeSource # No such source: \"%s\"", id.data());
  }

  void ModulePanner::addSoundRectangleSpeakers(SoundRectangle * r)
  {
    // Add the two speakers
    int x1 = r->location().x;
    int x2 = r->location().x + r->size().x;
    int y = r->location().y + r->size().y / 2;

    if(r->leftChannel() == r->rightChannel())
      x1 = x2 = 0.5f * (x1+x2);

    setSpeaker(r->leftChannel(), x1, y);
    setSpeaker(r->rightChannel(), x2, y);

    //  Radiant::info("ModuleRectPanner::addSoundRectangle # new speaker %d at %d,%d", r.leftChannel(), x1, y);
    //  Radiant::info("ModuleRectPanner::addSoundRectangle # new speaker %d at %d,%d", r.rightChannel(), x2, y);
  }

  float ModulePanner::computeGain(const LoudSpeaker * ls, Nimble::Vector2 srcLocation) const
  {
    switch(*m_operatingMode) {
    case RADIAL:
      return computeGainRadial(ls, srcLocation);
    case RECTANGLES:
      return computeGainRectangle(ls, srcLocation);
    default:
      return 0;
    }
  }

  float ModulePanner::computeGainRadial(const LoudSpeaker * ls, Nimble::Vector2 srcLocation) const
  {
    float d = (srcLocation - ls->m_location.asVector()).length();
    float rel = d / m_maxRadius;

    float inv = 1.0f - rel;

    return std::min(inv * 2.0f, 1.0f);
  }

  float ModulePanner::computeGainRectangle(const LoudSpeaker * ls, Nimble::Vector2 srcLocation) const
  {
    float gain = 0.f;

    const SoundRectangle * r = getContainingRectangle(ls);
    if(r) {
      //Radiant::info("ModuleRectPanner::computeGain # SPEAKER (%f,%f) source is inside rectangle (%d,%d) (%d,%d)", ls->m_location.x(), ls->m_location.y(), r->location().x, r->location().y, r->size().x, r->size().y);

      Nimble::Vector2 tmp(r->location().x, r->location().y);
      Nimble::Vector2 local = srcLocation - tmp;

      // Compute gain in y direction
      Nimble::LinearInterpolator<float> iy;
      iy.addKey(-r->fade(), 0.f);
      iy.addKey(0.f, 1.f);
      iy.addKey(r->size().y, 1.f);
      iy.addKey(r->size().y + r->fade(), 0.f);

      float gainY = iy.interpolate(local.y);

      // Compute gain in x direction
      float rectMidX = (float)(r->location().x) + r->size().x * 0.5f;
      Nimble::LinearInterpolator<float> ix;

      if(ls->m_location.x() < rectMidX) {
        // Left channel
        ix.addKey(-r->fade(), 0.f);
        ix.addKey(0.f, 1.f);
        ix.addKey(r->size().x, 1.f - r->stereoPan());
        ix.addKey(r->size().x + r->fade(), 0.f);
      } else {
        // Right channel
        ix.addKey(-r->fade(), 0.f);
        ix.addKey(0.f, 1.f - r->stereoPan());
        ix.addKey(r->size().x, 1.f);
        ix.addKey(r->size().x + r->fade(), 0.f);
      }

      float gainX = ix.interpolate(local.x);

      //Radiant::info("ModuleRectPanner::computeGain # gain x %f, gain y %f", gainX, gainY);

      gain = gainX * gainY;
    }

    //Radiant::info("ModuleRectPanner::computeGain # speaker at %f,%f | source %f,%f | gain %f", ls->m_location.x(), ls->m_location.y(), srcLocation.x, srcLocation.y, gain);

    return gain;
  }

  const SoundRectangle * ModulePanner::getContainingRectangle(const LoudSpeaker * ls) const
  {
    // Find the channel for the speaker
    int channel = -1;
    bool found = false;
    for(size_t i = 0; i < m_speakers->size(); i++) {
      channel = static_cast<int> (i);
      if((*m_speakers)[i].get() == ls) {
        found = true;
        break;
      }
    }

    if(!found)
      return 0;

    for(Rectangles::const_iterator it = m_rectangles->begin(); it != m_rectangles->end(); ++it) {
      const SoundRectangle * r = *it;

      if(channel == r->leftChannel() || channel == r->rightChannel())
        return r;
    }

    // Should never happen
    assert(0);

    return 0;
  }

}
