#include "BezierSplineBuilder.hpp"
#include "BezierSplineFitter.hpp"

namespace Luminous
{
  class BezierSplineBuilder::D
  {
  public:
    D(BezierSpline & path, float maxStrokeRadiusRate)
      : m_path(path)
      , m_maxStrokeRadiusRate(maxStrokeRadiusRate)
    {}

    inline void addPoint(Nimble::Vector3f p);

  public:
    std::vector<Nimble::Vector3f> m_inputPoints;
    BezierSpline & m_path;
    /// This is here to avoid allocating new vector every frame
    BezierSpline m_tmpPath;
    Nimble::Rectf m_stableBounds;
    Nimble::Rectf m_mutableBounds;
    float m_maxStrokeRadiusRate;
  };

  void BezierSplineBuilder::D::addPoint(Nimble::Vector3f p)
  {
    if (m_inputPoints.empty()) {
      m_inputPoints.push_back(p);
      return;
    }

    Nimble::Vector3f prev = m_inputPoints.back();
    float lenDiffSqr = (prev.vector2() - p.vector2()).lengthSqr();
    float radiusDiff = p.z - prev.z;
    if (radiusDiff * radiusDiff > m_maxStrokeRadiusRate * m_maxStrokeRadiusRate * lenDiffSqr)
      p.z = prev.z + std::copysign(m_maxStrokeRadiusRate * std::sqrt(lenDiffSqr), radiusDiff);
    m_inputPoints.push_back(p);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  BezierSplineBuilder::BezierSplineBuilder(BezierSpline & path, float maxStrokeRadiusRate)
    : m_d(new D(path, maxStrokeRadiusRate))
  {
  }

  BezierSplineBuilder::~BezierSplineBuilder()
  {
  }

  size_t BezierSplineBuilder::addPoint(Nimble::Vector3f p, float noiseThreshold, float maxFitErrorSqr, float fitErrorAcc)
  {
    auto size = m_d->m_inputPoints.size();
    if (noiseThreshold > 0 && size >= 2) {
      Nimble::Vector2f test = m_d->m_inputPoints[size - 2].vector2();
      if ((test - p.vector2()).length() < noiseThreshold) {
        auto & back = m_d->m_inputPoints.back();
        back.x = p.x;
        back.y = p.y;
        back.z = std::max(back.z, p.z);
      } else {
        m_d->addPoint(p);
        ++size;
      }
    } else {
      m_d->addPoint(p);
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
        if (m_d->m_inputPoints[size] == p.point)
          break;

      Luminous::BezierSplineFitter pathFitter(m_d->m_inputPoints.data() + size, m_d->m_inputPoints.size() - size);
      BezierSpline & newPath = m_d->m_tmpPath;
      newPath.clear();
      float err = maxFitErrorSqr;
      if (fitErrorAcc > 0) {
        float speed = (m_d->m_inputPoints.back().vector2() - m_d->m_inputPoints[m_d->m_inputPoints.size()-2].vector2()).length();
        float tmp = std::max(1.f, speed/fitErrorAcc);
        err *= tmp*tmp;
      }
      pathFitter.fit(newPath, err, p.point - p.ctrlIn);
      newPath[0].ctrlIn = p.ctrlIn;

      m_d->m_path.pop_back();
      firstAddedNodeIndex = m_d->m_path.size();
      m_d->m_path.insert(m_d->m_path.end(), newPath.begin(), newPath.end());
    }

    // The algorithm here works so that it replaces the last two BezierNodes
    // with a totally new bezier spline. To get accurate bounding box of the
    // spline incrementally, we don't cache the bounding box of the last two
    // bezier curves.
    auto it = m_d->m_path.data() + (std::max<size_t>(1, firstAddedNodeIndex) - 1);
    auto end = m_d->m_path.data() + m_d->m_path.size();
    auto stableEnd = end - std::min<size_t>(2, m_d->m_path.size());
    auto mutableBegin = end - std::min<size_t>(3, m_d->m_path.size());

    m_d->m_stableBounds.expand(splineBounds2D(it, stableEnd));

    m_d->m_mutableBounds = m_d->m_stableBounds;
    m_d->m_mutableBounds.expand(splineBounds2D(mutableBegin, end));

    return std::max<size_t>(m_d->m_path.size(), 2) - 2;
  }

  const Nimble::Rectf & BezierSplineBuilder::bounds() const
  {
    return m_d->m_mutableBounds;
  }
}
