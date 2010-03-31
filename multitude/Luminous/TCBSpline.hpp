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

#ifndef TCB_SPLINE_HPP
#define TCB_SPLINE_HPP

#include <Luminous/Export.hpp>

#include <Nimble/Vector2.hpp>
#include <Nimble/Matrix3.hpp>

#include <vector>

namespace Luminous {

/// A 2D TCB-spline class
  /// @todo Doc, use template to allow different types, difficult to edit
class LUMINOUS_API TCBSpline2
{
  public:
    TCBSpline2() {}
    TCBSpline2(int segments,
        const std::vector<float> & time,
        const std::vector<Nimble::Vector2f> & points,
        const std::vector<float> & tension,
        const std::vector<float> & continuity,
        const std::vector<float> & bias);

    virtual ~TCBSpline2();

    Nimble::Vector2f value(float t) const;
    Nimble::Vector2f firstDerivative(float t) const;

    float length() const { return m_time.back(); }

    void render() const;
    void renderQuads(float step, float thickness, const Nimble::Matrix3f & m) const;

    void transform(const Nimble::Matrix3f & m);

  protected:
    void computePoly(int i0, int i1, int i2, int i3);

    void getKeyInfo(float t, int & key, float & dt) const;

    void rebuildPolys();

    size_t m_segments;
    std::vector<float> m_time;

    std::vector<Nimble::Vector2f> m_points;
    std::vector<float> m_tension;
    std::vector<float> m_continuity;
    std::vector<float> m_bias;

    std::vector<Nimble::Vector2f> m_A;
    std::vector<Nimble::Vector2f> m_B;
    std::vector<Nimble::Vector2f> m_C;
    std::vector<Nimble::Vector2f> m_D;
};

}

#endif
