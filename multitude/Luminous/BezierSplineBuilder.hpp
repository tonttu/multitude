#pragma once

#include "BezierSplineFitter.hpp"

#include <Nimble/Rect.hpp>

namespace Luminous
{
  /// Build a Bezier spline one point at a time from point series. This is
  /// meant for interactive use for generating a spline from touch / pen
  /// movement and the class only recalculates as small part of the spline
  /// as possible to minimize any visible noise.
  ///
  /// If you already have a ready point array, use BezierSplineFitter instead
  /// to get fewer control points and slightly smoother spline.
  class LUMINOUS_API BezierSplineBuilder
  {
  public:
    /// @param path The generated spline, updated after every addPoint call
    BezierSplineBuilder(std::vector<BezierNode> & path);
    ~BezierSplineBuilder();

    /// Adds a new sample point to the builder. Based on the parameters this
    /// might add, remove or change couple of the last control points in the spline.
    /// @param p New unfiltered point to add to the builder
    /// @param noiseThreshold expected maximum noise from a stationary object,
    ///        used to filter out small movements
    /// @param maxFitErrorSqr see maxErrorSqr parameter in BezierSplineFitter::fit
    void addPoint(BezierSplineFitter::Point p, float noiseThreshold, float maxFitErrorSqr);

    /// Bounding box of all spline control points, taking account the spline width
    const Nimble::Rectf & bounds() const;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}
