/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef RESONANT_MODULERECTPANNER_HPP
#define RESONANT_MODULERECTPANNER_HPP

#include "Export.hpp"
#include "ModulePanner.hpp"
#include "SoundRectangle.hpp"

namespace Resonant
{
  /** This class performs audio panning based on rectangular sound regions.
    Rectangle panning logic has been moved to Resonant::ModulePanner, so
    this class only exists for backwards compatibility
    */
  class RESONANT_API ModuleRectPanner : public Resonant::ModulePanner
  {
  public:
    /// Constructs a new module
    ModuleRectPanner();
  };

}

#endif // MODULERECTPANNER_HPP
