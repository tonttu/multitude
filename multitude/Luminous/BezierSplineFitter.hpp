#pragma once

#include "Export.hpp"
#include "BezierSpline.hpp"

namespace Luminous
{
  /// Fits a cubic Bezier spline to an array of points. This is an
  /// implementation of An Algorithm for Automatically Fitting Digitized Curves
  /// by Philip J. Schneider from "Graphics Gems", Academic Press, 1990.
  /// This is also based on code by Juerg Lehni in paper.js project:
  /// https://github.com/paperjs/paper.js/blob/develop/src/path/PathFitter.js
  class LUMINOUS_API BezierSplineFitter
  {
  public:
    /// @param points an array of points that will not be copied to the
    ///        fitter class. Make sure the pointer remains valid until
    ///        this class is destroyed.
    BezierSplineFitter(const Nimble::Vector3f * points, size_t size);
    ~BezierSplineFitter();

    /// Returns the generated spline in a new vector
    /// @param maxErrorSqr squared max error between points and the fitted curve
    BezierSpline fit(float maxErrorSqr, Nimble::Vector3f leftTangent = {0, 0, 0},
                     Nimble::Vector3f rightTangent = {0, 0, 0}) const;

    /// Inserts the generated spline points at the end of the given vector
    void fit(BezierSpline & nodes, float maxErrorSqr,
             Nimble::Vector3f leftTangent = {0, 0, 0}, Nimble::Vector3f rightTangent = {0, 0, 0}) const;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}
