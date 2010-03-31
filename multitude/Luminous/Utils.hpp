/* COPYRIGHT
 *
 * This file is part of Luminous.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Luminous.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in
 * file "LGPL.txt" that is distributed with this source package or obtained
 * from the GNU organization (www.gnu.org).
 *
 */


#ifndef LUMINOUS_UTILS_HPP
#define LUMINOUS_UTILS_HPP

#include <Luminous/Luminous.hpp>

#include <Nimble/Rect.hpp>
#include <Nimble/Matrix3.hpp>

#include <Nimble/Vector2.hpp>
#include <Nimble/Vector4.hpp>

namespace Luminous {

  /// OpenGL utility functions
  /** This class has functions for drawing various simple primitives -
      circles, lines, textured rectangles, etc.*/
  /// @todo deprecate, do new things in RenderContext
  class LUMINOUS_API Utils
  {
  public:

    static void blendCenterSeamHorizontal(int w, int h,
                      int seamWidth,
                      bool withGrid = false);

    enum Edge {
      LEFT,
      RIGHT,
      TOP,
      BOTTOM
    };

    static void fadeEdge(float w, float h, float seam,
             float gamma, Edge e, bool withGrid);

    /** Draw a textured rectangle (GL_QUAD). It is expected that the
    caller has set up the texture modes as necessary. */
    static void glTexRect(float x1, float y1, float x2, float y2);
    static void inline glTexRect(Nimble::Vector2f v1, Nimble::Vector2f v2)
    { glTexRect(v1.x, v1.y, v2.x, v2.y); }
    static void glTexRect(Nimble::Vector2 size, const Nimble::Matrix3 & m);
    static void glTexRectAA(const Nimble::Rect & r, const float * rgba);
    static void glTexRectAA(Nimble::Vector2 size, const Nimble::Matrix3 & m, const float * rgba);
    static void glTexRect(Nimble::Vector2f v1, Nimble::Vector2f v2,
              Nimble::Vector2f uv1, Nimble::Vector2f uv2);
    static void glCenteredTexRect(Nimble::Vector2 size, const Nimble::Matrix3 & m);
    static void glRectWithHole(const Nimble::Rect & area,
                   const Nimble::Rect & hole);
    static void glRectWithHole(const Nimble::Rect & area,
                   const Nimble::Rect & hole,
                   const Nimble::Matrix3 & m);
    static void glRectWithHoleAA(const Nimble::Rect & area,
                 const Nimble::Rect & hole,
                 const Nimble::Matrix3 & m,
                 const float * rgba);

    /// Draw a square using GL_LINE_STRIP
    static void glLineRect(float x1, float y1, float x2, float y2);
    static void glLineRect(const float * corner1, const float *corner2);
    static void glPoint(float x, float y);
    static void glPoint(float *);
    /// Draw a line segment
    static void glLine(float x1, float y1, float x2, float y2);
    static void glLine(const float * p1, const float * p2);
    static void glSoftLine(float x1, float y1, float x2, float y2, float width,
                           const float * color);
    static void glSoftLine(const float * v1, const float * v2, float width,
                           const float * color);
    static void glFilledSoftLine(float x1, float y1, float x2, float y2,
                 float width, float edgeWidth,
                 const float * color);
    static void glFilledSoftLine(const float * v1, const float * v2,
                 float width, float edgeWidth,
                 const float * color);
    static void glFilledLineAA(const float * v1, const float * v2,
                   float width,
                   const Nimble::Matrix3 & m,
                   const float * color);
    /// Draw a cross (X)
    static void glCross(float centerx, float centery, float size, float radians);
    static void glCrossf(float centerx, float centery, float size, float radians);
    static void glCross(const float *loc, float size, float radians);
    /// Draw an arc using GL_LINE_STRIP
    static void glArc(float centerx, float centery, float radius,
              float fromRadians, float toRadians, int linesegments);
    /// Draw an arc with soft edges.
    /** This function can be used to draw arcs that have analytical
        edge-antialiasing. */
    static void glSoftArc(float centerx, float centery, float radius,
              float fromRadians, float toRadians, float width,
              int linesegments, const float * color);
    static void glFilledSoftArc(float centerx, float centery, float radius,
                float fromRadians, float toRadians,
                                float width, float blendwidth,
                int linesegments, const float * color);
    static void glFilledSoftArc(const float * center, float radius,
                float fromRadians, float toRadians,
                                float width, float blendwidth,
                int linesegments, const float * color);
    static void glFilledSoftArc(const Nimble::Matrix3 & m, float radius,
                float fromRadians, float toRadians,
                                float width, float blendwidth,
                int linesegments, const float * color);
    static void glSolidSoftArc(float centerx, float centery, float radius,
                   float fromRadians, float toRadians,
                   float blendwidth,
                   int linesegments, const float * color);
    /// Draw a circle using GL_LINE_STRIP, uses glArc
    static void glCircle(float centerx, float centery, float radius,
             int linesegments);
    /// Draw a filled circle using GL_TRIANGLE_FAN
    static void glFilledCirclef(float centerx, float centery, float radius,
       int linesegments);
    /// Draw a circle with soft edges.
    /** This function can be used to draw circles that have analytical
        edge-antialiasing. */
    static void glSoftCircle(float centerx, float centery, float radius,
                             float width,
                             int linesegments, const float * color);
    static void glFilledSoftCircle(float centerx, float centery, float radius,
                   float width, float blendwidth,
                   int linesegments, const float * color);
    static void glFilledSoftCircle(const float * center, float radius,
                   float width, float blendwidth,
                   int linesegments, const float * color);
    static void glFilledSoftCircle(const Nimble::Matrix3 & m, float radius,
                   float width, float blendwidth,
                   int linesegments, const float * color);
    static void glSolidSoftCircle(float centerx, float centery, float radius,
                  float blendwidth,
                  int linesegments, const float * color);

    static void glSolidSoftCircle(const Nimble::Matrix3 & m, float radius,
                  float blendwidth,
                  int segments, const float * color);

    /// Draw a circle sector ('pie slice') using GL_LINE_STRIP
    static void glSectorf(float centerx, float centery, float radius,
              float fromRadians, float toRadians, int lineSegments);

   /// Draw a filled circle sector ('pie slice') using GL_TRIANGLE_FAN
    static void glFilledSectorf(float centerx, float centery, float radius,
       float fromRadians, float toRadians, int lineSegments);

    static void glFilledSoftLinePolygon(const Nimble::Vector2f * corners, int n,
                    float width, float blendwidth,
                    const float * color);

    static void glFilledSoftLineTriangle(Nimble::Vector2f c1,
                     Nimble::Vector2f c2,
                     Nimble::Vector2f c3,
                     float width, float blendwidth,
                     const float * color);

    static void glTriangle(float x1, float y1, float x2, float y2, float x3, float y3);
    /// Draw a rounded rectangle
    static void glRoundedRectf(const float x1, const float y1,
                   const float x2, const float y2,
                   const float cornerRadius,
                   const int cornerLineSegments);


    static void glRoundedRectfv(const Nimble::Vector2 & low,
                const Nimble::Vector2 & high,
                const float cornerRadius,
                const int cornerLineSegments);


    static void glSoftRoundedRectf(float x1, float y1,
                   float x2, float y2,
                   float cornerRadius,
                   int cornerLineSegments,
                   float lineWidth, float blendWidth,
                   const float * rgba,
                   const Nimble::Matrix3 & m);

    /// Draw a filled rounded rectangle
    static void glFilledRoundedRectf(const float x1, const float y1,
                     const float x2, const float y2,
                     const float cornerRadius,
                     const int cornerLineSegments);
    static void glFilledRoundedRectfv(const Nimble::Vector2 & low,
                      const Nimble::Vector2 & high,
                      const float cornerRadius,
                      const int cornerLineSegments);

    static void glSoftFilledRoundedRectf
      (const float x1, const float y1, const float x2, const float y2,
       const float cornerRadius, const int cornerLineSegments,
       float blendwidth, const float * rgba, const Nimble::Matrix3 & m);

    static void glSoftFilledRoundedRectfv
      (const Nimble::Vector2 & low, const Nimble::Vector2 & high,
       const float cornerRadius, const int cornerLineSegments,
       float blendwidth, const float * rgba, const Nimble::Matrix3 & m);

    /// Enable the most usual OpenGL blend mode
    /** The most usual blend mode is the semi-transparent -glass style
        blend, there the color of a pixel is a weighted average of the
        old and new color values. The blend factor is specified by the
        alpha value. */
    static void glUsualBlend();
    static void glAdditiveBlend();
    static void glSubtractiveBlend();

    static void glGrayf(float level);
    static inline void glWhite() { glGrayf(1.0f); }

    /** Check that there are no OpenGL errors. If there has been an
    error, then the error is printed along with msg. */
    static bool glCheck(const char * msg);

    static void glCircularHalo(float x, float y, float inside, float outside,
                   float radians1,
                   float radians2,
                   int segments, int sectors,
                   const float * rgba);

    static void glCircularHalo(float x, float y, float inside, float outside,
                   float radians1,
                   float radians2,
                   int segments, int sectors,
                   const float * rgba,
                   const Nimble::Matrix3 & m);

    static void glCircularHalo(float inside, float outside,
                   float radians1,
                   float radians2,
                   int segments, int sectors,
                   const float * rgba,
                   const Nimble::Matrix3 & m);

    static void glHorizontalHalo(float x1, float y1,
                 float x2, float y2,
                 float inside, float outside,
                 int segments, int sectors,
                 const float * rgba);

    static void glRectHalo(float x1, float y1,
               float x2, float y2,
               float inside, float outside,
               int segments, int sectors,
               const float * rgba,
               const Nimble::Matrix3 & m);

    static inline void glVertex2(const Nimble::Matrix3 & m, const Nimble::Vector2 & v)
    {
      glVertex2fv((m * v).data());
    }

    static inline void glVertex2(const Nimble::Matrix3 & m, float x, float y)
    {
      glVertex2fv((m * Nimble::Vector2(x, y)).data());
    }

    static inline Nimble::Vector4 project(const Nimble::Matrix3 & m,
                      const Nimble::Vector2 & xy)
    {
      Nimble::Vector3 xyw = m * xy;

      // return Nimble::Vector4(xyw.x / xyw[2], xyw.y / xyw[2], 0, xyw[2]);
      return Nimble::Vector4(xyw.x, xyw.y, 0, xyw[2]);
    }

    static void glRedYellowGreenRamp(float x0, float y0, float x1, float y1, const Nimble::Matrix3 & m);

  };

}

#endif
