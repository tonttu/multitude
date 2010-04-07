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

namespace Radiant
{

    template class RingBuffer<float>;
    template class RingBuffer<int>;
    template class RingBuffer<Nimble::Vector2>;

    template class RingBufferDelay<Nimble::Vector2>;
}
