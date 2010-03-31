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

#ifndef FP_PATH_HPP
#define FP_PATH_HPP

#include <Luminous/Export.hpp>
#include <Luminous/GLSLProgramObject.hpp>
#include <Luminous/TCBSpline.hpp>

#include <Nimble/Matrix3.hpp>
#include <Nimble/Vector2.hpp>

#include <Valuable/DOMDocument.hpp>
#include <Valuable/DOMElement.hpp>
#include <Valuable/HasValues.hpp>

#include <vector>

namespace Luminous {

  /// Path provides some utility functions to manipulate series of points
  /// @todo: rewrite and/or document
  class LUMINOUS_API Path : public Valuable::HasValues
  {
    public:
      Path();

      void addPoint(Nimble::Vector2f p) { m_points.push_back(p); }

      void clear() { m_points.clear(); }

      void render();

      void renderSpline();
      void renderLineStrip(const Nimble::Matrix3f & m) const;

      void renderDebug() const;

      void prepare();

      void simplify(float clusterTolerance, float dpTolerance);

      void simplifyAngular(float degrees);

      void transform(const Nimble::Matrix3f & m);

      size_t size() const { return m_points.size(); }
      Nimble::Vector2f point(size_t i) const { return m_points[i]; }

    virtual const char * type() const { return "Path"; }

      virtual Valuable::DOMElement serializeXML(Valuable::DOMDocument * doc);
      virtual bool deserializeXML(Valuable::DOMElement element);

      Nimble::Vector2f center() const;

      static bool intersect(const Path & p1, const Nimble::Matrix3f & m1, const Path & p2, const Nimble::Matrix3f & m2);

      bool isDegenerate() const;

      TCBSpline2 * spline();

      void debugDump() const;


      typedef std::vector<Nimble::Vector2f> container;
      container points() { return m_points; }

    protected:

      container m_points;
  };

}

#endif
