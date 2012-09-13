/* COPYRIGHT
 *
 * This file is part of Radiant.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Radiant.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#include "Color.hpp"
#include "Trace.hpp"

#include <wctype.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Radiant
{
  
  Color::Color()
  : Nimble::Vector4f(0.0f, 0.0f, 0.0f, 1.0f)
  {}

  Color::Color(const char * color)
  {
    size_t clen = strlen(color);

    make(0, 0, 0, 1);

    if(color[0] == '#') {
      char tmp[3];
      tmp[2] = 0;

      for(size_t i = 0; i < 4 && (i * 2 + 2) < clen; i++) {
	tmp[0] = color[i * 2 + 1];
	tmp[1] = color[i * 2 + 2];

        // ishexnumber
	if(!isxdigit(tmp[0]) || !isxdigit(tmp[1]))
	  break;

	char * foo = 0;
	int val = strtol(tmp, & foo,  16);
        get((int) i) = val / 255.0f;
	// printf("Got color %d, %f", i, val / 255.0f);
      }
    }
    else {
      error("Color::Color(const char * color) # "
	    "Argument string should start with #");
    }
  }

  Color::Color(float r, float g, float b, float a)
  {
    setRGBA(r, g, b, a);
  }

  Color::Color(const Nimble::Vector4f & v) 
  {
    setRGBA(v.x, v.y, v.z, v.w);
  }

  Color::~Color()
  {}

  void Color::setRGBA(float r, float g, float b, float a)
  {
    make(r, g, b, a);
  }
}
