#include "SplineManager.hpp"

#include <Luminous/RenderContext.hpp>
#include <Luminous/VertexDescription.hpp>

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector4.hpp>

#include <iterator>

namespace
{

  class Vertex
  {
  public:
    Vertex()
      : location(0, 0)
      , color(0, 0, 0, 0)
    {
    }

    Nimble::Vector2f location;
    Nimble::Vector4f color;
  };

  Luminous::SplineManager::SplineInfo createInfo(Valuable::Node::Uuid id,
                                        const Luminous::SplineManager::SplineData & data)
  {
    Luminous::SplineManager::SplineInfo info;
    info.id = id;
    info.data = data;
    return info;
  }


  //////////////////////////////////////////////////////////////////

  class SplineInternal
  {
  public:
    int size() const;

    void addPoint(const Luminous::SplineManager::Point & point, float minimumDistance = 1.f);

    void processPoints();
    void processPoint(const Luminous::SplineManager::Point & point, size_t index, bool newPoint = true,
                      bool fitCurve = true);

    bool erase(const Nimble::Rectf & eraser, const Nimble::Matrix3f & transformer,
               Luminous::SplineManager::Splines & newStrokes);

    bool erase(const Nimble::Circle & eraser,
               Luminous::SplineManager::Splines & newStrokes);

    Luminous::SplineManager::SplineData segment(int low, int high = -1) const;
    void saveSegment(int low, int high, Luminous::SplineManager::Splines & newStrokes,
                     const Luminous::SplineManager::Points * start = nullptr,
                     const Luminous::SplineManager::Points * end = nullptr);

    void saveSegment(int low, int high, Luminous::SplineManager::Splines & newStrokes,
                     const std::array<Nimble::Vector2f, 4>& points);

    Luminous::SplineManager::SplineData m_data;
    Nimble::Rectf m_bounds;
    std::vector<Luminous::BezierCurve> m_curves;
    bool m_finished = false;
    static const int m_pointsPerCurve = 3; // 4 control points for cubic bezier, but start point is the end point of previous curve
    std::vector<Vertex> m_vertices;
  };

  int SplineInternal::size() const
  {
    return m_data.points.size();
  }

  void SplineInternal::addPoint(const Luminous::SplineManager::Point & point, float minimumDistance)
  {
    auto index = m_data.points.size() -1;
    if (index > 0) {
      auto offset = index == 1 ? 1 : m_pointsPerCurve;
      auto prev = m_data.points[index - offset];
      if ((prev - point).length() < minimumDistance) {
        m_data.points.pop_back();
        m_data.points.append(point);
        processPoint(point, index, false);
        return;
      }
    }
    m_data.points.append(point);
    processPoint(point, index + 1);
    return;
  }

  void SplineInternal::processPoints()
  {
    m_bounds = Nimble::Rectf();
    m_curves.clear();
    for (auto i = 0; i < m_data.points.size(); i += m_pointsPerCurve) {
      processPoint(m_data.points.at(i), i, true, false);
    }
  }

  void SplineInternal::processPoint(const Luminous::SplineManager::Point & point, size_t index, bool newPoint, bool fitCurve)
  {
    if (index == 0) {
      // first point, nothing to draw here
      m_bounds.expand(point);
      return;
    }
    if (newPoint) {
      Luminous::BezierCurve newCurve;
      m_curves.push_back(newCurve);
    }
    Luminous::BezierCurve & curve = m_curves.back();
    if (fitCurve) {
      if (!newPoint) {
        // We're refitting existing curve, remove previous control points and previous end point
        for (int i = 0; i < m_pointsPerCurve -1; i++)
          m_data.points.pop_back();
      }
      m_data.points.pop_back();
      // current curve's start point
      Luminous::SplineManager::Point start = m_data.points.takeLast();

      if (m_curves.size() > 1) {
        // fit curve to previous
        auto& prevCurve = m_curves[m_curves.size() -2];
        curve.setEndPoints(start, point);
        Luminous::BezierCurve::fitCurves(prevCurve, curve);
        // previous curve's last control point can change in refitting, so rewrite it
        m_data.points.pop_back();
        m_data.points.push_back(prevCurve[m_pointsPerCurve -1]);
      }
      else {
        curve.setEndPoints(start, point);
      }

      for (int i = 0; i <= m_pointsPerCurve; i++)
        m_data.points.push_back(curve[i]);
    }
    else {
      for (int i = m_pointsPerCurve; i >= 0; i--) {
        curve.m_controlPoints[m_pointsPerCurve - i] = m_data.points[index - i];
      }
    }
    // Stroke bounds doesn't include stroke width, only the control points.
    // @todo: handle width if erasing takes width into account
    m_bounds.expand(curve.bounds());
  }

  bool SplineInternal::erase(const Nimble::Rectf & eraser, const Nimble::Matrix3f & transformer,
                     Luminous::SplineManager::Splines & newStrokes)
  {
    bool shouldRemove = false;
    int low = -1, high = -1;
    Luminous::SplineManager::Points extraEnd, extraStartCurrent, extraStartNext;
    Luminous::BezierCurve left, right;

    // split the stroke to new strokes where erased
    for (size_t i = 0; i < m_curves.size(); i++) {
      Luminous::BezierCurve curve = m_curves[i];
      // transform the curve to normalized eraser coordinates for easier calculations
      for (size_t j = 0; j < curve.m_controlPoints.size(); j++) {
        curve.m_controlPoints[j] = transformer.project(curve[j]);
      }
      Nimble::Rectf curveBounds = curve.bounds();
      bool hit = false;

      if (curveBounds.intersects(eraser)) {
        if (eraser.contains(curveBounds)) {
          extraStartCurrent = extraStartNext;
          extraStartNext.clear();
          // remove the segment
          hit = true;
        }
        else {
          std::vector<float> intersections;
          Luminous::BezierCurve::findIntersections(curve, eraser, intersections);

          // split the segment
          if (intersections.size() > 0) {
            extraStartCurrent = extraStartNext;
            extraStartNext.clear();

            hit = true;
            bool take;
            Luminous::BezierCurve original = m_curves[i];
            Luminous::BezierCurve::subdivideCurve(original, left, right, intersections[0]);

            // add first segment to current curve if start point is not erased
            if (std::abs(curve[0].x) <= 1 && std::abs(curve[0].y) <= 1)
              take = false;
            else
              take = true;

            if (take) {
              for(size_t ind = 0; ind < left.m_controlPoints.size(); ++ind)
                extraEnd.append(left.m_controlPoints[ind]);
            }
            take = !take;
            float part = intersections[0];

            for (size_t index = 1; index < intersections.size(); index++) {
              // calculate t matching the intersection in the remaining curve
              float t =  (intersections[index] - part) / (1.f - part);
              Luminous::BezierCurve::subdivideCurve(right, left, right, t);
              part = intersections[index];
              // make a new stroke for the segment
              if (take) {
                saveSegment(-1, -1, newStrokes, left.m_controlPoints);
              }
              take = !take;
            }

            // add last segment to next curve if end point is not erased
            if (!(std::abs(curve[m_pointsPerCurve].x) <= 1 && std::abs(curve[m_pointsPerCurve].y) <= 1))
              for(size_t ind = 0; ind < right.m_controlPoints.size(); ++ind)
                extraStartNext.append(right[ind]);
          }
        }
      }
      if (hit) {
        shouldRemove = true;
        // save the current continuous segment as a new stroke and start a new segment
        if ((low >= 0 && high >= 0) || !extraEnd.isEmpty() || !extraStartCurrent.isEmpty()) {
          saveSegment(low, high, newStrokes, &extraStartCurrent, &extraEnd);

          low = -1; high = -1;
          extraEnd.clear();
        }
      }
      else {
        if (low < 0)
          low = i * (m_pointsPerCurve);
        high = (i + 1) * (m_pointsPerCurve);
      }
    }
    // save the current continuous segment as a new stroke
    if ((low > 0 && high > 0) || !extraStartNext.isEmpty()) {
      saveSegment(low, -1, newStrokes, &extraStartNext, &extraEnd);
    }

    return shouldRemove;
  }

  bool SplineInternal::erase(const Nimble::Circle & eraser,
                             Luminous::SplineManager::Splines & newStrokes)
  {
    Nimble::Rect eraserRect = eraser.boundingBox();

    bool shouldRemove = false;
    int low = -1, high = -1;
    Luminous::SplineManager::Points extraEnd, extraStartCurrent, extraStartNext;
    Luminous::BezierCurve left, right;

    // split the stroke to new strokes where erased
    for (size_t i = 0; i < m_curves.size(); i++) {
      Luminous::BezierCurve curve = m_curves[i];
      Nimble::Rectf curveBounds = curve.bounds();
      bool hit = false;

      if (curveBounds.intersects(eraserRect)) {
        if (eraser.contains(curveBounds)) {
          extraStartCurrent = extraStartNext;
          extraStartNext.clear();
          // remove the segment
          hit = true;
        }
        else {
          std::vector<float> intersections;
          Luminous::BezierCurve::findIntersections(curve, eraser, intersections);

          // split the segment
          if (intersections.size() > 0) {
            extraStartCurrent = extraStartNext;
            extraStartNext.clear();

            hit = true;
            bool take;
            Luminous::BezierCurve original = m_curves[i];
            Luminous::BezierCurve::subdivideCurve(original, left, right, intersections[0]);

            // add first segment to current curve if start point is not erased
            if (eraser.contains(Nimble::Vector2f(curve[0].x,curve[0].y)))
              take = false;
            else
              take = true;

            if (take) {
              for(size_t ind = 0; ind < left.m_controlPoints.size(); ++ind)
                extraEnd.append(left[ind]);
            }
            take = !take;
            float part = intersections[0];

            for (size_t index = 1; index < intersections.size(); index++) {
              // calculate t matching the intersection in the remaining curve
              float t =  (intersections[index] - part) / (1.f - part);
              Luminous::BezierCurve::subdivideCurve(right, left, right, t);
              part = intersections[index];
              // make a new stroke for the segment
              if (take) {
                saveSegment(-1, -1, newStrokes, left.m_controlPoints);
              }
              take = !take;
            }

            // add last segment to next curve if end point is not erased
            if (!eraser.contains(Nimble::Vector2f(curve[m_pointsPerCurve].x,curve[m_pointsPerCurve].y)))
              for(size_t ind = 0; ind < right.m_controlPoints.size(); ++ind)
                extraStartNext.append(right[ind]);
          }
        }
      }
      if (hit) {
        shouldRemove = true;
        // save the current continuous segment as a new stroke and start a new segment
        if ((low >= 0 && high >= 0) || !extraEnd.isEmpty() || !extraStartCurrent.isEmpty()) {
          saveSegment(low, high, newStrokes, &extraStartCurrent, &extraEnd);

          low = -1; high = -1;
          extraEnd.clear();
        }
      }
      else {
        if (low < 0)
          low = i * (m_pointsPerCurve);
        high = (i + 1) * (m_pointsPerCurve);
      }
    }
    // save the current continuous segment as a new stroke
    if ((low > 0 && high > 0) || !extraStartNext.isEmpty()) {
      saveSegment(low, -1, newStrokes, &extraStartNext, &extraEnd);
    }

    return shouldRemove;
  }


  Luminous::SplineManager::SplineData SplineInternal::segment(int low, int high) const
  {
    Luminous::SplineManager::SplineData newData;
    newData.color = m_data.color;
    newData.width = m_data.width;
    newData.depth = m_data.depth;
    if (low >= 0)
      newData.points.append(m_data.points.mid(low, high >= 0 ? (high - low +1): -1));
    return newData;
  }

  void SplineInternal::saveSegment(int low, int high, Luminous::SplineManager::Splines & newStrokes,
                                   const Luminous::SplineManager::Points * start,
                                   const Luminous::SplineManager::Points * end)
  {
    Luminous::SplineManager::SplineInfo info;
    info.id = Valuable::Node::generateId();
    info.data = segment(low, high);
    if (start && !start->isEmpty()) {
      if (!info.data.points.isEmpty())
        info.data.points.pop_front();
      info.data.points = *start + info.data.points;
    }
    if (end && !end->isEmpty()) {
      if (!info.data.points.isEmpty())
        info.data.points.pop_back();
      info.data.points.append(*end);
    }
    newStrokes.append(info);
  }

  void SplineInternal::saveSegment(int low, int high, Luminous::SplineManager::Splines & newStrokes,
                                   const std::array<Nimble::Vector2f, 4>& start)
  {
    Luminous::SplineManager::SplineInfo info;
    info.id = Valuable::Node::generateId();
    info.data = segment(low, high);
    if (!info.data.points.isEmpty())
      info.data.points.pop_front();

    for(size_t i = 0; i < start.size(); ++i)
      info.data.points.prepend(start[i]);
    newStrokes.append(info);
  }
}

  ///////////////////////////////////////////////////////////////////

namespace Luminous
{

  Nimble::Vector2f BezierCurve::operator[](int pos) const
  {
    return m_controlPoints.at(pos);
  }

  Nimble::Vector2f& BezierCurve::operator[](int pos)
  {
    return m_controlPoints[pos];
  }

  void BezierCurve::setEndPoints(const Nimble::Vector2f &start, const Nimble::Vector2f &end)
  {
    Nimble::Vector2f p1, p2;
    p1 = start + 1.f / 3.f * (end - start);
    p2 = 0.5f * (end + p1);
    m_controlPoints = { start, p1, p2, end };
  }

  void BezierCurve::fitCurves(BezierCurve &prev, BezierCurve &next)
  {
    const float k = 0.2f;
    Nimble::Vector2f d = k * (next[3] - prev[0]);
    prev[2] = next[0] - d;
    prev[3] = next[0];

    next[1] = next[0] + d;
    next[2] = 0.5f*(next[1] + next[3]);
  }

  Nimble::Rectf BezierCurve::bounds() const
  {
    Nimble::Rectf bb;
    for (const auto & point : m_controlPoints)
      bb.expand(point);
    return bb;
  }

  float BezierCurve::size() const
  {
    // length of bounding box's diagonal
    auto bb = bounds();
    return (bb.high() - bb.low()).length();
  }

  void BezierCurve::evaluateCurve(const BezierCurve &curve, std::vector<std::pair<Nimble::Vector2f, float> > &points,
                                  float begin, float end)
  {
    if(isFlat(curve)) {
      points.push_back(std::make_pair(curve.m_controlPoints.back(), end));
      return;
    }

    BezierCurve left, right;
    const float t = 0.5f;
    subdivideCurve(curve, left, right, t);
    float mid = begin + (end - begin)*t;
    evaluateCurve(left, points, begin, mid);
    evaluateCurve(right,points, mid, end);
  }

  void BezierCurve::evaluateCurve(const BezierCurve & curve, Luminous::SplineManager::Points & points)
  {
    if (isFlat(curve)) {
      points.push_back(curve.m_controlPoints.back());
      return;
    }

    BezierCurve left, right;
    subdivideCurve(curve, left, right);
    evaluateCurve(left, points);
    evaluateCurve(right,points);
  }


  void BezierCurve::subdivideCurve(const BezierCurve & curve, BezierCurve & left,
                                   BezierCurve & right, float t)
  {
    // De Casteljau's algorithm
    auto p0 = curve[0];
    auto p1 = curve[1];
    auto p2 = curve[2];
    auto p3 = curve[3];

    auto p11 = (1.f-t)*p0 + t*p1;
    auto p21 = (1.f-t)*p1 + t*p2;
    auto p31 = (1.f-t)*p2 + t*p3;
    auto p12 = (1.f-t)*p11 + t*p21;
    auto p22 = (1.f-t)*p21 + t*p31;
    auto p13 = (1.f-t)*p12 + t*p22;

    left.m_controlPoints = { p0, p11, p12, p13 };
    right.m_controlPoints = { p13, p22, p31, p3 };
  }

  bool BezierCurve::isFlat(const BezierCurve & curve, float tolerance)
  {
    return curveValue(curve) <= tolerance;
  }

  float BezierCurve::curveValue(const BezierCurve & curve)
  {
    // calculate the maximum difference between the middle control points and a straight
    // line between the end points.
    Nimble::Vector2f a = curve[3] - curve[0];
    Nimble::Vector2f an = a.perpendicular().normalized();
    Nimble::Vector2f b = curve[1] - curve[0];
    Nimble::Vector2f c = curve[2] - curve[0];
    float projB = std::abs(Nimble::dot(b, an));
    float projC = std::abs(Nimble::dot(c, an));
    return (std::max(projB, projC));
  }

  void BezierCurve::findIntersections(const BezierCurve & curve, const Nimble::Rectf & rect,
                                      std::vector<float> & intersections, float t, float tTolerance,
                                      float sizeTolerance, int depth)
  {
    if (!curve.bounds().intersects(rect)) {
      return;
    }
    if (rect.contains(curve.bounds())) {
      return;
    }
    if (curve.size() < sizeTolerance || std::pow(0.5f, depth) < tTolerance) {
      intersections.push_back(t);
      return;
    }

    BezierCurve left, right;
    subdivideCurve(curve, left, right, 0.5f);
    // track t at the middle point of the section
    depth++;
    findIntersections(left, rect, intersections, t - std::pow(0.5f, depth), tTolerance, sizeTolerance, depth);
    findIntersections(right, rect, intersections, t + std::pow(0.5f, depth), tTolerance, sizeTolerance, depth);
  }

  void BezierCurve::findIntersections(const BezierCurve & curve, const Nimble::Circle & circle,
                                      std::vector<float> & intersections, float t, float tTolerance,
                                      float sizeTolerance, int depth)
  {
    Nimble::Rectf curveBounds = curve.bounds();
    if (!circle.intersects(curveBounds)) {
      return;
    }
    if (circle.contains(curveBounds)) {
      return;
    }
    if (curve.size() < sizeTolerance || std::pow(0.5f, depth) < tTolerance) {
      intersections.push_back(t);
      return;
    }

    BezierCurve left, right;
    subdivideCurve(curve, left, right, 0.5f);
    // track t at the middle point of the section
    depth++;
    findIntersections(left, circle, intersections, t - std::pow(0.5f, depth), tTolerance, sizeTolerance, depth);
    findIntersections(right, circle, intersections, t + std::pow(0.5f, depth), tTolerance, sizeTolerance, depth);
  }

  ///////////////////////////////////////////////////////////////////////////////////////

  class SplineManager::D
  {
  public:
    D()
    {
      m_descr.addAttribute<Nimble::Vector2f>("vertex_position");
      m_descr.addAttribute<Nimble::Vector4f>("vertex_color");
      fillBuffer();
      m_vertexArray.addBinding(m_vertexBuffer, m_descr);
    }

    void clear();

    void fillBuffer();

    void recalculate();
    void recalculateAll();
    void bakeStroke(SplineInternal & stroke);
    void recalculate(SplineInternal & stroke);

    void updateBoundingBox();

    void render(Luminous::RenderContext & r) const;

    // these can be called without immediate recalculation to make processing
    // multiple strokes at a time faster, for example when deserializing or erasing
    // note: respective recalculations must be called eventually
    void endStroke(Valuable::Node::Uuid id, bool calculate = true);
    void addStroke(const SplineInfo & info, bool calculate = true);
    void removeStroke(Valuable::Node::Uuid id, bool calculate = true);

    void simplifyStroke(SplineInternal & stroke, float tolerance = 0.2f) const;

  public:
    std::vector<Vertex> m_vertices;
    QMap<Valuable::Node::Uuid, SplineInternal> m_strokes;
    QMap<float, Valuable::Node::Uuid> m_depthMap;

    Nimble::Rect m_bounds;

    Luminous::Buffer m_vertexBuffer;
    Luminous::VertexArray m_vertexArray;
    size_t m_finishedIndex = 0;

    Luminous::VertexDescription m_descr;
  };

  void SplineManager::D::clear()
  {
    m_vertices.clear();
    m_bounds = Nimble::Rectf();
    m_strokes.clear();
    m_finishedIndex = 0;
    fillBuffer();
  }

  void SplineManager::D::fillBuffer()
  {
    m_vertexBuffer.setData(m_vertices.data(), sizeof(Vertex) * m_vertices.size(),
                           Luminous::Buffer::DYNAMIC_DRAW);
  }

  void SplineManager::D::recalculate()
  {
    // truncate vertices from unfinished strokes and recalculate only those
    m_vertices.resize(m_finishedIndex);

    for (const auto & id : m_depthMap.values()) {
      auto & stroke = m_strokes[id];
      if (!stroke.m_finished)
        recalculate(stroke);
    }

    fillBuffer();
    updateBoundingBox();
  }

  void SplineManager::D::recalculateAll()
  {
    m_vertices.clear();

    std::vector<Valuable::Node::Uuid> unfinishedStrokes;

    // Calculate finished strokes first, so we can easily keep them while recalculating
    // strokes that are still edited
    for (const auto & id : m_depthMap.values()) {
      auto & stroke = m_strokes[id];
      if (!stroke.m_finished)
        unfinishedStrokes.push_back(id);
      else
        recalculate(stroke);
    }

    m_finishedIndex = m_vertices.size();

    for (const auto & id : unfinishedStrokes) {
      auto & stroke = m_strokes[id];
      recalculate(stroke);
    }

    fillBuffer();
    updateBoundingBox();
  }

  void SplineManager::D::bakeStroke(SplineInternal & stroke)
  {
    m_vertices.resize(m_finishedIndex);
    recalculate(stroke);
    m_finishedIndex = m_vertices.size();

    recalculate();
  }

  void SplineManager::D::recalculate(SplineInternal & stroke)
  {
    if(stroke.m_curves.size() < 1) {
      return;
    }

    // use cached stroke data if available
    if (stroke.m_finished && !stroke.m_vertices.empty()) {
      if(!m_vertices.empty()) {
        // degenerated triangle
        m_vertices.push_back(m_vertices.back());
        m_vertices.push_back(stroke.m_vertices.front());
      }
      m_vertices.insert(m_vertices.end(), stroke.m_vertices.begin(), stroke.m_vertices.end());
      return;
    }

    Points points;
    int offset = m_vertices.size();

    // first point
    points.push_back(stroke.m_curves[0].m_controlPoints[0]);

    for (size_t i = 0; i < stroke.m_curves.size(); i++) {
      BezierCurve::evaluateCurve(stroke.m_curves[i], points);
    }

    const int n = (int) points.size();

    Nimble::Vector2f cprev;
    Nimble::Vector2f cnow = points[0];
    Nimble::Vector2f cnext;
    Nimble::Vector2f avg;
    Nimble::Vector2f dirNext;
    Nimble::Vector2f dirPrev;

    cnext = points[1];
    dirNext = cnext - cnow;
    dirNext.normalize();
    avg = dirNext.perpendicular();

    if (avg.length() < 1e-5) {
      avg.make(1,0);
    } else {
      avg.normalize();
    }

    avg *= stroke.m_data.width * 0.5f;

    Vertex v;
    v.color = stroke.m_data.color.toVector();
    v.location = cnow - avg;

    if(!m_vertices.empty()) {
      // degenerated triangle
      m_vertices.push_back(m_vertices.back());
      m_vertices.push_back(v);
      offset += 2;
    }

    m_vertices.push_back(v);

    v.location = cnow + avg;
    m_vertices.push_back(v);

    for (int i = 1; i < n; ++i) {

      cprev = cnow;
      cnow = cnext;

      if (i+1 > n-1) {
        cnext = 2.0f*cnow - cprev;
      } else {
        cnext = points[i+1];
      }

      dirPrev = dirNext;
      dirNext = cnext - cnow;

      if (dirNext.length() < 1e-5f) {
        dirNext = dirPrev;
      } else {
        dirNext.normalize();
      }

      avg = (dirPrev + dirNext).perpendicular();
      avg.normalize();

      float dp = Nimble::Math::Clamp(dot(avg, dirPrev.perpendicular()), 0.7f, 1.0f);
      avg /= dp;
      avg *= stroke.m_data.width * 0.5f;

      v.location = cnow - avg;
      m_vertices.push_back(v);

      v.location = cnow + avg;
      m_vertices.push_back(v);
    }

    // cache stroke data for finished stroke
    if (stroke.m_finished) {
      stroke.m_vertices.assign(m_vertices.begin() + offset, m_vertices.end());
    }
  }

  void SplineManager::D::updateBoundingBox()
  {
    m_bounds = Nimble::Rect();
    for (const auto & vertex : m_vertices)
      m_bounds.expand(vertex.location);
  }

  void SplineManager::D::render(Luminous::RenderContext & r) const
  {
    // Nothing to render
    if(m_vertices.empty())
      return;

    auto b = r.render<Vertex, Luminous::BasicUniformBlock>(r.splineShader().translucent(),
                                                           Luminous::PRIMITIVE_TRIANGLE_STRIP, 0,
                                                           m_vertices.size(),
                                                           1.f, m_vertexArray, r.splineShader());

    /// @todo what color to use here?
    b.uniform->color = Nimble::Vector4f(1,1,1,r.opacity());
    b.uniform->depth = b.depth;

    // Fill the uniform data
    b.uniform->projMatrix = r.viewTransform().transposed();
    b.uniform->modelMatrix = r.transform().transposed();

#ifdef SPLINES_DEBUG
    Luminous::Style strokeStyle;
    strokeStyle.setStrokeColor(Radiant::Color(1.f,1.f,1.f));
    strokeStyle.setStrokeWidth(2.f);
    Luminous::Style pointStyle;
    pointStyle.setFillColor(Radiant::Color(0.f,0.f,1.f));
    Luminous::Style pointStyle2;
    pointStyle2.setFillColor(Radiant::Color(1.f,0.f,0.f));
    for (const auto & stroke : m_strokes) {
      r.drawRect(stroke.m_bounds, strokeStyle);
      for (auto curve : stroke.m_curves) {
        for (int i = 0; i <= stroke.m_pointsPerCurve; i++)
          r.drawCircle(curve[i], 3.f, i == 0 || i == stroke.m_pointsPerCurve ? pointStyle : pointStyle2);
      }
    }
    strokeStyle.setStrokeColor(Radiant::Color(.5f, .5f, 1.f, 1.f));
    r.drawRect(m_bounds, strokeStyle);
#endif
  }

  void SplineManager::D::endStroke(Valuable::Node::Uuid id, bool calculate)
  {
    if (m_strokes.contains(id)) {
      SplineInternal & stroke = m_strokes[id];
      stroke.m_finished = true;
      // scale tolerance depending on the size of the stroke
      float tolerance = 0.0005f * stroke.m_bounds.size().toVector().length();
      simplifyStroke(stroke, tolerance);
      if (calculate)
        bakeStroke(stroke);
    }
  }

  void SplineManager::D::addStroke(const SplineInfo & info, bool calculate)
  {
    SplineInternal newStroke;
    newStroke.m_data = info.data;
    newStroke.processPoints();
    m_strokes[info.id] = newStroke;
    m_depthMap.insertMulti(info.data.depth, info.id);
    endStroke(info.id, calculate);
  }

  void SplineManager::D::removeStroke(Valuable::Node::Uuid id, bool calculate)
  {
    if(m_strokes.remove(id)) {
      for (auto it = m_depthMap.begin(); it != m_depthMap.end();) {
        if (it.value() == id) {
          it = m_depthMap.erase(it);
        } else {
          ++it;
        }
      }
      if(calculate)
        recalculateAll();
    }
  }

  void SplineManager::D::simplifyStroke(SplineInternal & stroke, float tolerance) const
  {
    Points original = stroke.m_data.points;
    Points newPoints;
    int n = stroke.m_pointsPerCurve;

    // just one curve, nothing to do here
    if (original.size() < n)
      return;

    int i = 0;
    // always include first 2 points
    newPoints.append(original.at(i));
    newPoints.append(original.at(i + 1));
    Point prev = original.at(i);

    i += n; // i should always point to an end point
    float error = 0;

    while (i + n < original.size()) {
      // try to skip some points
      Point cur = original.at(i);
      Point next = original.at(i + n);

      BezierCurve curve;
      curve.m_controlPoints = { prev, cur, cur, next };
      float diff = BezierCurve::curveValue(curve);
      if (error + diff < tolerance) {
        // skip this one and accumulate some error
        error += diff * 0.5f;
      }
      else {
        newPoints.append(original.at(i - 1));
        newPoints.append(original.at(i));
        newPoints.append(original.at(i + 1));
        prev = original.at(i);
        error = 0;
      }
      i += n;
    }

    // always include last 2 points
    newPoints.append(original.at(i - 1));
    newPoints.append(original.at(i));

    stroke.m_data.points = newPoints;
    stroke.processPoints();
  }

  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////


  SplineManager::SplineManager()
    : m_d(new D)
  {
  }

  SplineManager::~SplineManager()
  {
  }

  SplineManager::SplineManager(const SplineManager & other)
  {
    m_d = std::unique_ptr<D>(new D);
    m_d->m_strokes = other.m_d->m_strokes;
    m_d->m_bounds = other.m_d->m_bounds;
    m_d->m_finishedIndex = other.m_d->m_finishedIndex;
    m_d->m_vertices = other.m_d->m_vertices;
    m_d->fillBuffer();
  }

  SplineManager & SplineManager::operator=(const SplineManager & other)
  {
    m_d = std::unique_ptr<D>(new D);
    m_d->m_strokes = other.m_d->m_strokes;
    m_d->m_bounds = other.m_d->m_bounds;
    m_d->m_finishedIndex = other.m_d->m_finishedIndex;
    m_d->m_vertices = other.m_d->m_vertices;
    m_d->fillBuffer();
    return *this;
  }

  SplineManager::SplineManager(SplineManager&& other)
    : m_d(std::move(other.m_d))
  {
  }

  SplineManager & SplineManager::operator=(SplineManager && strokes)
  {
    m_d = std::move(strokes.m_d);
    return *this;
  }

  Nimble::Rect SplineManager::boundingBox() const
  {
    return m_d->m_bounds;
  }

  bool SplineManager::erase(const Nimble::Rectangle & eraser,
                     Splines * removedStrokes, Splines * addedStrokes,
                     QString * errorText)
  {
    // test for an early exit
    if (!boundingBox().intersects(eraser.boundingBox()))
      return true;

    bool recalculate = false;
    Splines newStrokes;

    // transformation to coordinates where the eraser is at (-1,-1) to (1,1) for easier calculations
    std::array<Nimble::Vector2f, 4> corners = eraser.computeCorners();
    std::array<Nimble::Vector2f, 4> normalized = {{
      Nimble::Vector2(-1.f, -1.f),
      Nimble::Vector2(1.f, -1.f),
      Nimble::Vector2(1.f, 1.f),
      Nimble::Vector2(-1.f, 1.f)
    }};
    bool ok;
    Nimble::Matrix3f transformer = Nimble::Matrix3f::mapCorrespondingPoints(corners, normalized, &ok);
    if (!ok) {
      if (errorText)
        *errorText = QString("Failed to process eraser area");
      return false;
    }

    for (auto id : m_d->m_strokes.keys()) {
      SplineInternal & stroke = m_d->m_strokes[id];
      if (stroke.m_bounds.intersects(eraser.boundingBox())) {
        // remove or split strokes where erased
        if (eraser.isInside(Nimble::Rectangle(stroke.m_bounds)) ||
            stroke.erase(Nimble::Rectf(normalized[0], normalized[2]),
                         transformer, newStrokes)) {
          if (removedStrokes) {
            removedStrokes->append(createInfo(id, stroke.m_data));
          }
          m_d->removeStroke(id, false);
          recalculate = true;
        }
      }
    }

    // add the new split strokes
    for (auto newStroke : newStrokes) {
      m_d->addStroke(newStroke, false);
    }
    if (addedStrokes)
      addedStrokes->append(newStrokes);

    // if anything was erased, need to recalculate all vertices
    if (recalculate)
      m_d->recalculateAll();
    else if (newStrokes.size() > 0)
      m_d->recalculate();
    return true;
  }

  bool SplineManager::erase(const Nimble::Circle & eraser,
                            Splines * removedStrokes, Splines * addedStrokes,
                            QString * errorText)
  {
    // test for an early exit
    Nimble::Rect eraserBounds = eraser.boundingBox();
    if (!boundingBox().intersects(eraserBounds))
      return true;

    bool recalculate = false;
    Splines newStrokes;

    for (auto id : m_d->m_strokes.keys()) {
      SplineInternal & stroke = m_d->m_strokes[id];
      if (stroke.m_bounds.intersects(eraserBounds)) {
        // remove or split strokes where erased
        if (Nimble::Rectangle(eraserBounds).isInside(Nimble::Rectangle(stroke.m_bounds)) ||
            stroke.erase(eraser, newStrokes)) {
          if (removedStrokes) {
            removedStrokes->append(createInfo(id, stroke.m_data));
          }
          m_d->removeStroke(id, false);
          recalculate = true;
        }
      }
    }

    // add the new split strokes
    for (auto newStroke : newStrokes) {
      m_d->addStroke(newStroke, false);
    }
    if (addedStrokes)
      addedStrokes->append(newStrokes);

    // if anything was erased, need to recalculate all vertices
    if (recalculate)
      m_d->recalculateAll();
    else if (newStrokes.size() > 0)
      m_d->recalculate();
    return true;
  }

  Valuable::Node::Uuid SplineManager::beginSpline(Point p, float splineWidth,
                                                  Radiant::ColorPMA c, float depth)
  {
    SplineData data{ splineWidth, c, depth, { p } };
    return beginSpline(data);
  }

  Valuable::Node::Uuid SplineManager::beginSpline(const SplineData & data)
  {
    Valuable::Node::Uuid id = Valuable::Node::generateId();
    SplineInternal newStroke;

    newStroke.m_data = data;
    newStroke.processPoints();

    m_d->m_strokes[id] = newStroke;
    m_d->m_depthMap.insertMulti(data.depth, id);
    m_d->recalculate();
    return id;
  }

  void SplineManager::continueSpline(Valuable::Node::Uuid id, const Point & point, float minimumDistance)
  {
    if (m_d->m_strokes.contains(id)) {
      SplineInternal & stroke = m_d->m_strokes[id];
      stroke.addPoint(point, minimumDistance);
      m_d->recalculate();
    }
  }

  void SplineManager::endSpline(Valuable::Node::Uuid id)
  {
    m_d->endStroke(id);
  }

  Valuable::Node::Uuid SplineManager::addSpline(const SplineData & data)
  {
    auto id = Valuable::Node::generateId();
    addSpline(createInfo(id, data));
    return id;
  }

  void SplineManager::addSpline(const SplineInfo & info)
  {
    m_d->addStroke(info);
  }

  void SplineManager::removeSpline(Valuable::Node::Uuid id)
  {
    m_d->removeStroke(id);
  }

  void SplineManager::removeSplines(Splines splines)
  {
    if (splines.isEmpty())
      return;
    for (const auto & stroke : splines)
      m_d->removeStroke(stroke.id, false);
    m_d->recalculateAll();
  }

  SplineManager::SplineData SplineManager::spline(Valuable::Node::Uuid id) const
  {
    if (m_d->m_strokes.contains(id))
      return m_d->m_strokes.value(id).m_data;
    return SplineData();
  }

  SplineManager::Splines SplineManager::allSplines() const
  {
    Splines strokes;
    for (auto it = m_d->m_strokes.begin(); it != m_d->m_strokes.end(); it++) {
      strokes.push_back(createInfo(it.key(), it.value().m_data));
    }
    return strokes;
  }

  void SplineManager::render(Luminous::RenderContext & r) const
  {
    m_d->render(r);
  }

  QString SplineManager::serialize() const
  {
    QString str = QString("%1\n").arg(m_d->m_strokes.size());
    for(auto id : m_d->m_strokes.keys()) {
      str.append(serializeSpline(createInfo(id, m_d->m_strokes[id].m_data)));
    }
    return str;
  }

  void SplineManager::deserialize(const QString &str)
  {
    clear();

    QStringList lines = str.split("\n");
    if (lines.isEmpty())
      return;

    int line = 0;
    int paths = lines[line++].toInt();

    for(int i = 0; i < paths; ++i) {
      if (line >= lines.size()) {
        Radiant::warning("Spline::deserialize # was expecting more data, some strokes may be missing!");
        break;
      }
      int count = lines[line++].toInt();
      auto strokeLines = lines.mid(line, count);
      // @todo: remove the depth correcting when depth is handled in migration
      SplineInfo info = deserializeSpline(strokeLines, currentDepth() + 0.1f);

      if (!info.data.points.isEmpty())
        m_d->addStroke(info, false);

      line += count;
    }
    m_d->recalculateAll();
  }

  void SplineManager::clear()
  {
    if(m_d)
      m_d->clear();
  }

  bool SplineManager::isEmpty() const
  {
    return m_d->m_vertices.size() == 0;
  }

  float SplineManager::currentDepth() const
  {
    if (m_d->m_depthMap.isEmpty())
      return 0.f;
    auto it = m_d->m_depthMap.end();
    it--;
    return it.key();
  }

  QString SplineManager::serializeSpline(const SplineInfo & stroke)
  {
    const SplineData & data = stroke.data;
    auto id = stroke.id;
    const SplineManager::Points & points = data.points;
    QString str;
    // stroke color, width, id and depth
    str.append(QString::number(data.color.red()));   str.append(' ');
    str.append(QString::number(data.color.green())); str.append(' ');
    str.append(QString::number(data.color.blue()));  str.append(' ');
    str.append(QString::number(data.color.alpha())); str.append(' ');
    str.append(QString::number(data.width));         str.append(' ');
    str.append(QString::number((qlonglong) id));     str.append(' ');
    str.append(QString::number(data.depth));         str.append("\n");

    // stroke points
    str.append(QString::number(int(points.size()))); str.append("\n");
    for(const Point& p : points) {
      str.append(QString::number(p.x));    str.append(' ');
      str.append(QString::number(p.y));    str.append('\n');
    }
    int linecount = int(points.size()) + 2;
    str.prepend(QString::number(linecount) + "\n");
    return str;
  }

  SplineManager::SplineInfo SplineManager::deserializeSpline(const QStringList & lines, float defaultDepth)
  {
    if (lines.isEmpty())
      return SplineManager::SplineInfo();

    SplineManager::SplineData data;
    int line = 0;

    // stroke color, width, id and depth
    QStringList strokeParams = lines[line++].split(" ");
    // 4 color values, 1 width value, 1 id value and 1 depth value, depth value may be missing
    // @todo: should just migrate the data so there's always depth value
    if(strokeParams.size() < 6) {
      Radiant::warning("Spline::dezerializeStroke # Failed to deserialize stroke, wrong number of parameters.");
      return SplineManager::SplineInfo();
    }

    Radiant::Color color = Radiant::Color(strokeParams[0].toFloat(), strokeParams[1].toFloat(),
        strokeParams[2].toFloat(), strokeParams[3].toFloat());
    float strokeWidth = strokeParams[4].toFloat();
    Valuable::Node::Uuid strokeId = strokeParams[5].toLongLong();
    if (strokeParams.size() > 6)
      data.depth = strokeParams[6].toFloat();
    else
      data.depth = defaultDepth;

    data.color = color;
    data.width = strokeWidth;

    // stroke points
    int points = lines[line++].toInt();
    points = std::min(points, lines.size() - line);
    if (line + points > lines.count()) {
      Radiant::warning("Spline::dezerializeStroke # Failed to deserialize stroke, not enough data for points.");
      return SplineManager::SplineInfo();
    }

    for(int j = 0; j < points; ++j) {
      QStringList numbers = lines[line++].split(" ");

      // Points have x and y values
      if(numbers.size() != 2)
        continue;

      SplineManager::Point p;
      p = Nimble::Vector2f(numbers[0].toFloat(), numbers[1].toFloat());

      data.points.push_back(p);
    }
    return createInfo(strokeId, data);
  }

}
