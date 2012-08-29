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

#include <Luminous/Utils.hpp>
#include <Luminous/DummyOpenGL.hpp>

#include <Radiant/Trace.hpp>

#include <cmath>
#include <cassert>

namespace Luminous {

  using namespace Nimble;

//  void Utils::fadeEdge(float w, float h, float seam,
//               float gamma, Edge e, bool withGrid)
//  {
//    glDisable(GL_LIGHTING);
//    glDisable(GL_CULL_FACE);
//    glDisable(GL_TEXTURE_2D);
//    // glUseProgram(0);
//    glUsualBlend();
//    glEnable(GL_BLEND);
//    glShadeModel(GL_SMOOTH);

//    MatrixStep ms;

//    bool horiz = true;

//    if(e == RIGHT) // The default
//      ;
//    else if(e == LEFT) {
//      glScalef(-1, 1, 1);
//      glTranslatef(-w, 0, 0);
//    }
//    else {
//      horiz = false;

//      if(e == BOTTOM) {
//    glScalef(1, -1, 1);
//    glTranslatef(0, -h, 0);
//      }
//    }

//    int i, n = 16;
//    float left = w - seam;
//    float top = h - seam;

//    glBegin(GL_QUAD_STRIP);

//    // glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

//    if(horiz) {

//      float hextra = h * 0.5f;

//      for(i = 0; i <= n; i++) {

//    float rel = i / (float) n;
//    float x = left + seam * rel * 1 + 0.00001;

//    glColor4f(0.0f, 0.0f, 0.0f, powf(rel, gamma));
//    glVertex2f(x, -hextra);
//    glVertex2f(x, h + hextra);
//      }
//    }
//    else {
//      float wextra = w * 0.5f;

//      for(i = 0; i <= n; i++) {

//    float rel = i / (float) n;
//    float y = top + seam * rel * 1.0 + 0.00001f;

//    glColor4f(0.0f, 0.0f, 0.0f, powf(rel, gamma));
//    glVertex2f(-wextra, y);
//    glVertex2f(w + wextra, y);

//      }
//    }

//    glEnd();

//    if(!withGrid)
//      return;

//    glLineWidth(3);

//    static const Vector3 colors[4] = {
//      Vector3(1, 0, 0),
//      Vector3(0, 1, 0),
//      Vector3(0, 0, 1),
//      Vector3(0.5, 0.5, 0)
//    };

//    glColor3fv(colors[(int) e].data());
//    glBegin(GL_LINES);

//    n = 10;
//    float first = 1.5f;
//    float last = w - 1.5f;
//    float ampl = last - first;

//    for(i = 0; i <= n; i++) {
//      float x = first + ampl * i / (float)n;
//      glVertex2f(x, 0.0f);
//      glVertex2f(x, h);
//    }

//    last = h - 1.5f;
//    ampl = last - first;

//    glVertex2f(left, 0);
//    glVertex2f(left, h);

//    for(i = 0; i <= n; i++) {
//      float y = first + ampl * i / (float)n;
//      glVertex2f(0.0f, y);
//      glVertex2f(w, y);
//    }

//    glVertex2f(0.0f, 0.0f);
//    glVertex2f(w, h);

//    glVertex2f(w, 0.0f);
//    glVertex2f(0.0f, h);

//    glColor3f(0.0f, 0.0f, 0.0f);

//    glVertex2f(left + seam * 0.5f, 0.0f);
//    glVertex2f(left + seam * 0.5f, h);

//    glEnd();
//  }
}