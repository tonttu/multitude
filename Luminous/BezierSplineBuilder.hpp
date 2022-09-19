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

#include "BezierSpline.hpp"

#include <Nimble/Rect.hpp>

#include <memory>

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
    /// @param maxStrokeRadiusRate Limits how quickly the stroke radius
    ///        (half of the stroke width) can change between two consecutive
    ///        points. Relative to the distance between the two points.
    BezierSplineBuilder(BezierSpline & path, float maxStrokeRadiusRate);
    ~BezierSplineBuilder();

    /// Adds a new sample point to the builder. Based on the parameters this
    /// might add, remove or change couple of the last control points in the spline.
    /// @param p New unfiltered point to add to the builder. Interpretes the .z
    ///        component as half of the stroke width.
    /// @param noiseThreshold expected maximum noise from a stationary object,
    ///        used to filter out small movements
    /// @param maxFitErrorSqr see maxErrorSqr parameter in BezierSplineFitter::fit
    /// @returns number of stable points in the output path. Stable points do not
    ///          change in the following calls to this function.
    size_t addPoint(Nimble::Vector3f p, float noiseThreshold, float maxFitErrorSqr, float fitErrorAcc = 0);

    /// Bounding box of all spline control points, taking account the spline width
    const Nimble::Rectf & bounds() const;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}
