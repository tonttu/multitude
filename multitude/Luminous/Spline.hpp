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

    Spline(Spline && spline);
    Spline & operator=(Spline && spline);

    void addControlPoint(Nimble::Vector2 point, Nimble::Vector4 color, float width);
    void clear();

    void render(Luminous::RenderContext &) const;

    void recalculate();
  private:
    class D;
    D * m_d;
  };

}

#endif // SPLINE_HPP
