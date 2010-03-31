/* COPYRIGHT
 *
 * This file is part of Valuable.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Valuable.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUABLE_HPP
#define VALUABLE_VALUABLE_HPP

/** A library for automatically saving and loading class member values.

    The purpose of this framework is to handle:

    <UL>

    <LI> Saving classes with members to XML files
    
    <LI> Loading classes with members from XML files

    <LI> Set/get parameter member values dynamically by string name

    </UL>
 */

#include <Valuable/Export.hpp>

namespace Valuable
{

  void VALUABLE_API initialize();
  void VALUABLE_API terminate();

}

#endif

