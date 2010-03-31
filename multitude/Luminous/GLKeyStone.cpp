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

#include <Luminous/Luminous.hpp>
#include <Luminous/GLKeyStone.hpp>

#include <Valuable/DOMElement.hpp>

namespace Luminous {

  using Nimble::Matrix4;
  using Nimble::Vector2;
  using Nimble::Vector4;

  GLKeyStone::GLKeyStone(HasValues * parent, const std::string & name)
  : HasValues(parent, name, false),
  m_selected(0),
  m_rotations(this, "rotations", false, 0)
  {
    setVertex(0, 0, 0);
    setVertex(1, 1, 0);
    setVertex(2, 1, 1);
    setVertex(3, 0, 1);

    addValue("v1", &m_vertices[0]);
    addValue("v2", &m_vertices[1]);
    addValue("v3", &m_vertices[2]);
    addValue("v4", &m_vertices[3]);

    calculateMatrix();
  }

  GLKeyStone::~GLKeyStone()
  {}

  bool GLKeyStone::deserializeXML(Valuable::DOMElement e)
  {
    if(!Valuable::HasValues::deserializeXML(e))
      return false;

    calculateMatrix();

    return true;
  }

  int GLKeyStone::closestVertex(Nimble::Vector2 loc)
  {
    int index = 0;
    float best = (m_vertices[0].asVector() - loc).length();

    for(int i = 1; i < 4; i++) {
      float d = (m_vertices[i].asVector() - loc).length();
      if(best > d) {
        best = d;
        index = i;
      }
    }
    return index;
  }

  bool GLKeyStone::moveVertex(Vector2 loc)
  {
    selectVertex(loc);

    if((m_vertices[m_selected].asVector() - loc).length() > 0.1f)
      return false;

    m_vertices[m_selected] = loc;

    calculateMatrix();

    return true;
  }

  void GLKeyStone::selectVertex(Vector2 loc)
  {
    m_selected = closestVertex(loc);
  }

  void GLKeyStone::rotateVertices()
  {
    Vector2 v = m_vertices[0].asVector();

    m_vertices[0] = m_vertices[1].asVector();
    m_vertices[1] = m_vertices[2].asVector();
    m_vertices[2] = m_vertices[3].asVector();
    m_vertices[3] = v;

    m_rotations++;

    calculateMatrix();
  }


  /** Calculates the projection matrix. See Paul Heckbert's master's
   * thesis, pages 19-21. */

  void GLKeyStone::calculateMatrix()
  {
    // Formula from page 20.
    float dx1 = m_vertices[1][0] - m_vertices[2][0];
    float dx2 = m_vertices[3][0] - m_vertices[2][0];
    float dy1 = m_vertices[1][1] - m_vertices[2][1];
    float dy2 = m_vertices[3][1] - m_vertices[2][1];

    float sx = m_vertices[0][0] - m_vertices[1][0] +
      m_vertices[2][0] - m_vertices[3][0];

    float sy = m_vertices[0][1] - m_vertices[1][1] +
      m_vertices[2][1] - m_vertices[3][1];

    float del = Nimble::Math::Det(dx1, dx2, dy1, dy2);

    float g = Nimble::Math::Det(sx, dx2, sy, dy2) / del;
    float h = Nimble::Math::Det(dx1, sx, dy1, sy) / del;

    float a = m_vertices[1][0] - m_vertices[0][0] + g * m_vertices[1][0];
    float b = m_vertices[3][0] - m_vertices[0][0] + h * m_vertices[3][0];
    float c = m_vertices[0][0];

    float d = m_vertices[1][1] - m_vertices[0][1] + g * m_vertices[1][1];
    float e = m_vertices[3][1] - m_vertices[0][1] + h * m_vertices[3][1];
    float f = m_vertices[0][1];

    m_matrix.make(a, b, 0, c,
                  d, e, 0, f,
                  0, 0, 1, 0,
                  g, h, 0, 1);
  }

  Vector4 GLKeyStone::project(Vector2 v)
  {
    Vector4 tmp(v.x, v.y, 0.5, 1.0);
    Vector4 p = m_matrix * tmp;

    // g * u + h * v + 1
    float zscale = m_matrix[3][0] * v.x + m_matrix[3][1] * v.y + 1.0f;
    p.z *= zscale;
    return p;
  }

  Vector4 GLKeyStone::projectCorrected(const Nimble::Matrix4 & m, Nimble::Vector2 v)
  {
    Vector4 tmp(v.x, v.y, 0.0, 1.0);
    Vector4 p = m * tmp;

    // g * u + h * v + 1
    float zscale = m[3][0] * v.x + m[3][1] * v.y + 1.0f;
    p.z *= zscale;
    p.x /= p.w;
    p.y /= p.w;
    return p;
  }

  void GLKeyStone::applyGlState() const
  {
    /* if(*m_vertices[0] == Vector2(0, 0) &&
       *m_vertices[1] == Vector2(1, 0) &&
       *m_vertices[2] == Vector2(1, 1) &&
       *m_vertices[3] == Vector2(0, 1))
      return;
    */
    glTranslatef(-1.0, -1.0, 0.0);
    glScalef(2.0, 2.0, 2.0);
    //if(GLEW_ARB_transpose_matrix)
      glMultTransposeMatrixf(m_matrix.data());
    /*
      else {
      Matrix4 tmp(m_matrix);
      tmp.transpose();
      glMultMatrixf(tmp.data());
    }
    */
    glScalef(0.5, 0.5, 0.5);
    glTranslatef(1.0, 1.0, 0.0);
  }

  void GLKeyStone::cleanExterior() const
  {
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_CULL_FACE);
    glColor3f(0, 0, 0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluOrtho2D(0, 1, 0, 1);

    glBegin(GL_TRIANGLE_STRIP);

    glVertex2f(0, 0);
    glVertex2fv(closest(Vector2(0, 0)).data());

    glVertex2f(1, 0);
    glVertex2fv(closest(Vector2(1, 0)).data());

    glVertex2f(1, 1);
    glVertex2fv(closest(Vector2(1, 1)).data());

    glVertex2f(0, 1);
    glVertex2fv(closest(Vector2(0, 1)).data());

    glVertex2f(0, 0);
    glVertex2fv(closest(Vector2(0, 0)).data());

    glEnd();
  }

  Vector2 GLKeyStone::closest(Vector2 loc) const
  {
    int index = 0;
    float best = (m_vertices[0].asVector() - loc).length();

    for(int i = 1; i < 4; i++) {
      float d = (m_vertices[i].asVector() - loc).length();
      if(best > d) {
        best = d;
        index = i;
      }
    }

    return m_vertices[index].asVector();
  }
}
