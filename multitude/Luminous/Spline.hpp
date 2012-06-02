#ifndef LUMINOUS_SPLINE_HPP
#define LUMINOUS_SPLINE_HPP

#include "Export.hpp"

#include <Nimble/Vector4.hpp>

#include <Patterns/NotCopyable.hpp>

namespace Luminous {

  class RenderContext;

  class LUMINOUS_API Spline : public Patterns::NotCopyable
  {
  public:
    Spline();
    ~Spline();

    void addControlPoint(Nimble::Vector2 point, Nimble::Vector4 color, float width);
    void clear();

    void render(Luminous::RenderContext &) const;

    void recalculate();
  private:
    class Point;
    class Vertex;
    class D;
    D * m_d;
  };

}

#endif // SPLINE_HPP
