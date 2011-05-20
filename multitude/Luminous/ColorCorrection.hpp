/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */

#ifndef LUMINOUS_COLOR_CORRECTION_HPP
#define LUMINOUS_COLOR_CORRECTION_HPP

#include <Valuable/HasValues.hpp>
#include <Valuable/ValueContainer.hpp>

#include <vector>

namespace Luminous
{
  // Color correction curves for red, green and blue channels
  class ColorCorrection : public Valuable::HasValues
  {
  public:
    ColorCorrection(HasValues * parent, const std::string & name, bool transit = false);

    Nimble::Vector3T<uint8_t>& getValue(int idx);

    const Nimble::Vector3T<uint8_t>& getValue(int idx) const;

    const Nimble::Vector3T<uint8_t>* getLUT() const;

    void setIdentity();
    void changeUniform(int channel, int v);

  private:
    Valuable::ValueContainer< std::vector<Nimble::Vector3T<uint8_t> > > m_lut;
  };
}

#endif
