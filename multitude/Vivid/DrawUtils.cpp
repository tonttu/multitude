#include "DrawUtils.hpp"

#include <Luminous/Luminous.hpp>

namespace Vivid
{

  namespace DrawUtils
  {

    void drawGrid(KFbxXMatrix &transform)
    {
      glPushMatrix();

      glMultMatrixd((double*)transform);

      glColor3f(0.3f, 0.3f, 0.3f);
      glLineWidth(1.f);

      const int hw = 500;
      const int step = 20;
      const int bigstep = 100;

      for(int i = -hw; i <= hw; i += step) {
        if(i % bigstep == 0)
          glLineWidth(2.f);
        else
          glLineWidth(1.f);

        glBegin(GL_LINES);
        glVertex3f(i, 0, -hw);
        glVertex3f(i, 0,  hw);
        glEnd();

        glBegin(GL_LINES);
        glVertex3f(-hw, 0, i);
        glVertex3f( hw, 0, i);
        glEnd();
      }

      glPopMatrix();
    }

    void setupPerspective(double fovY, double aspect, double nearPlane, double farPlane, KFbxVector4 &eye, KFbxVector4 &center, KFbxVector4 &up)
    {
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      gluPerspective(fovY, aspect, nearPlane, farPlane);

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      gluLookAt(eye[0], eye[1], eye[2], center[0], center[1], center[2], up[0], up[1], up[2]);
    }

    void setupOrthogonal(double leftPlane, double rightPlane, double bottomPlane, double topPlane, double nearPlane, double farPlane, KFbxVector4 & eye, KFbxVector4 & center, KFbxVector4 & up)
    {
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(leftPlane, rightPlane, bottomPlane, topPlane, nearPlane, farPlane);

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      gluLookAt(eye[0], eye[1], eye[2], center[0], center[1], center[2], up[0], up[1], up[2]);
    }

  }

}
