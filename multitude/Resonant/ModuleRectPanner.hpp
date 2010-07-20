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
