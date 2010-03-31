/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: Helsinki University of Technology, MultiTouch Oy and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include <Radiant/Color.hpp>
#include <Radiant/Trace.hpp>

#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <WinPort.h>
#endif


int main(int argc, char ** argv)
{
  /* Test that settings colors works properly. */

  const int ncolors = 5;

  Radiant::Color colors[ncolors] = {
    Radiant::Color("#AABBCC"),   // RGB hex-color
    Radiant::Color("#11223344"), // RGBA hex-color
    Radiant::Color(0, 100, 255, 80), // Integer color
    Radiant::Color(0.1f, 0.2f, 0.3f, 0.4f), // Floating point color
    // Floating point color from a vector:
    Radiant::Color(Nimble::Vector4f(0.1f, 0.2f, 0.3f, 0.4f))
  };

  for(int i = 0; i < ncolors; i++) {
    Radiant::Color c = colors[i];
    Radiant::info("Color %d RGBA components are: %.3f %.3f %.3f %.3f", i+1,
		  c.red(), c.green(), c.blue(), c.alpha());
  }
  
  return 0;
}

