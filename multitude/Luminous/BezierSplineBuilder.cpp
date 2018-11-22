#include "BezierSplineBuilder.hpp"

namespace Luminous
{
  class BezierSplineBuilder::D
  {
  public:
    std::vector<BezierSplineFitter::Point> m_inputPoints;
    std::vector<BezierNode> m_path;
    Nimble::Rectf m_bounds;
  };

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  BezierSplineBuilder::BezierSplineBuilder()
    : m_d(new D())
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

    if (m_d->m_path.size() <= 2) {
      Luminous::BezierSplineFitter pathFitter(m_d->m_inputPoints.data(), m_d->m_inputPoints.size());
      m_d->m_path = pathFitter.fit(maxFitErrorSqr);
      for (auto & node: m_d->m_path) {
        const float radius = 0.5f * node.strokeWidth;
        m_d->m_bounds.expand(node.point, radius);
        m_d->m_bounds.expand(node.ctrlIn, radius);
        m_d->m_bounds.expand(node.ctrlOut, radius);
      }
    } else {
      m_d->m_path.pop_back();

      auto p = m_d->m_path.back();
      for (--size; size > 0; --size)
        if (m_d->m_inputPoints[size].point == p.point)
          break;

      Luminous::BezierSplineFitter pathFitter(m_d->m_inputPoints.data() + size, m_d->m_inputPoints.size() - size);
      auto newPath = pathFitter.fit(maxFitErrorSqr, p.point - p.ctrlIn);
      newPath[0].ctrlIn = p.ctrlIn;
      for (auto & node: newPath) {
        const float radius = 0.5f * node.strokeWidth;
        m_d->m_bounds.expand(node.point, radius);
        m_d->m_bounds.expand(node.ctrlIn, radius);
        m_d->m_bounds.expand(node.ctrlOut, radius);
      }

      m_d->m_path.pop_back();
      m_d->m_path.insert(m_d->m_path.end(), newPath.begin(), newPath.end());
    }
  }

  const std::vector<BezierNode> & BezierSplineBuilder::path() const
  {
    return m_d->m_path;
  }

  const Nimble::Rectf & BezierSplineBuilder::bounds() const
  {
    return m_d->m_bounds;
  }
}
