#pragma once

#include "Export.hpp"
#include "BezierCurve.hpp"

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
    struct Point
    {
      Nimble::Vector2f point;
      float strokeWidth;
    };

  public:
    /// @param points an array of points that will not be copied to the
    ///        fitter class. Make sure the pointer remains valid until
    ///        this class is destroyed.
    BezierSplineFitter(const Point * points, size_t size);
    ~BezierSplineFitter();

    /// Returns the generated spline in a new vector
    /// @param maxErrorSqr squared max error between points and the fitted curve
    std::vector<BezierNode> fit(float maxErrorSqr, Nimble::Vector2f leftTangent = {0, 0},
                                Nimble::Vector2f rightTangent = {0, 0}) const;

    /// Inserts the generated spline points at the end of the given vector
    void fit(std::vector<BezierNode> & nodes, float maxErrorSqr,
             Nimble::Vector2f leftTangent = {0, 0}, Nimble::Vector2f rightTangent = {0, 0}) const;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };
}
