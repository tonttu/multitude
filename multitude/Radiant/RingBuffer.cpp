/* COPYRIGHT
 *
 * This file is part of Nimble.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Nimble.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#include "RingBuffer.hpp"
#include "RingBufferImpl.hpp"

#include <Nimble/Vector2.hpp>

namespace Nimble {

}

template class Radiant::RingBuffer<float>;
template class Radiant::RingBuffer<int>;
template class Radiant::RingBuffer<Nimble::Vector2>;
