/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "BezierSplineFitter.hpp"
#include "CubicBezierCurve.hpp"

namespace Luminous
{
  struct ErrorResult
  {
    float maxErrorSqr = 0;
    size_t pointIdx = 0;
  };

  class BezierSplineFitter::D
  {
  public:
    static constexpr float s_epsilon = 1e-12f;

  public:
    // Fit a Bezier curve to a (sub)set of digitized points
    void fitCubic(BezierSpline & nodes, float error, size_t first, size_t last,
                  Nimble::Vector3f tan1, Nimble::Vector3f tan2);

    void addCurve(BezierSpline & nodes, const CubicBezierCurve & curve);

    // Use least-squares method to find Bezier control points for region.
    CubicBezierCurve generateBezier(size_t first, size_t last, const std::vector<float> & uPrime,
                                    Nimble::Vector3f tan1, Nimble::Vector3f tan2);

    // Given set of points and their parameterization, try to find
    // a better parameterization.
    bool reparameterize(size_t first, size_t last, std::vector<float> & u, const CubicBezierCurve & curve);

    // Use Newton-Raphson iteration to find better root.
    float findRoot(const CubicBezierCurve & curve, Nimble::Vector3f point, float u);

    // Evaluate a bezier curve at a particular parameter value
    Nimble::Vector3f evaluate(int degree, const Nimble::Vector3f * curve, float t);

    // Assign parameter values to digitized points
    // using relative distances between points.
    std::vector<float> chordLengthParameterize(size_t first, size_t last);

    // Find the maximum squared distance of digitized points to fitted curve.
    ErrorResult findMaxError(size_t first, size_t last, const CubicBezierCurve & curve,
                             const std::vector<float> & u);

  public:
    const Nimble::Vector3f * m_points;
    size_t m_pointsCount;
  };

  void BezierSplineFitter::D::fitCubic(
      BezierSpline & nodes, float error, size_t first, size_t last,
      Nimble::Vector3f tan1, Nimble::Vector3f tan2)
  {
    //  Use heuristic if region only has two points in it
    if (last - first == 1) {
      Nimble::Vector3f pt1 = m_points[first];
      Nimble::Vector3f pt2 = m_points[last];
      float dist = (pt1 - pt2).length() / 3.f;
      addCurve(nodes, { pt1, pt1 + tan1.normalized(dist),
               pt2 + tan2.normalized(dist), pt2 });
      return;
    }
    // Parameterize points, and attempt to fit curve
    std::vector<float> uPrime = chordLengthParameterize(first, last);
    float maxError = std::max(error, error * error);
    size_t split;
    bool parametersInOrder = true;
    // Try 4 iterations
    for (int i = 0; i <= 4; i++) {
      CubicBezierCurve curve = generateBezier(first, last, uPrime, tan1, tan2);
      //  Find max deviation of points to fitted curve
      const ErrorResult maxErrorRes = findMaxError(first, last, curve, uPrime);
      if (maxErrorRes.maxErrorSqr < error && parametersInOrder) {
        addCurve(nodes, curve);
        return;
      }
      split = maxErrorRes.pointIdx;
      // If error is too large, give up. Otherwise do reparameterization
      // and try again.
      if (maxErrorRes.maxErrorSqr >= maxError)
        break;
      parametersInOrder = reparameterize(first, last, uPrime, curve);
      maxError = maxErrorRes.maxErrorSqr;
    }
    // Fitting failed -- split at max error point and fit recursively
    Nimble::Vector3f tanCenter = m_points[split - 1] - m_points[split + 1];
    fitCubic(nodes, error, first, split, tan1, tanCenter);
    fitCubic(nodes, error, split, last, -tanCenter, tan2);
  }

  void BezierSplineFitter::D::addCurve(BezierSpline & nodes, const CubicBezierCurve & curve)
  {
    auto & prev = nodes.back();
    prev.ctrlOut = curve[1];
    nodes.push_back(BezierNode{curve[2], curve[3], curve[3]});
  }

  CubicBezierCurve BezierSplineFitter::D::generateBezier(
      size_t first, size_t last, const std::vector<float> & uPrime,
      Nimble::Vector3f tan1, Nimble::Vector3f tan2)
  {
    Nimble::Vector3f pt1 = m_points[first];
    Nimble::Vector3f pt2 = m_points[last];
    // Create the C and X matrices
    float C[2][2] = {{0, 0}, {0, 0}};
    float X[2] = {0, 0};

    for (size_t i = 0, l = last - first + 1; i < l; i++) {
      float u = uPrime[i];
      float t = 1.f - u;
      float b = 3.f * u * t;
      float b0 = t * t * t;
      float b1 = b * t;
      float b2 = b * u;
      float b3 = u * u * u;
      Nimble::Vector3f a1 = tan1.normalized(b1);
      Nimble::Vector3f a2 = tan2.normalized(b2);
      Nimble::Vector3f tmp = m_points[first + i] - pt1 * (b0 + b1) - pt2 * (b2 + b3);
      C[0][0] += dot(a1, a1);
      C[0][1] += dot(a1, a2);
      // Save one dot product by using the already calculated value
      C[1][0] = C[0][1];
      C[1][1] += dot(a2, a2);
      X[0] += dot(a1, tmp);
      X[1] += dot(a2, tmp);
    }

    // Compute the determinants of C and X
    float detC0C1 = C[0][0] * C[1][1] - C[1][0] * C[0][1];
    float alpha1, alpha2;
    if (std::abs(detC0C1) > s_epsilon) {
      // Cramer's rule
      float detC0X = C[0][0] * X[1]    - C[1][0] * X[0],
          detXC1 = X[0]    * C[1][1] - X[1]    * C[0][1];
      // Derive alpha values
      alpha1 = detXC1 / detC0C1;
      alpha2 = detC0X / detC0C1;
    } else {
      // Matrix is under-determined, try assuming alpha1 == alpha2
      float c0 = C[0][0] + C[0][1],
          c1 = C[1][0] + C[1][1];
      alpha1 = alpha2 = std::abs(c0) > s_epsilon ? X[0] / c0
          : std::abs(c1) > s_epsilon ? X[1] / c1
          : 0;
    }

    // If alpha negative, use the Wu/Barsky heuristic (see text)
    // (if alpha is 0, you get coincident control points that lead to
    // divide by zero in any subsequent NewtonRaphsonRootFind() call.
    float segLength = (pt2 - pt1).length();
    float eps = s_epsilon * segLength;
    Nimble::Vector3f handle1, handle2;
    bool recalc = true;
    if (alpha1 < eps || alpha2 < eps) {
      // fall back on standard (probably inaccurate) formula,
      // and subdivide further if needed.
      alpha1 = alpha2 = segLength / 3;
    } else {
      // Check if the found control points are in the right order when
      // projected onto the line through pt1 and pt2.
      Nimble::Vector3f line = pt2 - pt1;
      // Control points 1 and 2 are positioned an alpha distance out
      // on the tangent vectors, left and right, respectively
      handle1 = tan1.normalized(alpha1);
      handle2 = tan2.normalized(alpha2);
      if (dot(handle1, line) - dot(handle2, line) > segLength * segLength) {
        // Fall back to the Wu/Barsky heuristic above.
        alpha1 = alpha2 = segLength / 3;
        recalc = true; // Force recalculation
      } else {
        recalc = false;
      }
    }

    // First and last control points of the Bezier curve are
    // positioned exactly at the first and last data points
    return {pt1,
            pt1 + (recalc ? tan1.normalized(alpha1) : handle1),
            pt2 + (recalc ? tan2.normalized(alpha2) : handle2),
            pt2};
  }

  bool BezierSplineFitter::D::reparameterize(size_t first, size_t last, std::vector<float> & u, const CubicBezierCurve & curve)
  {
    for (size_t i = first; i <= last; i++)
      u[i - first] = findRoot(curve, m_points[i], u[i - first]);
    // Detect if the new parameterization has reordered the points.
    // In that case, we would fit the points of the path in the wrong order.
    for (size_t i = 1, l = u.size(); i < l; i++)
      if (u[i] <= u[i - 1])
        return false;

    return true;
  }

  float BezierSplineFitter::D::findRoot(const CubicBezierCurve & curve, Nimble::Vector3f point, float u)
  {
    Nimble::Vector3f curve1[3];
    Nimble::Vector3f curve2[2];
    // Generate control vertices for Q'
    for (int i = 0; i <= 2; i++)
      curve1[i] = (curve[i + 1] - curve[i]) * 3.f;
    // Generate control vertices for Q''
    for (int i = 0; i <= 1; i++)
      curve2[i] = (curve1[i + 1] - curve1[i]) * 2.f;
    // Compute Q(u), Q'(u) and Q''(u)
    Nimble::Vector3f pt = evaluate(3, curve.data(), u);
    Nimble::Vector3f pt1 = evaluate(2, curve1, u);
    Nimble::Vector3f pt2 = evaluate(1, curve2, u);
    Nimble::Vector3f diff = pt - point;
    float df = dot(pt1, pt1) + dot(diff, pt2);
    // u = u - f(u) / f'(u)
    return (df >= -s_epsilon && df <= s_epsilon) ? u : u - dot(diff, pt1) / df;
  }

  Nimble::Vector3f BezierSplineFitter::D::evaluate(int degree, const Nimble::Vector3f * curve, float t)
  {
    Nimble::Vector3f tmp[4];
    for (int i = 0; i <= degree; ++i)
      tmp[i] = curve[i];

    // Triangle computation
    for (int i = 1; i <= degree; i++)
      for (int j = 0; j <= degree - i; j++)
        tmp[j] = tmp[j] * (1 - t) + (tmp[j + 1] * t);
    return tmp[0];
  }

  std::vector<float> BezierSplineFitter::D::chordLengthParameterize(size_t first, size_t last)
  {
    std::vector<float> u(last-first+1);
    for (size_t i = first + 1; i <= last; i++) {
      u[i - first] = u[i - first - 1]
          + (m_points[i] - m_points[i - 1]).length();
    }
    for (size_t i = 1, m = last - first; i <= m; i++)
      u[i] /= u[m];
    return u;
  }

  ErrorResult BezierSplineFitter::D::findMaxError(
      size_t first, size_t last, const CubicBezierCurve & curve, const std::vector<float> & u)
  {
    ErrorResult res;
    res.pointIdx = (last - first + 1) / 2;
    for (size_t i = first + 1; i < last; i++) {
      Nimble::Vector3f P = evaluate(3, curve.data(), u[i - first]);
      Nimble::Vector3f v = P - m_points[i];
      float distSqr = v.lengthSqr();
      if (distSqr >= res.maxErrorSqr) {
        res.maxErrorSqr = distSqr;
        res.pointIdx = i;
      }
    }

    return res;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  BezierSplineFitter::BezierSplineFitter(const Nimble::Vector3f * points, size_t size)
    : m_d(new D())
  {
    m_d->m_points = points;
    m_d->m_pointsCount = size;
  }

  BezierSplineFitter::~BezierSplineFitter()
  {
  }

  BezierSpline BezierSplineFitter::fit(float maxErrorSqr, Nimble::Vector3f leftTangent,
                                       Nimble::Vector3f rightTangent) const
  {
    BezierSpline nodes;
    fit(nodes, maxErrorSqr, leftTangent, rightTangent);
    return nodes;
  }

  void BezierSplineFitter::fit(BezierSpline & nodes, float maxErrorSqr,
                               Nimble::Vector3f leftTangent, Nimble::Vector3f rightTangent) const
  {
    if (m_d->m_pointsCount > 0) {
      auto & p = m_d->m_points;
      // To support reducing paths with multiple points in the same place
      // to one segment:
      nodes.push_back(BezierNode{p[0], p[0], p[0]});
      if (m_d->m_pointsCount > 1) {
        m_d->fitCubic(nodes, maxErrorSqr, 0, m_d->m_pointsCount - 1,
            leftTangent.isZero() ? p[1] - p[0] : leftTangent,
            rightTangent.isZero() ? p[m_d->m_pointsCount - 2] - p[m_d->m_pointsCount - 1] : rightTangent);
      }
    }
  }
}
