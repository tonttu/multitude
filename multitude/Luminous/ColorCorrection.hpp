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
    ColorCorrection(Node * parent = 0, const QString & name = "", bool transit = false);
    virtual ~ColorCorrection();

    int nearestControlPoint(float x, int channel, bool modifiers, Nimble::Vector2f & controlPointOut) const;
    /// @returns the new control point index
    int addControlPoint(float x, float y, int channel, bool modifiers);
    void removeControlPoint(int index, int channel);
    const std::vector<Nimble::Vector2f> & controlPoints(int channel) const;
    std::vector<Nimble::Vector2f> controlPoints(int channel, bool modifiers) const;

    /// @param modifiers include gamma, brightness and contrast
    float value(float x, int channel, bool clamp, bool modifiers) const;
    Nimble::Vector3f valueRGB(float x) const;

    bool isIdentity() const;
    void setIdentity();
    void changeUniform(int channel, float v);

    void encode(Radiant::BinaryData & bd) const;
    bool decode(Radiant::BinaryData & bd);

    void fill(std::vector<Nimble::Vector3ub> & to) const;

    Nimble::Vector3 gamma() const;
    void setGamma(const Nimble::Vector3 & gamma);

    Nimble::Vector3 contrast() const;
    void setContrast(const Nimble::Vector3 & contrast);

    Nimble::Vector3 brightness() const;
    void setBrightness(const Nimble::Vector3 & brightness);

    virtual Valuable::ArchiveElement serialize(Valuable::Archive & archive) const;
    virtual bool deserialize(const Valuable::ArchiveElement & element);
    virtual bool readElement(const Valuable::ArchiveElement &);

  private:
    void changed();
    void checkChanged();

  private:
    class D;
    D * m_d;
  };
}

#endif
