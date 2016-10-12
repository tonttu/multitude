#pragma once

#include "Export.hpp"

#include <Nimble/Rectangle.hpp>
#include <Nimble/Circle.hpp>

#include <Valuable/Node.hpp>

namespace Luminous {
  class RenderContext;

  /// Class for creating, managing and rendering a set of splines on a single 2D surface.
  /// SplineManager contains multiple splines that each has its own width, color
  /// and relative order (depth)
  class LUMINOUS_API SplineManager
  {
  public:
    typedef Nimble::Vector2f Point;
    typedef QList<Point> Points;

    /// Raw representation of the spline. The points are interpreted according the
    /// internal logic of the class.
    struct SplineData
    {
      float width;
      Radiant::ColorPMA color;
      float depth;
      Points points;
    };

    struct SplineInfo
    {
      Valuable::Node::Uuid id;
      SplineData data;
    };

    typedef QList<SplineInfo> Splines;

    /// Constructor
    SplineManager();
    /// Destructor
    virtual ~SplineManager();

    /// Copy constructor
    SplineManager(const SplineManager & other);
    /// Copy assignment operator
    SplineManager & operator=(const SplineManager & other);
    /// Move constructor
    SplineManager(SplineManager && other);
    /// Move assignment operator
    SplineManager & operator=(SplineManager && other);

    /// Bounding box of all contained splines
    /// @return bounding box as a rect
    Nimble::Rect boundingBox() const;

    /// Erase splines inside area. Hit splines are removed and replaced with new subsplines
    /// for any remaining parts outside the eraser. Supply removedSplines and addedSplines
    /// if the data should be saved for later use (for example to restore previous state)
    /// @param eraser the area where splines should be erased
    /// @param removedSplines list of splines that were removed
    /// @param addedSplines list of splines that were added in case some of the splines
    ///                     needed to be splitted
    /// @param errorText text describing an error in erasing (if any)
    /// @return false if there was an error in erasing, otherwise true
    bool erase(const Nimble::Rectangle & eraser,
               Splines * removedSplines = nullptr, Splines * addedSplines = nullptr,
               QString * errorText = nullptr);

    /// Similar to the previous method but with a circular area to
    /// erase, specified by center and radius.
    /// @param eraser the area where spines should be erased
    /// @param removedSplines list of splines that were removed
    /// @param addedSplines list of splines that were added
    /// @param errorText text describing an error in erasing (if any)
    /// @return false if there was an error in erasing, otherwise true
    bool erase(const Nimble::Circle & eraser,
               Splines * removedSplines, Splines * addedSplines,
               QString * errorText = nullptr);

    /// Begins a new spline and returns its id.
    /// @param p starting point of the spline
    /// @param splineWidth width of the spline
    /// @param depth depth-level of the spline
    /// @param color color of the spline
    /// @return id of the new stroke
    Valuable::Node::Uuid beginSpline(Point p, float splineWidth, Radiant::ColorPMA color, float depth);

    /// Begin a new spline and return its id. Use the id to continue or end the spline
    /// @param data contains the spline color, width and initial points
    /// @return id of the new stroke
    Valuable::Node::Uuid beginSpline(const SplineData & data);

    /// Continue the spline with the given id
    /// @param id of the spline
    /// @param point new point to add to the path of spline
    /// @param minimumDistance from the previous point to add a new point, otherwise move
    ///                        the last point to new location
    void continueSpline(Valuable::Node::Uuid id, const Point & point, float minimumDistance = 1.f);

    /// End the spline with the given id, marking it as finished
    void endSpline(Valuable::Node::Uuid id);

    /// Add a complete spline and generate an id for it.
    /// @param data contains the stroke color, width and points
    /// @return id of the new stroke
    Valuable::Node::Uuid addSpline(const SplineData & data);

    /// Add a complete spline and assign the given id for it. This function interprets
    /// the points in the data according to the internal representation of splines
    /// @param info contains the stroke data and id
    void addSpline(const SplineInfo & info);

    /// Remove a spline with the given id
    /// @param id of the stroke to remove
    void removeSpline(Valuable::Node::Uuid id);

    /// Remove the given splines in a batch. This is faster than removing splines one by one
    /// @param splines splines to remove
    void removeSplines(Splines splines);

    /// Return the data of spline with the given id, if it exists
    /// otherwise returns an empty SplineData
    /// @param id of the spline
    /// @return data SplineData of the found spline
    SplineData spline(Valuable::Node::Uuid id) const;

    /// Return all splines in this container
    /// @return list of splines with their ids and data
    Splines allSplines() const;

    /// Render the strokes
    void render(Luminous::RenderContext & r) const;

    /// Serialize the strokes to a string
    /// @return serialized strokes
    QString serialize() const;

    /// Deserialize the strokes from a string
    void deserialize(const QString & str);

    /// Serialize single spline to a string
    /// @return serialized spline
    static QString serializeSpline(const SplineInfo & spline);

    /// Deserialize single spline from a string list
    /// Serialized string needs to be split to lines to use this
    /// @return deserialized spline
    static SplineInfo deserializeSpline(const QStringList & lines, float defaultDepth = 0.f);

    /// Remove all data in the container
    void clear();

    /// Returns true if there's no data in the container
    bool isEmpty() const;

    /// Returns the depth of the currently topmost spline
    float currentDepth() const;

  private:
    class D;
    std::unique_ptr<D> m_d;
  };

  // ------------------------------------------------------------------------------------------

  /// Cubic bezier curve. The curve is defined by four control points. It is guaranteed
  /// that the curve intersects the first and the last control points. Middle control points
  /// are not necessarily intersected.
  class LUMINOUS_API BezierCurve
  {
  public:
    /// Read-access to the control points of the curve
    Nimble::Vector2f operator[](int pos) const;

    /// Read-write access to the control points of the curve
    Nimble::Vector2f& operator[](int pos);

    /// Sets the end points for the curve and calculates control points
    /// @param start Start point of the
    /// @param end
    void setEndPoints(const Nimble::Vector2f& start, const Nimble::Vector2f& end);

    /// Fits curves so that their concatenated curve is smooth (first and second derivatives
    /// exist and are continuos)
    /// @param curve1 First curve to fit. End point and second control point may be modified
    /// @param curve2 Second curve to fit. First control point may be modified
    static void fitCurves(BezierCurve& curve1, BezierCurve& curve2);

    /// Calculates derivate in parameter point t
    Nimble::Vector2f derivate(float t) const;

    /// Minimal rectangle containing the control points of the curve.
    Nimble::Rectf bounds() const;

    /// Length of bounding box's diagonal
    float size() const;

    /// Makes polyline approximation of the curve. Does not include the start point
    /// @param curve Curve to approximate
    /// @param points Result is stored into this vector. Second member of the pair is the
    ///               parametrization of the original curve
    /// @param begin Parameter used in the recursive procedure.
    /// @param end Parameter used in the recursive procedure.
    static void evaluateCurve(const BezierCurve & curve,
                              std::vector<std::pair<Nimble::Vector2f, float>>& points,
                              float begin=0.0f, float end=1.0f);

    /// Makes polyline approximation of the curve. Does not include the start point
    /// @param curve Curve to approximate
    /// @param points Result is stored into this vector
    static void evaluateCurve(const BezierCurve & curve, Luminous::SplineManager::Points & points);

    /// Splits curve into two curves at the given parameter
    /// @param curve Curve to split
    /// @param left First half of the curve (before t)
    /// @param right Second half of the curve (after t)
    /// @param t Where to split the curve
    static void subdivideCurve(const BezierCurve & curve, BezierCurve & left,
                               BezierCurve & right, float t = 0.5f);

    /// Checks whether the curve is flat given the tolerance. Compares @ref curveValue to
    /// the tolerance.
    static bool isFlat(const BezierCurve & curve, float tolerance = .05f);

    /// Calculates indicator value for the curvedness. 0 means that the curve is completely
    /// flat. Values greater than 0 means that the curve is not straight
    static float curveValue(const BezierCurve & curve);

    /// Calculates intersections between the given curve and the given rectangle. Recursively
    /// subdivides the curve.
    /// @param curve Curve to inspect
    /// @param rect Rectangle to inspect
    /// @param intersections Results of the intersection points are stored into this vector.
    ///                      Intersection point is represented by the parameter value of the
    ///                      curve.
    /// @param t Parameter used in the recursive procedure
    /// @param tTolerance Error tolerance for the parameterization
    /// @param sizeTolerance Error tolerance for the curve's @ref size
    /// @param depth Parameter used in the recursive procedure
    static void findIntersections(const BezierCurve & curve, const Nimble::Rectf & rect,
                                  std::vector<float> & intersections, float t = 0.5f,
                                  float tTolerance = 0.01f, float sizeTolerance = .3f,
                                  int depth = 1);

    /// Calculates intersections between the given curve and the given circle. Recursively
    /// subdivides the curve.
    /// @param curve Curve to inspect
    /// @param circle Circle to inspect
    /// @param intersections Results of the intersection points are stored into this vector.
    ///                      Intersection point is represented by the parameter value of the
    ///                      curve.
    /// @param t Parameter used in the recursive procedure
    /// @param tTolerance Error tolerance for the parameterization
    /// @param sizeTolerance Error tolerance for the curve's @ref size
    /// @param depth Parameter used in the recursive procedure
    static void findIntersections(const BezierCurve & curve, const Nimble::Circle & circle,
                                  std::vector<float> & intersections, float t = 0.5f,
                                  float tTolerance = 0.01f, float sizeTolerance = .3f,
                                  int depth = 1);

    std::array<Nimble::Vector2f, 4> m_controlPoints;
  };

}
