/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_VALUABLE_HPP
#define VALUABLE_VALUABLE_HPP



#define debugValuable(...) (Radiant::trace("Valuable", Radiant::Trace::DEBUG, __VA_ARGS__))
/** A library for automatically saving and loading class member values.

    The purpose of this framework is to handle:


    * Saving classes with members to XML files
    
    * Loading classes with members from XML files

    * Set/get parameter member values dynamically by string name
 */

#include <Valuable/Export.hpp>

namespace Valuable
{
  class StyleValue;
  class Node;
  class Attribute;
}

#endif

