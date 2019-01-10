#include "BezierSplineBuilder.hpp"

namespace Luminous
{
  class BezierSplineBuilder::D
  {
  public:
    D(BezierSpline & path)
      : m_path(path)
    {}

  public:
    std::vector<BezierSplineFitter::Point> m_inputPoints;
    BezierSpline & m_path;
    /// This is here to avoid allocating new vector every frame
    BezierSpline m_tmpPath;
    Nimble::Rectf m_readyBounds;
    Nimble::Rectf m_mutableBounds;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  BezierSplineBuilder::BezierSplineBuilder(BezierSpline & path)
    : m_d(new D(path))
  {
  }

  BezierSplineBuilder::~BezierSplineBuilder()
  {
  }

  void BezierSplineBuilder::addPoint(BezierSplineFitter::Point p, float noiseThreshold, float maxFitErrorSqr)
  {
    auto size = m_d->m_inputPoints.size();
    if (noiseThreshold > 0 && size >= 2) {
      Nimble::Vector2f test = m_d->m_inputPoints[size - 2].point;
      if ((test - p.point).length() < noiseThreshold) {
        auto & back = m_d->m_inputPoints.back();
        back.point = p.point;
        back.strokeWidth = std::max(back.strokeWidth, p.strokeWidth);
      } else {
        m_d->m_inputPoints.push_back(p);
        ++size;
      }
    } else {
      m_d->m_inputPoints.push_back(p);
      ++size;
    }

    size_t firstAddedNodeIndex;

    if (m_d->m_path.size() <= 2) {
      Luminous::BezierSplineFitter pathFitter(m_d->m_inputPoints.data(), m_d->m_inputPoints.size());
      m_d->m_path.clear();
      pathFitter.fit(m_d->m_path, maxFitErrorSqr);
      firstAddedNodeIndex = 0;
    } else {
      m_d->m_path.pop_back();

      auto p = m_d->m_path.back();
      for (--size; size > 0; --size)
        if (m_d->m_inputPoints[size].point == p.point)
          break;

      Luminous::BezierSplineFitter pathFitter(m_d->m_inputPoints.data() + size, m_d->m_inputPoints.size() - size);
      BezierSpline & newPath = m_d->m_tmpPath;
      newPath.clear();
      pathFitter.fit(newPath, maxFitErrorSqr, p.point - p.ctrlIn);
      newPath[0].ctrlIn = p.ctrlIn;

      m_d->m_path.pop_back();
      firstAddedNodeIndex = m_d->m_path.size();
      m_d->m_path.insert(m_d->m_path.end(), newPath.begin(), newPath.end());
    }

    // The algorithm here works so that it replaces the last two BezierNodes
    // with a totally new bezier spline. To get accurate bounding box of the
    // spline incrementally, we handle the last two points separately.
    auto it = m_d->m_path.begin() + firstAddedNodeIndex;
    auto fixedEnd = m_d->m_path.end() - std::min<size_t>(2, m_d->m_path.size());

    while (it < fixedEnd) {
      BezierNode & node = *it;
      const float radius = 0.5f * node.strokeWidth;
      m_d->m_readyBounds.expand(node.point, radius);
      m_d->m_readyBounds.expand(node.ctrlIn, radius);
      m_d->m_readyBounds.expand(node.ctrlOut, radius);

      ++it;
    }

    m_d->m_mutableBounds = m_d->m_readyBounds;
    while (it != m_d->m_path.end()) {
      BezierNode & node = *it;
      const float radius = 0.5f * node.strokeWidth;
      m_d->m_mutableBounds.expand(node.point, radius);
      m_d->m_mutableBounds.expand(node.ctrlIn, radius);
      m_d->m_mutableBounds.expand(node.ctrlOut, radius);

      ++it;
    }
  }

  const Nimble::Rectf & BezierSplineBuilder::bounds() const
  {
    return m_d->m_mutableBounds;
  }
}
