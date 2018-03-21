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

  template <typename T>
  Luminous::SplineManager::SplineInfo createInfo(Valuable::Node::Uuid id,
                                                 T && data)
  {
    Luminous::SplineManager::SplineInfo info;
    info.id = id;
    info.data = std::forward<T>(data);
    return info;
  }


  //////////////////////////////////////////////////////////////////

  class SplineInternal
  {
  public:
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

    /// True if this spline has been succesfully rendered to SplineManager::D::m_vertices
    bool m_baked = false;
    /// Vertex index to SplineManager::D::m_vertices
    int m_bakedIndex = 0;
    /// Past-the-end vertex index to SplineManager::D::m_vertices
    int m_bakedIndexEnd = 0;

    /// Iterator to SplineManager::D::m_depthIndex for improving index removal time
    QMap<float, SplineInternal*>::iterator m_depthIndexIterator;
  };

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
        curve.set(m_pointsPerCurve - i, m_data.points[index - i]);
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
      for (size_t j = 0; j < curve.count(); j++) {
        curve.set(j, transformer.project(curve[j]));
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
              for(size_t ind = 0; ind < left.count(); ++ind)
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
                saveSegment(-1, -1, newStrokes, left.points());
              }
              take = !take;
            }

            // add last segment to next curve if end point is not erased
            if (!(std::abs(curve[m_pointsPerCurve].x) <= 1 && std::abs(curve[m_pointsPerCurve].y) <= 1))
              for(size_t ind = 0; ind < right.count(); ++ind)
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
              for(size_t ind = 0; ind < left.count(); ++ind)
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
                saveSegment(-1, -1, newStrokes, left.points());
              }
              take = !take;
            }

            // add last segment to next curve if end point is not erased
            if (!eraser.contains(Nimble::Vector2f(curve[m_pointsPerCurve].x,curve[m_pointsPerCurve].y)))
              for(size_t ind = 0; ind < right.count(); ++ind)
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

  BezierCurve::BezierCurve()
  {
  }

  BezierCurve::BezierCurve(const std::array<Nimble::Vector2f, 4> & points)
  {
    setPoints(points);
  }

  Nimble::Vector2f BezierCurve::operator[](int pos) const
  {
    return m_controlPoints.at(pos);
  }

  void BezierCurve::set(int pos, const Nimble::Vector2f & point)
  {
    assert(point.isFinite());
    if (point.isFinite())
      m_controlPoints[pos] = point;
    else
      Radiant::error("BezierCurve::set # Control point must be finite!");
  }

  void BezierCurve::setPoints(const std::array<Nimble::Vector2f,4> & points)
  {
    for (auto i = 0; i < 4; i++)
      set(i, points[i]);
  }

  size_t BezierCurve::count() const
  {
    return m_controlPoints.size();
  }

  const std::array<Nimble::Vector2f, 4> & BezierCurve::points() const
  {
    return m_controlPoints;
  }

  void BezierCurve::setEndPoints(const Nimble::Vector2f &start, const Nimble::Vector2f &end)
  {
    set(0, start);
    set(3, end);
    Nimble::Vector2f p1, p2;
    p1 = m_controlPoints[0] + 1.f / 3.f * (m_controlPoints[3] - m_controlPoints[0]);
    p2 = 0.5f * (m_controlPoints[3] + p1);
    set(1, p1);
    set(2, p2);
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

    left.setPoints({{ p0, p11, p12, p13 }});
    right.setPoints({{ p13, p22, p31, p3 }});
  }

  Nimble::Vector2f BezierCurve::derivate(float t) const
  {
    float tm = 1-t;
    auto p0 = m_controlPoints[0];
    auto p1 = m_controlPoints[1];
    auto p2 = m_controlPoints[2];
    auto p3 = m_controlPoints[3];

    return 3*tm*tm*(p1 - p0) + 6*tm*t*(p2 - p1) + 3*t*t*(p3 - p2);
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

  /// For effiency reasons we keep all generated vertices in one big vector
  /// (SplineManager::D::m_vertices) where finished splines are in the beginning
  /// (in depth order) and then all unfinished splines at the end. This way we
  /// don't need to recreate and reupload the whole vector if we just add one
  /// vertex to the end.
  ///
  /// However, it's also possible to add new finished or unfinished splines
  /// between existing splines by giving arbitrary depth value. Since we need
  /// to render the vertices in correct order, we do the final rendering in
  /// batches. One batch is continuous section in m_vertices that can be
  /// rendered with a single render command and has all the items in order.
  ///
  /// An example:
  /// There are two existing splines A (depth 0) and B (depth 2), and we are
  /// adding a new spline C (depth 1). m_vertices looks something like this:
  /// AAAAAAAAAAAAAAAAAAAAAAAABBBBBBBBBBBBBBBBBBBBBBBBCCCCCCCCCCCCCCCCCCCCC
  /// <------------------ finished ------------------><--- unfinished ---->
  /// <---- render pass 0 ---><---- render pass 2 ---><-- render pass 1 -->
  ///
  /// Since the new spline need to be rendered between A and B, we need to have
  /// three render batches to render the unfinished spline C before spline B.
  struct RenderBatch
  {
    int offset = -1;
    int vertexCount = 0;
    bool finished = false;
  };

  class SplineManager::D
  {
  public:
    D()
    {
      m_descr.addAttribute<Nimble::Vector2f>("vertex_position");
      m_descr.addAttribute<Nimble::Vector4f>("vertex_color");
      m_vertexArray.addBinding(m_vertexBuffer, m_descr);
    }

    void clear();

    /// @param vertexOffset first vertex that has changed
    void fillBuffer(size_t vertexOffset);

    void recalculate();
    void recalculate(SplineInternal & stroke);

    void render(Luminous::RenderContext & r) const;

    void endStroke(Valuable::Node::Uuid id, bool simplify = false);
    void endStroke(SplineInternal & stroke, bool simplify);
    void addStroke(const SplineInfo & info, bool simplify = false);
    void removeStroke(Valuable::Node::Uuid id);

    void simplifyStroke(SplineInternal & stroke, float tolerance = 0.2f) const;

  public:
    std::vector<Vertex> m_vertices;
    std::vector<RenderBatch> m_renderBatches;
    QMap<Valuable::Node::Uuid, SplineInternal> m_strokes;
    // Index m_strokes by depth
    QMap<float, SplineInternal*> m_depthIndex;

    Nimble::Rect m_bounds;

    Luminous::Buffer m_vertexBuffer;
    Luminous::VertexArray m_vertexArray;

    bool m_hasTranslucentVertices = false;

    Luminous::VertexDescription m_descr;

    bool m_dirty = true;
  };

  void SplineManager::D::clear()
  {
    m_vertices.clear();
    m_bounds = Nimble::Rectf();
    m_strokes.clear();
    m_depthIndex.clear();
    m_renderBatches.clear();
    m_hasTranslucentVertices = false;
  }

  void SplineManager::D::fillBuffer(size_t vertexOffset)
  {
    const size_t bytesNeeded = sizeof(Vertex) * m_vertices.size();
    /// Allocate slightly larger buffer than we need so that we don't need
    /// to allocate a new buffer every time a small change is made to the
    /// spline.
    const size_t maxUsedBytes = bytesNeeded + 16*1024;

    /// If possible, only upload changed part of m_vertices
    if (m_vertexBuffer.data() == m_vertices.data()) {
      if (m_vertexBuffer.bufferSize() >= bytesNeeded && m_vertexBuffer.bufferSize() <= maxUsedBytes) {
        size_t offsetBytes = sizeof(Vertex) * vertexOffset;
        m_vertexBuffer.invalidateRegion(offsetBytes, bytesNeeded - offsetBytes);
        return;
      }
    }

    m_vertexBuffer.setData(m_vertices.data(), bytesNeeded,
                           Luminous::Buffer::DYNAMIC_DRAW, maxUsedBytes);
  }

  void SplineManager::D::recalculate()
  {
    /// Stroke, render batch index
    std::vector<std::pair<SplineInternal *, int>> unfinishedStrokes;

    m_renderBatches.clear();
    m_hasTranslucentVertices = false;
    m_bounds = Nimble::Rect();
    float maxWidth = 0;

    bool useCachedValues = true;
    int invalidateOffset = 0;
    int index = 0;

    for (SplineInternal * strokePtr: m_depthIndex) {
      SplineInternal & stroke = *strokePtr;
      if (stroke.m_curves.empty())
        continue;

      m_hasTranslucentVertices |= stroke.m_data.color.a < 0.999f;

      /// Calculate finished strokes first, so we can easily keep them while
      /// recalculating strokes that are still edited
      if (stroke.m_finished) {

        /// This stroke has already been baked
        if (useCachedValues && stroke.m_baked && stroke.m_bakedIndex == index) {
          if (m_renderBatches.empty() || !m_renderBatches.back().finished) {
            m_renderBatches.emplace_back();
            m_renderBatches.back().finished = true;
            m_renderBatches.back().offset = index;
          }
          m_renderBatches.back().vertexCount += stroke.m_bakedIndexEnd - stroke.m_bakedIndex;
          index = stroke.m_bakedIndexEnd;
          m_bounds.expand(stroke.m_bounds);
          maxWidth = std::max(maxWidth, stroke.m_data.width);
          continue;
        }

        if (useCachedValues) {
          useCachedValues = false;
          invalidateOffset = index;
          m_vertices.resize(index);
        }

        int offset = m_vertices.size();
        recalculate(stroke);
        if (m_renderBatches.empty() || !m_renderBatches.back().finished) {
          m_renderBatches.emplace_back();
          m_renderBatches.back().finished = true;
          m_renderBatches.back().offset = offset;
        }
        m_renderBatches.back().vertexCount += m_vertices.size() - offset;
        m_bounds.expand(stroke.m_bounds);
        maxWidth = std::max(maxWidth, stroke.m_data.width);
      } else {
        if (m_renderBatches.empty() || m_renderBatches.back().finished)
          m_renderBatches.emplace_back();
        unfinishedStrokes.push_back({strokePtr, m_renderBatches.size() - 1});
      }
    }

    if (useCachedValues) {
      invalidateOffset = index;
      m_vertices.resize(index);
    }

    for (const auto p: unfinishedStrokes) {
      SplineInternal & stroke = *p.first;
      RenderBatch & rb = m_renderBatches[p.second];

      int offset = m_vertices.size();
      recalculate(stroke);
      if (rb.offset == -1)
        rb.offset = offset;
      rb.vertexCount += m_vertices.size() - offset;
      m_bounds.expand(stroke.m_bounds);
      maxWidth = std::max(maxWidth, stroke.m_data.width);
    }

    /// Take the spline width into account, since the bounding box is from the
    /// control points and not from the actual spline outline. In the worst case
    /// this might be slightly pessimistic and create too large m_bounds, since
    /// maybe the splines at the edges are not so wide as some splines in the
    /// center.
    m_bounds.grow(maxWidth * 0.5f);

    fillBuffer(invalidateOffset);
    m_dirty = false;
  }

  void SplineManager::D::recalculate(SplineInternal & stroke)
  {
    stroke.m_bakedIndex = m_vertices.size();

    // use cached stroke data if available
    if (stroke.m_finished && !stroke.m_vertices.empty()) {
      if(!m_vertices.empty()) {
        // degenerated triangle
        m_vertices.push_back(m_vertices.back());
        m_vertices.push_back(stroke.m_vertices.front());
      }
      m_vertices.insert(m_vertices.end(), stroke.m_vertices.begin(), stroke.m_vertices.end());
      stroke.m_baked = true;
      stroke.m_bakedIndexEnd = m_vertices.size();
      return;
    }

    Points points;
    int offset = m_vertices.size();

    // first point
    points.push_back(stroke.m_curves[0][0]);

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
      stroke.m_baked = true;
      stroke.m_bakedIndexEnd = m_vertices.size();
    } else {
      stroke.m_baked = false;
    }
  }

  void SplineManager::D::render(Luminous::RenderContext & r) const
  {
    for (auto rb: m_renderBatches) {
      auto b = r.render<Vertex, Luminous::BasicUniformBlock>(m_hasTranslucentVertices || r.opacity() < 0.9999f,
                                                             Luminous::PRIMITIVE_TRIANGLE_STRIP, rb.offset,
                                                             rb.vertexCount,
                                                             1.f, m_vertexArray, r.splineShader());

      /// @todo what color to use here?
      b.uniform->color = Nimble::Vector4f(1,1,1,r.opacity());
      b.uniform->depth = b.depth;

      // Fill the uniform data
      b.uniform->projMatrix = r.viewTransform().transposed();
      b.uniform->modelMatrix = r.transform().transposed();
    }

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

  void SplineManager::D::endStroke(Valuable::Node::Uuid id, bool simplify)
  {
    auto it = m_strokes.find(id);
    if (it != m_strokes.end())
      endStroke(it.value(), simplify);
  }

  void SplineManager::D::endStroke(SplineInternal & stroke, bool simplify)
  {
    stroke.m_finished = true;
    if (simplify) {
      // scale tolerance depending on the size of the stroke
      float tolerance = 0.0005f * stroke.m_bounds.size().toVector().length();
      simplifyStroke(stroke, tolerance);
    }
    m_dirty = true;
  }

  void SplineManager::D::addStroke(const SplineInfo & info, bool simplify)
  {
    SplineInternal & newStroke = m_strokes[info.id];
    /// If we are replacing and not adding, make sure the old entry from depth index is removed
    if (newStroke.m_depthIndexIterator != decltype(newStroke.m_depthIndexIterator)())
      m_depthIndex.erase(newStroke.m_depthIndexIterator);
    newStroke.m_data = info.data;
    newStroke.processPoints();
    newStroke.m_depthIndexIterator = m_depthIndex.insertMulti(info.data.depth, &newStroke);
    assert(m_strokes.size() == m_depthIndex.size());
    endStroke(newStroke, simplify);
  }

  void SplineManager::D::removeStroke(Valuable::Node::Uuid id)
  {
    auto it = m_strokes.find(id);
    if (it != m_strokes.end()) {
      m_depthIndex.erase(it->m_depthIndexIterator);
      m_strokes.erase(it);
      assert(m_strokes.size() == m_depthIndex.size());
      m_dirty = true;
    }
  }

  void SplineManager::D::simplifyStroke(SplineInternal & stroke, float tolerance) const
  {
    int n = stroke.m_pointsPerCurve;
    // just one curve, nothing to do here
    if (stroke.m_data.points.size() <= n)
      return;

    Points newPoints;
    Points original = std::move(stroke.m_data.points);

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
      curve.setPoints({{ prev, cur, cur, next }});
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

    stroke.m_data.points = std::move(newPoints);
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
    for (SplineInternal & spline: m_d->m_strokes)
      spline.m_depthIndexIterator = m_d->m_depthIndex.insertMulti(
            spline.m_depthIndexIterator.key(), &spline);
    m_d->m_bounds = other.m_d->m_bounds;
    m_d->m_vertices = other.m_d->m_vertices;
    m_d->m_renderBatches = other.m_d->m_renderBatches;
    m_d->m_hasTranslucentVertices = other.m_d->m_hasTranslucentVertices;

    m_d->fillBuffer(0);
  }

  SplineManager & SplineManager::operator=(const SplineManager & other)
  {
    m_d = std::unique_ptr<D>(new D);
    m_d->m_strokes = other.m_d->m_strokes;
    for (SplineInternal & spline: m_d->m_strokes)
      spline.m_depthIndexIterator = m_d->m_depthIndex.insertMulti(
            spline.m_depthIndexIterator.key(), &spline);
    m_d->m_bounds = other.m_d->m_bounds;
    m_d->m_vertices = other.m_d->m_vertices;
    m_d->m_renderBatches = other.m_d->m_renderBatches;
    m_d->m_hasTranslucentVertices = other.m_d->m_hasTranslucentVertices;
    m_d->fillBuffer(0);
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

    for (auto it = m_d->m_strokes.begin(); it != m_d->m_strokes.end();) {
      SplineInternal & stroke = it.value();
      if (stroke.m_bounds.intersects(eraser.boundingBox())) {
        // remove or split strokes where erased
        if (eraser.isInside(Nimble::Rectangle(stroke.m_bounds)) ||
            stroke.erase(Nimble::Rectf(normalized[0], normalized[2]),
                         transformer, newStrokes)) {
          if (removedStrokes) {
            removedStrokes->append(createInfo(it.key(), std::move(stroke.m_data)));
          }

          m_d->m_depthIndex.erase(stroke.m_depthIndexIterator);
          recalculate = true;
          it = m_d->m_strokes.erase(it);
          assert(m_d->m_depthIndex.size() == m_d->m_strokes.size());
          continue;
        }
      }
      ++it;
    }

    // add the new split strokes
    for (auto newStroke : newStrokes) {
      m_d->addStroke(newStroke);
    }
    if (addedStrokes)
      addedStrokes->append(newStrokes);

    // if anything was changed, need to recalculate m_vertices
    if (recalculate || newStrokes.size() > 0)
      m_d->m_dirty = true;
    return true;
  }

  bool SplineManager::erase(const Nimble::Circle & eraser,
                            Splines * removedStrokes, Splines * addedStrokes,
                            QString * /*errorText*/)
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
            removedStrokes->append(createInfo(id, std::move(stroke.m_data)));
          }
          m_d->removeStroke(id);
          recalculate = true;
        }
      }
    }

    // add the new split strokes
    for (auto newStroke : newStrokes) {
      m_d->addStroke(newStroke);
    }
    if (addedStrokes)
      addedStrokes->append(newStrokes);

    // if anything was changed, need to recalculate m_vertices
    if (recalculate || newStrokes.size() > 0)
      m_d->m_dirty = true;
    return true;
  }

  Valuable::Node::Uuid SplineManager::beginSpline(Point p, float splineWidth,
                                                  Radiant::ColorPMA c, float depth)
  {
    SplineData data{ splineWidth, c, depth, { p } };
    return beginSpline(data);
  }

  Valuable::Node::Uuid SplineManager::beginSpline(const SplineData & data, Valuable::Node::Uuid id)
  {
    if (id < 0)
      id = Valuable::Node::generateId();
    SplineInternal & newStroke = m_d->m_strokes[id];

    newStroke.m_data = data;
    newStroke.processPoints();

    newStroke.m_depthIndexIterator = m_d->m_depthIndex.insertMulti(data.depth, &newStroke);
    assert(m_d->m_depthIndex.size() == m_d->m_strokes.size());
    m_d->m_dirty = true;
    return id;
  }

  void SplineManager::continueSpline(Valuable::Node::Uuid id, const Point & point, float minimumDistance)
  {
    auto it = m_d->m_strokes.find(id);
    if (it != m_d->m_strokes.end()) {
      SplineInternal & stroke = it.value();
      stroke.addPoint(point, minimumDistance);
      m_d->m_dirty = true;
    }
  }

  void SplineManager::endSpline(Valuable::Node::Uuid id)
  {
    m_d->endStroke(id, true);
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

  void SplineManager::addSplines(const Splines & splines)
  {
    if (splines.isEmpty())
      return;
    for (const auto & info : splines)
      m_d->addStroke(info);
    m_d->m_dirty = true;
  }

  void SplineManager::removeSpline(Valuable::Node::Uuid id)
  {
    m_d->removeStroke(id);
  }

  void SplineManager::removeSplines(const Splines & splines)
  {
    if (splines.isEmpty())
      return;
    for (const auto & stroke : splines)
      m_d->removeStroke(stroke.id);
    m_d->m_dirty = true;
  }

  void SplineManager::addAndRemoveSplines(const SplineManager::Splines & addedSplines,
                                          const SplineManager::Splines & removedSplines)
  {
    for (const auto & info : addedSplines)
      m_d->addStroke(info);
    for (const auto & info : removedSplines)
      m_d->removeStroke(info.id);
    m_d->m_dirty = true;
  }

  void SplineManager::
  addAndRemoveSplines(const SplineManager::Splines & addedSplines,
                      const std::vector<Valuable::Node::Uuid> & removedSplines)
  {
    for (const auto & info : addedSplines)
      m_d->addStroke(info);
    for (const auto & id : removedSplines)
      m_d->removeStroke(id);
    m_d->m_dirty = true;
  }

  SplineManager::SplineData SplineManager::spline(Valuable::Node::Uuid id) const
  {
    auto it = m_d->m_strokes.find(id);
    if (it != m_d->m_strokes.end())
      return it.value().m_data;
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

  void SplineManager::update()
  {
    if (m_d->m_dirty)
      m_d->recalculate();
  }

  void SplineManager::render(Luminous::RenderContext & r) const
  {
    m_d->render(r);
  }

  QString SplineManager::serialize() const
  {
    QString str = QString("%1\n").arg(m_d->m_strokes.size());
    for (auto it = m_d->m_strokes.begin(); it != m_d->m_strokes.end(); it++) {
      str.append(serializeSpline(createInfo(it.key(), it.value().m_data)));
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
        m_d->addStroke(info);

      line += count;
    }
    m_d->m_dirty = true;
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
    if (m_d->m_depthIndex.isEmpty())
      return 0.f;
    auto it = m_d->m_depthIndex.end();
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
    return createInfo(strokeId, std::move(data));
  }

}
