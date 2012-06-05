#ifndef LUMINOUS_SPLINE_HPP
#define LUMINOUS_SPLINE_HPP

#include "Export.hpp"

#include <Nimble/Vector4.hpp>
#include <Nimble/Rectangle.hpp>

#include <Patterns/NotCopyable.hpp>

class QIODevice;

namespace Luminous {

  class RenderContext;

  class LUMINOUS_API Spline : public Patterns::NotCopyable
  {
  public:
    Spline();
    ~Spline();

    Spline(Spline && spline);
    Spline & operator=(Spline && spline);

    void addControlPoint(Nimble::Vector2 point, Nimble::Vector4 color, float width, float time = 0.0f);
    void endPath();
    void clear();

    void erase(const Nimble::Rectangle & eraser, float time = 0.0f);

    void render(Luminous::RenderContext & r, float time = 0.0f) const;

    void recalculate();

    float endTime() const;

    void saveTo(QIODevice & io) const;
    void loadFrom(QIODevice & io);

  private:
    class D;
    D * m_d;
  };

}

#endif // SPLINE_HPP
