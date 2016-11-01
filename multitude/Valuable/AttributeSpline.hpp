/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef VALUABLE_ATTRIBUTESPLINE_HPP
#define VALUABLE_ATTRIBUTESPLINE_HPP

#include <Valuable/Attribute.hpp>

#include <vector>
#include <Nimble/Vector2.hpp>

namespace Valuable {


  class VALUABLE_API AttributeSpline : public Attribute
  {
  public:
    AttributeSpline(Valuable::Node* host=nullptr, const QByteArray& name="");

    void clear();
    int insert(float x, float y);
    void changeUniform(float v);

    bool isIdentity() const;
    int nearestControlPoint(float x, Nimble::Vector2f & controlPointOut) const;
    float value(float x) const;

    const std::vector<Nimble::Vector2f> & points() const { return m_points; }
    const std::vector<Nimble::Vector2f> & intermediatePoints() const { return m_intermediatePoints; }
    void setPoints(std::vector<Nimble::Vector2f> & points);
    void removeControlPoint(int index);

    virtual bool isChanged() const OVERRIDE;
    virtual void copyValueFromLayer(Layer from, Layer to) OVERRIDE;
    virtual void setAsDefaults() OVERRIDE;

    virtual bool deserialize(const Valuable::ArchiveElement& element) OVERRIDE;
    virtual Valuable::ArchiveElement serialize(Valuable::Archive& doc) const OVERRIDE;


    QByteArray serialize() const;
    bool deserialize(const QByteArray & str);

    void fixEdges();

    void changed() {
      //eventSend("changed");
      emitChange();
      //m_points.emitChange();
      //m_intermediatePoints.emitChange();
    }

  private:
    bool areDifferent(const std::vector<Nimble::Vector2f>& v1,
                      const std::vector<Nimble::Vector2f>& v2) const;

    void update(bool hasChanged=false);

    std::vector<Nimble::Vector2f> m_points;
    std::vector<Nimble::Vector2f> m_intermediatePoints;

    std::vector<Nimble::Vector2f> m_defaultPoints;
    std::vector<Nimble::Vector2f> m_prevPoints;

    bool m_isChanged;
  };


} // namespace Valuable

#endif // VALUABLE_ATTRIBUTESPLINE_HPP
