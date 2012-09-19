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

#include "Export.hpp"

#include <Valuable/AttributeVector.hpp>
#include <Valuable/Node.hpp>
#include <Valuable/AttributeContainer.hpp>

#include <vector>

namespace Luminous
{
  // Color correction curves for red, green and blue channels
  class LUMINOUS_API ColorCorrection : public Valuable::Node
  {
  public:
    ColorCorrection(Node * parent = 0, const QByteArray &name = "", bool transit = false);

    void setOffset(int idx, const Nimble::Vector3 & offset);
    void setOffset(int idx, int channel, float value);
    const Nimble::Vector3 & getOffset(int idx) const;

    const std::vector<Nimble::Vector3> & offsets() const { return *m_offsets; }
    void setOffsets(const std::vector<Nimble::Vector3f> & offsets);

    Nimble::Vector3 getValue(int idx) const;

    //const Nimble::Vector3T<float>* getLUT() const;

    bool isIdentity() const;
    void setIdentity();
    void changeUniform(int channel, float v);

    void encode(Radiant::BinaryData & bd) const;
    bool decode(Radiant::BinaryData & bd);

    void fillAsBytes(Nimble::Vector3T<uint8_t> * to) const;

    Nimble::Vector3 gamma() const { return m_gamma; }
    void setGamma(const Nimble::Vector3 & gamma) { m_gamma = gamma; }

    Nimble::Vector3 contrast() const { return m_contrast; }
    void setContrast(const Nimble::Vector3 & contrast) { m_contrast = contrast; }

    Nimble::Vector3 brightness() const { return m_brightness; }
    void setBrightness(const Nimble::Vector3 & brightness) { m_brightness = brightness; }

    void setChanged();

  private:
    /// If this is changed outside the class, setChanged must be called
    Valuable::AttributeContainer< std::vector<Nimble::Vector3> > m_offsets;
    Valuable::AttributeVector3f m_gamma;
    Valuable::AttributeVector3f m_contrast;
    Valuable::AttributeVector3f m_brightness;
    bool m_identity;

    std::vector<Nimble::Vector3T<uint8_t> > m_prev;
  };
}

#endif
