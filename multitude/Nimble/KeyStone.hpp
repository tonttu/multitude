/* COPYRIGHT
 *
 * This file is part of Nimble.
 *
 * Copyright: MultiTouch Oy, Helsinki University of Technology and others.
 *
 * See file "Nimble.hpp" for authors and more details.
 *
 * This file is licensed under GNU Lesser General Public
 * License (LGPL), version 2.1. The LGPL conditions can be found in 
 * file "LGPL.txt" that is distributed with this source package or obtained 
 * from the GNU organization (www.gnu.org).
 * 
 */

#ifndef NIMBLE_KEYSTONE_HPP
#define NIMBLE_KEYSTONE_HPP

#include <Nimble/Export.hpp>
#include <Nimble/LensCorrection.hpp>
#include <Nimble/Matrix3.hpp>
#include <Nimble/Rect.hpp>
#include <Nimble/Vector4.hpp>

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

    /// Updates the proection matrix
    void calculateMatrix();

    /// Project a vector from camera coordinates to the display coordinates
    /** This function applies the lens correction and projection
  matrix on the coordinates. */
    Nimble::Vector2 project(const Nimble::Vector2 &) const;
    /** Project the point from camera coordinates to normalized
  coordinates in range [0,1].*/
    Nimble::Vector2 project01(const Nimble::Vector2 &) const;
    /// Applies a 3x3 correction marix on a 2D vector.
    static Nimble::Vector2 project(const Nimble::Matrix3 & m,
                                   const Nimble::Vector2 & v)
    {
      Nimble::Vector3 p = m * v;
      return Nimble::Vector2(p.x / p.z, p.y / p.z);
    }
    
    /** Do inverse projection (from screen to camera coordinates),
  ignoring the camera barrel distortion. Useful as a rough
  estimation of the point location on the camera image. x*/
    Nimble::Vector2 projectInverse(const Nimble::Vector2 &) const;

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

    /// Information on which pixels are inside the camera area
    /** Each item (2D vector) contains values for the first pixel
  inside the camera area and width of the camera area, per
  scanline. */
    const std::vector<Nimble::Vector2i> & limits() const { return m_limits; }
    const std::vector<Nimble::Vector2i> & extraLimits() const
    { return m_extraLimits; }

    void addExtra(int index, float v);

    /// Number of pixels that this keystone camera area contains
    int containedPixelCount() const { return m_containedPixelCount; }

    /// The width of the output display area
    int dpyWidth()  const { return m_dpyWidth; }
    /// The height of the output display area
    int dpyHeight() const { return m_dpyHeight; }

    Nimble::Vector2f dpySize() const
    { return Nimble::Vector2i(m_dpyWidth, m_dpyHeight); }

    /// The x offset to the origin of the output display area
    int dpyX()  const { return m_dpyX; }
    /// The y offset to the origin of the output display area
    int dpyY()  const { return m_dpyY; }

    Nimble::Vector2f dpyOffset() const
    { return Nimble::Vector2i(m_dpyX, m_dpyY); }
    
    /// The output area of the screen
    /** This function basically returns the information you would
  get from dpyWidth, dpyheight, dpyX and dpyY. */
    Nimble::Rect outputBounds();
    /// Test the keystone correction routines
    static void testCorrection();

    /// Reference to the lens correction
    LensCorrection & lensCorrection() { return m_lensCorrection; }
    
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
    /** By default the extension matrix is set to identity. */
    void setOutputExtension(const Nimble::Matrix3 & m);

    const Nimble::Vector4f & extraBorders() const { return m_extra; }
    void setExtraBorders(const Nimble::Vector4f & borders)
    { m_extra = borders; updateLimits(); }

    Nimble::Vector3 centerShift()
    { return Vector3(m_centerShift.x, m_centerShift.y, m_centerShiftSpan); }
    void setCenterShift(Nimble::Vector3 params)
    { m_centerShift = params.xy(); m_centerShiftSpan = params[2]; }


    void updateLimits();

    /** Returns the version number of the object. Whenever the
  keystone information is modified, the version number is
  incremented. This information can be used by other objects to
  check is they need to update some of their data structures.*/
    int version() const { return m_version; }

    void setUseCenterShift(bool use) { m_useCenterShift = use; }

    /// Calculates the projection matrix.
    /** See Paul Heckbert's master's thesis, pages 19-21. Often you
        need to invert this. */
    static Nimble::Matrix3 projectionMatrix(const Nimble::Vector2 * vertices);

  private:

    void updated() { m_version++; }

    void updateLimits(std::vector<Nimble::Vector2i> & limits, 
                      const Vector4 * offsets = 0);


    LensCorrection  m_lensCorrection;

    /// Normalized vertices (from 0 to 1)
    Nimble::Vector2 m_vertices[4];
    /// Original vertices (in camera coordinates)
    Nimble::Vector2 m_originals[4];
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

    Vector2        m_dpyCenter;

    // Display area offset
    int            m_dpyX;
    int            m_dpyY;

    Nimble::Vector4f m_extra;

    // Storage of the pixels to traverse when doing image processing
    std::vector<Nimble::Vector2i> m_limits;
    std::vector<Nimble::Vector2i> m_extraLimits;
    // Total number of pixels to traverse.
    int            m_containedPixelCount;
    int            m_version;
  };
  
}


#endif
