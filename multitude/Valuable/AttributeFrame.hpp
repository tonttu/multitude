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

#ifndef VALUABLE_ATTRIBUTE_FRAME_HPP
#define VALUABLE_ATTRIBUTE_FRAME_HPP

#include <Nimble/Frame4.hpp>

#include "AttributeVector.hpp"

namespace Valuable
{
  class VALUABLE_API AttributeFrame : public AttributeVector<Nimble::Frame4f>
  {
  public:
    using AttributeVector<Nimble::Frame4f>::operator =;

    AttributeFrame(Node * host, const QString & name,
                   const Nimble::Frame4f & v = Nimble::Frame4f(), bool transit = false)
      : AttributeVector<Nimble::Frame4f>(host, name, v, transit)
    {
    }
  };

}

#endif // VALUABLE_ATTRIBUTE_FRAME_HPP
