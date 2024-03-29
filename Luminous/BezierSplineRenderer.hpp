/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#pragma once

#include "Export.hpp"
#include "BezierSpline.hpp"

#include <Nimble/Rect.hpp>

#include <Radiant/Color.hpp>

#include <Valuable/Node.hpp>

#include <atomic>
#include <memory>

namespace Luminous
{
  class RenderContext;

  /// Renderer for bezier splines. Uses level of detail triangle strip mipmaps
  /// for optimizing the number of vertices and the rendering quality.
  class LUMINOUS_API BezierSplineRenderer
  {
  public:
    struct RenderStats
    {
      std::atomic<uint32_t> renderedVertices{0};
      std::atomic<uint32_t> renderedStrokes{0};
      std::atomic<uint32_t> totalStrokes{0};

      void clear()
      {
        renderedVertices = 0;
        renderedStrokes = 0;
        totalStrokes = 0;
      }
    };

    /// One continuous bezier spline with constant color
    struct Stroke
    {
      /// If this is zero, it's set automatically to new value in addStroke
      Valuable::Node::Uuid id = 0;
      /// If this is empty, the rect is calculated automatically
      Nimble::Rect bbox;
      const BezierSpline * path = nullptr;
      Radiant::ColorPMA color{1, 1, 1, 1};
      float depth = 0;
      SplineStyle style;
    };

    struct RenderOptions
    {
      /// minScale and maxScale specify the expected scaling range in the
      /// RenderContext (see RenderContext::approximateScaling) given to
      /// render(). The size of the range defines how many mipmap levels we are
      /// using. To optimize memory consumption, set the range to as tight as
      /// possible.
      ///
      /// If we ever render with a scale smaller than minScale, we end up
      /// rendering too many vertices, which is inefficient. Respectively, if
      /// we render with a scale larger than maxScale, the spline visual
      /// quality suffers. On the other hand, if the range is too large, we
      /// allocate too big vectors for mipmaps.
      ///
      /// You should probably never change the defaults.
      float minScale = 0.001f;
      float maxScale = 10000.f;
      /// See maxCurveError parameter in Luminous::BezierSplineTessellator::BezierSplineTessellator
      float maxCurveError = 0.2f;
      /// See maxRoundCapError parameter in Luminous::BezierSplineTessellator::BezierSplineTessellator
      float maxRoundCapError = 0.2f;

      /// Set to non-null to receive debug information about rendering
      std::shared_ptr<RenderStats> stats;
      /// Set to true to render the splines as line strips instead of triangle
      /// strips. This is used just for debugging.
      bool renderAsLineStrip = false;

      /// Skip bounding-box checking and assume all strokes are always visible.
      /// The bbox checking only works properly with 2D transformations, this
      /// needs to be set if the splines are rendered using a more complicate
      /// transformations.
      bool forceRendering = false;
    };

  public:
    /// Ideally we would write BezierSplineRenderer(RenderOptions opts = RenderOptions()),
    /// but that triggers a bug in both clang and gcc:
    /// https://bugs.llvm.org/show_bug.cgi?id=36684
    /// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=88165
    BezierSplineRenderer();
    BezierSplineRenderer(RenderOptions opts);

    ~BezierSplineRenderer();

    /// Remove all strokes
    void clear();

    RenderOptions renderOptions() const;
    void setRenderOptions(RenderOptions opts);

    /// Adds a new stroke to the renderer. Returns a new generated id if the
    /// given stroke didn't have non-zero id.
    Valuable::Node::Uuid addStroke(Stroke s);
    Stroke takeStroke(Valuable::Node::Uuid id);
    Stroke stroke(Valuable::Node::Uuid id) const;

    /// Update stroke bezier spline data. Bounding box is calculated
    /// automatically or it can be given as a parameter.
    void setStrokePath(Valuable::Node::Uuid id, const BezierSpline * path, Nimble::Rect bbox = Nimble::Rect());
    void setStrokeColor(Valuable::Node::Uuid id, Radiant::ColorPMA color);
    void setStrokeDepth(Valuable::Node::Uuid id, float depth);
    void setStrokeStyle(Valuable::Node::Uuid id, SplineStyle style);

    void render(RenderContext & r) const;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}
