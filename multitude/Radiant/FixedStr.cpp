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

// The purpose of this file is to include FixedStr.hpp in the Radiant build
// thereby including any template instantiations defined in that file

#include "FixedStrImpl.hpp"

template class Radiant::FixedStrT<32> ;
template class Radiant::FixedStrT<256> ;
template class Radiant::FixedStrT<512> ;
