/* COPYRIGHT
 */

#include "SplinesImpl.hpp"

#include <Nimble/Vector3.hpp>

template Nimble::Vector3 Nimble::evalCatmullRom(float t, const std::vector<Nimble::Vector3> & cp, size_t index);


namespace Nimble
{

  Vector2 Interpolating::get(float t) const
  {
    if (m_points.empty()) return Vector2(0, 0);
    if (t <= 0) return m_points.front();
    if (t >= size()-1) return m_points.back();

    size_t ii = static_cast<int>(t);
    t -= ii;

    return getPoint(ii, t);
  }


  Vector2 Interpolating::get(size_t ii, float h1, float h2, float h3, float h4) const
  {
    return h1 * m_points[ii]   + h2 * m_points[ii+1]
        + h3 * m_tangents[ii] + h4 * m_tangents[ii+1];
  }

  Vector2 Interpolating::getPoint(size_t ii, float t) const
  {
    // Hermite curve
    float tt = t*t;
    float ttt = tt*t;
    float h2 = 3*tt - 2*ttt;
    float h1 = 1 - h2;
    float h3 = ttt - 2*tt + t;
    float h4 = ttt - tt;

    return get(ii, h1, h2, h3, h4);
  }

  Vector2 Interpolating::getDerivative(size_t ii, float t) const
  {
    // derivative of getPoint with respect to t.
    float tt = t*t;
    float h1 = 6*tt - 6*t;
    float h3 = 3*tt - 4*t + 1;
    float h4 = 3*tt - 2*t;

    return get(ii, h1, -h1, h3, h4);
  }

  void Interpolating::add(Vector2 point)
  {
    if (size() == 0) {
      m_points.push_back(point);
      m_tangents.push_back(Vector2(0, 0));
      return;
    }
    size_t last = m_points.size() - 1;

    if (last >= 1) m_tangents[last] = 0.5f * (point - m_points[last-1]);
    m_points.push_back(point);
    m_tangents.push_back(0.1f * (point - m_points[last]));
    last++;
  }

  void Interpolating::remove(size_t ii) {
    m_points.erase(m_points.begin() + ii);
    m_tangents.erase(m_tangents.begin() + ii);
  }

  void Interpolating::clear()
  {
    m_points.clear();
    m_tangents.clear();
  }
}
