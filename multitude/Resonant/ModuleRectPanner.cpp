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

#include "ModuleRectPanner.hpp"

#include <Nimble/Interpolation.hpp>

namespace Resonant
{

  ModuleRectPanner::ModuleRectPanner(Resonant::Application * app)
    : ModulePanner(app)
  {
  }

  void ModuleRectPanner::addSoundRectangle(const SoundRectangle &r)
  {
    //  Radiant::info("ModuleRectPanner::addSoundRectangle # new rect %d,%d %d,%d", r.location().x, r.location().y, r.size().x, r.size().y);

    m_rectangles.push_back(r);

    // Add the two speakers
    int x1 = r.location().x;
    int x2 = r.location().x + r.size().x;
    int y = r.location().y + r.size().y / 2;

    setSpeaker(r.leftChannel(), x1, y);
    setSpeaker(r.rightChannel(), x2, y);

    //  Radiant::info("ModuleRectPanner::addSoundRectangle # new speaker %d at %d,%d", r.leftChannel(), x1, y);
    //  Radiant::info("ModuleRectPanner::addSoundRectangle # new speaker %d at %d,%d", r.rightChannel(), x2, y);
  }

  float ModuleRectPanner::computeGain(const LoudSpeaker *ls, Nimble::Vector2 srcLocation) const
  {
    float gain = 0.f;

    const SoundRectangle * r = getContainingRectangle(ls);
    if(r) {
      //Radiant::info("ModuleRectPanner::computeGain # SPEAKER (%f,%f) source is inside rectangle (%d,%d) (%d,%d)", ls->m_location.x(), ls->m_location.y(), r->location().x, r->location().y, r->size().x, r->size().y);

      Nimble::Vector2 tmp = r->location();
      Nimble::Vector2 local = srcLocation - tmp;

      // Compute gain in y direction
      Nimble::LinearInterpolator<float> iy;
      iy.addKey(-r->fade(), 0.f);
      iy.addKey(0.f, 1.f);
      iy.addKey(r->size().y, 1.f);
      iy.addKey(r->size().y + r->fade(), 0.f);

      float gainY = iy.interpolate(local.y);

      // Compute gain in x direction
      float rectMidX = r->location().x + r->size().x / 2;
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

  const SoundRectangle * ModuleRectPanner::getContainingRectangle(const LoudSpeaker * ls) const
  {
    // Find the channel for the speaker
    int channel = -1;
    bool found = false;
    for(size_t i = 0; i < m_speakers.size(); i++) {
      channel = i;
      if(m_speakers[i].get() == ls) {
        found = true;
        break;
      }
    }

    if(!found)
      return 0;

    for(Rectangles::const_iterator it = m_rectangles.begin(); it != m_rectangles.end(); it++) {
      const SoundRectangle * r = &(*it);

      if(channel == r->leftChannel() || channel == r->rightChannel())
        return r;
    }

    // Should never happen
    assert(0);

    return 0;
  }

}
