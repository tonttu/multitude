/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef LUMINOUS_SPLINE_HPP
#define LUMINOUS_SPLINE_HPP

#include "Export.hpp"

#include <Nimble/Vector4.hpp>
#include <Nimble/Rectangle.hpp>

#include <Radiant/Color.hpp>

#include <Patterns/NotCopyable.hpp>

#include <set>

/// @cond

class QDataStream;

namespace Luminous {

  class RenderContext;

  class LUMINOUS_API Spline : public Patterns::NotCopyable
  {
  public:
    Spline();
    ~Spline();

    Spline(Spline && spline);
    Spline & operator=(Spline && spline);

    /// @param time use only posivive timestamps here, if negative time is given, it's clamped to 0
    void addControlPoint(Nimble::Vector2 point, Radiant::ColorPMA color, float width, float time = 0.0f);
    void endPath();
    void clear();

    void erase(const Nimble::Rectangle & eraser, float time = 0.0f,
               std::set<std::pair<size_t, size_t>> *erasedPoints=nullptr);

    void erasePermanent(const Nimble::Rectangle & eraser,
                        std::set<std::pair<size_t, size_t>> *erasedPoints=nullptr);

    void eraseWithTransparency(const Nimble::Rectangle & eraser,
                               std::set<std::pair<size_t, size_t>> *erasedPoints=nullptr);

    void render(Luminous::RenderContext & r, float time = 0.0f) const;

    void makeTransparent(const std::set<std::pair<size_t, size_t>>& points);
    void makeOpaque(const std::set<std::pair<size_t, size_t>>& points);

    void setCalculationParameters(float mingap, float maxgap);
    void recalculate();

    float beginTime() const;
    float endTime() const;

    /// @param points negative number == undo, positive == redo
    int undoRedo(int points);

    size_t controlPointCount() const;
    Nimble::Rect controlPointBounds() const;

    bool isEmpty() const;

    Spline clone() const;

    QString serialize() const;
    void deserialize(const QString& str);

  private:
    friend LUMINOUS_API QDataStream & operator<<(QDataStream & out, const Spline & spline);
    friend LUMINOUS_API QDataStream & operator>>(QDataStream & in, Spline & spline);

    class D;
    D * m_d;
  };

  LUMINOUS_API QDataStream & operator<<(QDataStream & out, const Spline & spline);
  LUMINOUS_API QDataStream & operator>>(QDataStream & in, Spline & spline);
}

/// @endcond

#endif // SPLINE_HPP
