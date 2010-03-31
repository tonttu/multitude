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

// The purpose of this file is to include ValueString.hpp and ValueStringImpl.hpp in the
// Valuable build, thereby including any template instantiations defined in those files

#include "ValueStringImpl.hpp"

template class Valuable::ValueStringT<std::string>;
template class Valuable::ValueStringT<std::wstring>;
