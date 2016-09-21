/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_COLOR_CORRECTION_HPP
#define LUMINOUS_COLOR_CORRECTION_HPP

#include "Export.hpp"

#include <Luminous/RGBCube.hpp>

#include <Valuable/AttributeVector.hpp>
#include <Valuable/Node.hpp>
#include <Valuable/AttributeContainer.hpp>

#include <vector>

/// @cond

namespace Luminous
{
  // Color correction curves for red, green and blue channels. Each color curve is a
  // function which domain and range is [0,1]
  class LUMINOUS_API ColorCorrection : public Valuable::Node
  {
  public:
    ColorCorrection(Node * parent = 0, const QByteArray &name = "", bool transit = false);
    virtual ~ColorCorrection();

    int nearestControlPoint(float x, int channel, bool modifiers, Nimble::Vector2f & controlPointOut) const;
    /// @returns the new control point index
    int addControlPoint(float x, float y, int channel, bool modifiers);
    void removeControlPoint(int index, int channel);
    const std::vector<Nimble::Vector2f> & controlPoints(int channel) const;
    std::vector<Nimble::Vector2f> controlPoints(int channel, bool modifiers) const;

    Nimble::Vector3 controlPoint(size_t index) const;
    void setControlPoint(size_t index, const Nimble::Vector3 & rgbvalue);
    void multiplyRGBValues(float mul, bool clamp);

    /// @param modifiers include gamma, brightness and contrast
    float value(float x, int channel, bool clamp, bool modifiers) const;
    Nimble::Vector3f value(float x) const;
    Nimble::Vector3f valueRGB(float x, bool clamp = true, bool modifiers = true) const;
    bool isIdentity() const;
    void setIdentity();
    void setIdentity(const std::vector<float> & points);
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

    /// @todo this breaks backwards compatibility. The firmware update process
    /// should migrate the configuration from screen.xml to separate
    /// color-correction.xml (see PictureModule::save())
//    virtual Valuable::ArchiveElement serialize(Valuable::Archive & archive) const;
    virtual bool deserialize(const Valuable::ArchiveElement & element);
    virtual bool readElement(const Valuable::ArchiveElement & element);

    const RGBCube & asRGBCube() const;

  private:
    void changed();

  private:
    class D;
    D * m_d;
  };
}

/// @endcond

#endif
