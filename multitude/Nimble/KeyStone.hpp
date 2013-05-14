/* Copyright (C) 2007-2013: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 * 
 */

#ifndef NIMBLE_KEYSTONE_HPP
#define NIMBLE_KEYSTONE_HPP

#include "Export.hpp"
#include "LensCorrection.hpp"
#include "Matrix3.hpp"
#include "Rect.hpp"
#include "Vector4.hpp"

#include <array>
#include <cstring>
#include <vector>

namespace Nimble {

  /** Keystone class for transforming between different (skewed) 2D
      coordinate systems, while taking account possible radial (camera
      lens) distortion.

      Conceptually the keystone correction works in following
      fashion. In practice the linear operations are accumulated to a
      single matrix multiplication.

      <OL>

      <LI> We fix the camera lens correction using object of type
      Nimble::LensCorrection. This is simple third-order polynomial
      radial mapping. In the future we can offer a customizeable
      approach so that you can for example set a custom object to do
      the lens correction.

      <LI> We transform the coordinates from camera coordinates to
      normalized coordinates in range [0,1]. This transformation is
      based on knowledge of the four corners that represent the area
      to be tracked.

      <LI> Then we apply extension matrix within the [0,1] space
      (more about that later)

      <LI> Then we multiply the coordinates with the display area
      (visible pixels) and translate them to desired position in the
      screen.

      </OL>

      The extension matrix would not be necessary if you could set the
      corner points of the tracking area freely. In practice it is
      often necessary to limit the camera area so that it does not
      match the desired output area exactly (there might be noise at
      the edges etc). To overcome this limitation one can use an
      extension matrix that that adjusts the area more easily. To
      define the extension matrix, you pass observed and desired
      display coordinates to the applyCorrection function (@see
      applyCorrection).

      @author Tommi Ilmonen
  */
  class NIMBLE_API KeyStone
  {
  public:
    KeyStone();

    virtual ~KeyStone() {}

    /// Set vertices, and other parameters.
    void setVertices(const char * str,
                     int w, int h,
                     int dpyw, int dpyh,
                     int dpyx, int dpyy);

    /// Set vertices, and other parameters.
    void setVertices(const Nimble::Vector2 * vertices,
                     int w, int h,
                     int dpyw, int dpyh,
                     int dpyx, int dpyy);

    /// Sets the output (display) geometry
    void setOutputGeometry(unsigned w, unsigned h, int x, int y);

    /// Sets one vertex without updating matrices.
    void setVertex(int index, float x, float y)
    { m_vertices[index].make(x, y); }

    /// Updates the projection matrix
    void calculateMatrix();

    /// Project a vector from camera coordinates to the display coordinates
    /// This function applies the lens correction and projection
    /// matrix on the coordinates.
    /// @param p Point in camera coordinates
    /// @return Point in display coordinates
    Nimble::Vector2 project(const Nimble::Vector2 & p) const;
    /// Project the point from camera coordinates to normalized
    /// coordinates in range [0,1].
    /// @param p Point in camera coordinates
    /// @return Point in normalized display coordinates
    Nimble::Vector2 project01(const Nimble::Vector2 & p) const;

    /// Do inverse projection (from screen to camera coordinates),
    /// ignoring the camera barrel distortion. Useful as a rough
    /// estimation of the point location on the camera image.
    /// @param p Point in screen coordinates
    /// @return Point in camera coordinates
    Nimble::Vector2 projectInverse(const Nimble::Vector2 & p) const;

    /// Returns a corner point in camera coordinates
    const Nimble::Vector2 & original(int i) const { return m_originals[i]; }
    /// Returns the center point of the camer acoordinates
    Nimble::Vector2 originalCenter() const
    {
      return 0.25f * (m_originals[0] + m_originals[1] +
                      m_originals[2] + m_originals[3]);
    }

    /// Moves the closest corner point
    void moveCorner(Nimble::Vector2);

    /// Returns the index of the corner closest to given point
    int closestCorner(Nimble::Vector2) const;

    /// Returns the coordinates of the top-left corner
    Nimble::Vector2 topLeft() const
    { return m_originals[closestCorner(Nimble::Vector2(0.0f, 0.0f))]; }
    /// Returns the coordinates of the top-right corner
    Nimble::Vector2 topRight() const
    { return m_originals[closestCorner(Nimble::Vector2(float(m_width), 0.0f))]; }

    /// Returns the coordinates of the bottom-left corner
    Nimble::Vector2 bottomLeft() const
    { return m_originals[closestCorner(Nimble::Vector2(0.0f, float(m_height)))]; }
    /// Returns the coordinates of the bottom-right corner
    Nimble::Vector2 bottomRight() const
    { return m_originals[closestCorner(Nimble::Vector2(float(m_width), float(m_height)))]; }

    /// Flips the corner points horizontally.
    void flipHorizontal();
    /// Flips the corner points vertically.
    void flipVertical();

    /// Rotates the keystone corners
    void rotate(int turns = 1);

    /// Writes the order of the corners to the given parameter
    /// @param[out] indices Array of four corner indices
    void getCornerOrdering(int indices[4]);

    /// Information on which pixels are inside the camera area.
    /// Each item (2D vector) contains values for the first pixel inside the
    /// camera area and width of the camera area, per scanline.
    /// @return List of scanline line segments
    const std::vector<Nimble::Vector2i> & limits() const { return m_limits; }
    /// Information on which pixels are part of the image processing area
    /// @return values work like the values returned from #limits.
    const std::vector<Nimble::Vector2i> & extraLimits() const
    { return m_extraLimits; }

    /// Add extra processing borders around one edge.
    /** @param index The index to the edge (0-3).
        @param v The number of pixels to add or subtract. */
    void addExtra(int index, float v);

    /// Number of pixels that this keystone camera area contains
    int containedPixelCount() const { return m_containedPixelCount; }

    /// The width of the output display area
    int dpyWidth()  const { return m_dpyWidth; }
    /// The height of the output display area
    int dpyHeight() const { return m_dpyHeight; }

    /// The size of the display area (in pixels) that matches this keystone area
    Nimble::Vector2i dpySize() const
    { return Nimble::Vector2i(m_dpyWidth, m_dpyHeight); }

    /// The x offset to the origin of the output display area
    int dpyX()  const { return m_dpyX; }
    /// The y offset to the origin of the output display area
    int dpyY()  const { return m_dpyY; }

    /// The offset to the origin of the output display area
    Nimble::Vector2i dpyOffset() const
    { return Nimble::Vector2i(m_dpyX, m_dpyY); }

    /// The output area of the screen
    /// @return Rectangle at [dpyX, dpyY] with size [dpyWidth, dpyHeight]
    Nimble::Rect outputBounds();
    /// Test the keystone correction routines
    static void testCorrection();

    /// The rectangle which contains the Region Of Interest of this keystone
    /// This is the ROI in camera images.
    /// @return Bounding rectangle of the ROI
    Nimble::Rect boundsROI() const { return m_boundsROI; }

    /// Reference to the lens correction
    LensCorrection & lensCorrection() { return m_lensCorrection; }
    /// Const reference to the lens correction
    const LensCorrection & lensCorrection() const { return m_lensCorrection; }

    /// Adjusts the lens correction
    void setLensParam(int i, float v);
    /** Applies correction, based on four screen-space coordinate pairs.

        @param targets The desired target coordinates.

        @param real The observed coordinates.

        @param center the center point observed coordinates
      */
    void calibrateOutput(const Nimble::Vector2 * targets,
                         const Nimble::Vector2 * real,
                         const Nimble::Vector2 * center);
    /// Returns the extension (fine-tuning) matrix
    const Nimble::Matrix3 & outputExtension() const { return m_matrixExtension;}
    /// Sets the extension (fine-tuning) matrix
    /** By default the extension matrix is set to identity.
    @param m extension matrix */
    void setOutputExtension(const Nimble::Matrix3 & m);

    /// Returns the extra pixels around the edges
    const Nimble::Vector4f & extraBorders() const { return m_extra; }
    /// Sets the extra pixels around the edges
    void setExtraBorders(const Nimble::Vector4f & borders)
    { m_extra = borders; updateLimits(); }

    /// Returns information about the center shift
    /// Center shift means that coordinates at the center of the image get this offset.
    /// @return First two elements mean the shift XY, last element is the center shift span
    Nimble::Vector3 centerShift() const
    { return Vector3(m_centerShift.x, m_centerShift.y, m_centerShiftSpan); }
    /// Sets the parameters for the center shifting
    void setCenterShift(Nimble::Vector3 params)
    { m_centerShift = params.vector2(); m_centerShiftSpan = params[2]; }

    /// Recalculates the limits of which pixels are inside the tracking area, and which are not.
    void updateLimits();

    /** Returns the generation number of the object. Whenever the
        keystone information is modified, the generation number is
        incremented. This information can be used by other objects to
        check is they need to update some of their data structures.*/
    /// @return Generation number
    int generation() const { return m_generation; }

    /// Controls if this keystone object uses the center shift features.
    void setUseCenterShift(bool use) { m_useCenterShift = use; }

    /// Calculates the projection matrix.
    /// See Paul Heckbert's master's thesis, pages 19-21. Often you
    /// need to invert this.
    /// @param vertices an array of four corner vertices
    /// @return Generated projection matrix
    static Nimble::Matrix3 projectionMatrix(const std::array<Nimble::Vector2, 4> & vertices);

  private:

    void updated() { m_generation++; }

    void updateLimits(std::vector<Nimble::Vector2i> & limits,
                      const Vector4 * offsets = 0);


    LensCorrection  m_lensCorrection;

    /// Normalized vertices (from 0 to 1)
    std::array<Nimble::Vector2, 4> m_vertices;
    /// Original vertices (in camera coordinates)
    std::array<Nimble::Vector2, 4> m_originals;
    /// Convert from camera coordinates to [0,1]-space
    Nimble::Matrix3 m_matrix;
    /// Convert from camera coordinates to output (screen) coordinates
    Nimble::Matrix3 m_matrixOut;
    /// Fix small discrepancies between ideal output and real output
    Nimble::Matrix3 m_matrixExtension;

    /// Center point shifting:

    Nimble::Vector2 m_centerShift;
    float           m_centerShiftSpan;
    bool            m_useCenterShift;

    // Camera width/height
    int            m_width;
    int            m_height;

    // Display area width/height
    int            m_dpyWidth;
    int            m_dpyHeight;

    Nimble::Vector2f        m_dpyCenter;

    // Display area offset
    int            m_dpyX;
    int            m_dpyY;

    Nimble::Vector4f m_extra;

    // Storage of the pixels to traverse when doing image processing
    std::vector<Nimble::Vector2i> m_limits;
    std::vector<Nimble::Vector2i> m_extraLimits;
    // Total number of pixels to traverse.
    int            m_containedPixelCount;
    int            m_generation;

    Nimble::Rect m_boundsROI;
  };

}


#endif
