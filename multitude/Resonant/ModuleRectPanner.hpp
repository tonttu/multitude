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

#ifndef RESONANT_MODULERECTPANNER_HPP
#define RESONANT_MODULERECTPANNER_HPP

#include "ModulePanner.hpp"
#include "SoundRectangle.hpp"

namespace Resonant
{
  /** This class performs audio panning based on rectangular sound regions.
    @sa SoundRectangle
    */
  class ModuleRectPanner : public Resonant::ModulePanner
  {
  public:
    /// Constructs a new module
    RESONANT_API ModuleRectPanner(Resonant::Application * app = 0);

    /// Adds a rectangle defining a sound region to the module
    RESONANT_API void addSoundRectangle(const SoundRectangle & r);

  protected:
    /// Computes the gain for individual speakers
    RESONANT_API virtual float computeGain(const LoudSpeaker *ls, Nimble::Vector2 srcLocation) const;

  private:
    const SoundRectangle * getContainingRectangle(const LoudSpeaker * ls) const;

    typedef std::vector<SoundRectangle> Rectangles;
    Rectangles m_rectangles;
  };

}

#endif // MODULERECTPANNER_HPP
