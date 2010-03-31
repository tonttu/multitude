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


#ifndef LUMINOUS_GL_KEYSTONE_HPP
#define LUMINOUS_GL_KEYSTONE_HPP

#include <Luminous/Export.hpp>

#include <Nimble/Matrix4.hpp>

#include <Valuable/HasValues.hpp>
#include <Valuable/ValueInt.hpp>
#include <Valuable/ValueVector.hpp>

namespace Luminous {

  /** Class for doing key-stone correction when rendering 2D OpenGL
      graphics. The key-stone matrix is a 4x4 trnasformation matrix
      that is setup up to transform the corner points of all
      polygons/lines/sprites so that they match some skewed coordinate
      system.

      This class is used to do keystone correction by applying an
      OpenGL transformation matrix that skews the output to match
      display geometry imperfections. It is typically used in
      projector-based systems where aligning the projectors physically
      is either impossible or very difficult.
  */
  /// @todo the "rotations" is stupid, and should be removed.
  class LUMINOUS_API GLKeyStone : public Valuable::HasValues
  {
  public:
    /// Creates a new GLKeyStone object.
    GLKeyStone(Valuable::HasValues * parent, const std::string & name);
    virtual ~GLKeyStone();

    /// Returns the object type name = "glkeystone"
    virtual const char * type() const { return "glkeystone"; }

    /// Reads in variables from the DOMElement and calculates the matrix
    virtual bool deserializeXML(Valuable::DOMElement e);

    /// Returns the index to the closest keystone vertex
    int closestVertex(Nimble::Vector2 loc);
    /// Sets the location of the given keystone vertex
    void setVertex(int index, float x, float y)
    { m_vertices[index] = Nimble::Vector2f(x, y); }

    /// Moves the vertex that is closest to the the "loc", to "loc"
    bool moveVertex(Nimble::Vector2 loc);
    /// Selects the closest vertex
    /** This function is used to select the vertex, so that it can be
    moved later on with moveLastVertex. This function is typically
    used by keystone calibration GUI. */
    void selectVertex(Nimble::Vector2 loc);
    /// Moves the index of the selected vertex by one.
    void selectNextVertex()
    { m_selected = (m_selected + 1) % 4; }
    /// Moves the selected vertex by the argument vector.
    void moveLastVertex(const Nimble::Vector2 & move)
    { m_vertices[m_selected] += move; calculateMatrix(); }

    /// Returns the index of the selected vertex.
    int selected() const { return m_selected; }
    /// Returns the location on the selected vertex.
    Nimble::Vector2f selectedVertex() const
    { return m_vertices[m_selected].asVector(); }

    /** Rotate the vertices. This function changes the indices of the
    vertices. */
    void rotateVertices();
    /** Reports the number of times this keystone has been
    rotated. This function is typically used by some GUI code to
    determine in what orientation the whole key-stone thing is. */
    int rotations() const { return m_rotations.asInt(); }

    /** Calculates the OpenGL keystone matrix. */
    void calculateMatrix();

    /** Returns the OpenGL keystone matrix. */
    const Nimble::Matrix4 & matrix() const { return m_matrix; }
    /** Projects the vector v using internal matrix, WITHOUT applying
    perspective correction. */
    Nimble::Vector4 project(Nimble::Vector2 v);
    /** Projects the vector v using matrix m, applying perspective
    correction. */
    static Nimble::Vector4 projectCorrected(const Nimble::Matrix4 & m,
                        Nimble::Vector2 v);
    /// Applies the keystone matrix to the current OpenGL context
    /** This method is typically used as the first step in rendering,
    before drawing anything on the screen. */
    void applyGlState() const;

    /// Cleans up the exterior after the
    void cleanExterior() const;
    /// Returns the location of the vertex that is closest to the argument v.
    Nimble::Vector2 closest(Nimble::Vector2 v) const;

  protected:
    Valuable::ValueVector2f m_vertices[4];
    Nimble::Matrix4 m_matrix;
    int     m_selected;
    Valuable::ValueInt m_rotations;
  };

}

#endif
