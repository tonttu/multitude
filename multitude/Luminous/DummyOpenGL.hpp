#ifndef LUMINOUS_DUMMYOPENGL_HPP
#define LUMINOUS_DUMMYOPENGL_HPP

#include "Export.hpp"
#include <Radiant/Platform.hpp>

#if defined(RADIANT_OSX)

/* Dummy implementations of various OpenGL functions/macros which are present in
   full OpenGL, but not in OpenGL ES 2.0.

   The point of these macros is to make the software compile. At the same time they will give out
   a warning about where these illegal functions are used.
*/

namespace Luminous
{
  LUMINOUS_API void dumymWarn(const char * funcname, const char * file, int line);
  LUMINOUS_API int dummyEnum(const char * file, int line);
}

#define DUMMY_OPENGL_E(enu) Luminous::dummyEnum(__FILE__, __LINE__)

#define DUMMY_OPENGL_M(macro) \
    Luminous::dumymWarn("##macro", __FILE__, __LINE__)

#define DUMMY_OPENGL_0(func) inline void func()\
    { Luminous::dumymWarn("##func", __FILE__, __LINE__); }
#define DUMMY_OPENGL_1(func) inline void func(int) \
    { Luminous::dumymWarn("##func", __FILE__, __LINE__); }

// #define GL_QUADS

DUMMY_OPENGL_0(glPushMatrix);
DUMMY_OPENGL_0(glPopMatrix);
DUMMY_OPENGL_0(glEnd);

#define glMultTransposeMatrixf(a) DUMMY_OPENGL_M(glMultTransposeMatrixf)
#define glMatrixMode(a) DUMMY_OPENGL_M(glMatrixMode)
#define glLoadIdentity() DUMMY_OPENGL_M(glLoadIdentity)


#define glEnableClientState(a) DUMMY_OPENGL_M(glEnableClientState)
#define glDisableClientState(a) DUMMY_OPENGL_M(glDisableClientState)

#define glVertexPointer(a, b, c, d) DUMMY_OPENGL_M(glVertexPointer)
#define glTexCoordPointer(a, b, c, d) DUMMY_OPENGL_M(glTexCoordPointer)
#define glColorPointer(a, b, c, d) DUMMY_OPENGL_M(glColorPointer)

#define GL_VERTEX_ARRAY DUMMY_OPENGL_E(GL_VERTEX_ARRAY)
#define GL_TEXTURE_COORD_ARRAY DUMMY_OPENGL_E(GL_TEXTURE_COORD_ARRAY)
#define GL_COLOR_ARRAY DUMMY_OPENGL_E(GL_COLOR_ARRAY)

#define glClipPlanef(a, b) DUMMY_OPENGL_M(glClipPlanef)
#define glClipPlane(a, b) DUMMY_OPENGL_M(glClipPlane)

#define GL_CLIP_PLANE0 DUMMY_OPENGL_E(GL_CLIP_PLANE0)
#define GL_CLIP_PLANE1 DUMMY_OPENGL_E(GL_CLIP_PLANE1)
#define GL_CLIP_PLANE2 DUMMY_OPENGL_E(GL_CLIP_PLANE2)
#define GL_CLIP_PLANE3 DUMMY_OPENGL_E(GL_CLIP_PLANE3)
#define GL_CLIP_PLANE4 DUMMY_OPENGL_E(GL_CLIP_PLANE4)
#define GL_CLIP_PLANE5 DUMMY_OPENGL_E(GL_CLIP_PLANE5)

#define glPointSize(a) DUMMY_OPENGL_M(glPointSize)
#define GL_POINT_SMOOTH DUMMY_OPENGL_E(GL_POINT_SMOOTH)

#define glTranslatef(a, b, c) DUMMY_OPENGL_M(glTranslatef)
#define glScalef(a, b, c) DUMMY_OPENGL_M(glScalef)

#define glOrthof(a, b, c, d, e, f) DUMMY_OPENGL_M(glOrthof)

#define glBegin(a) DUMMY_OPENGL_M(glBegin)
#define glPushAttrib(a) DUMMY_OPENGL_M(glPushAttrib)
#define glPopAttrib() DUMMY_OPENGL_M(glPopAttrib)

#define glDrawBuffer(a) DUMMY_OPENGL_M(glDrawBuffer)
#define glDrawBuffers(a,b) DUMMY_OPENGL_M(glDrawBuffers)

#define glBegin(a) DUMMY_OPENGL_M(glBegin)
// #define glMatrixMode(a) DUMMY_OPENGL_M(glMatrixMode)

#define glTexCoord2f(a,b) DUMMY_OPENGL_M(glTexCoord2f)
#define glTexCoord2fv(a) DUMMY_OPENGL_M(glTexCoord2fv)

#define glVertex2f(a,b) DUMMY_OPENGL_M(glVertex2f)
#define glVertex2fv(a) DUMMY_OPENGL_M(glVertex2fv)
#define glVertex2iv(a) DUMMY_OPENGL_M(glVertex2iv)

#define glVertex3f(a,b,c) DUMMY_OPENGL_M(glVertex3f)
#define glVertex3fv(a) DUMMY_OPENGL_M(glVertex3fv)

#define glVertex4f(a,b,c,d) DUMMY_OPENGL_M(glVertex4f)
#define glVertex4fv(a) DUMMY_OPENGL_M(glVertex4fv)

#define glMultiTexCoord2fv(a,b) DUMMY_OPENGL_M(glMultiTexCoord2fv)


#define glColor4f(a,b,c,d) DUMMY_OPENGL_M(glColor4f)
#define glColor4fv(a) DUMMY_OPENGL_M(glColor4fv)
#define glColor4ub(a,b,c,d) DUMMY_OPENGL_M(glColor4ub)

#define glColor3f(a,b,c) DUMMY_OPENGL_M(glColor3f)
#define glColor3fv(a) DUMMY_OPENGL_M(glColor3fv)

#define glOrtho(a,b,c,d,e,f) DUMMY_OPENGL_M(glOrtho)

#define glProgramParameteri(a,b,c) DUMMY_OPENGL_M(glProgramParameteri)

#define glLoadMatrixf(a) DUMMY_OPENGL_M(glLoadMatrixf)
#define gluOrtho2D(a,b,c,d) DUMMY_OPENGL_M(gluOrtho2D)

#define gluBuild1DMipmaps(a,b,c,d,e,f) DUMMY_OPENGL_M(gluBuild1DMipmaps)
#define gluBuild2DMipmaps(a,b,c,d,e,f,g) DUMMY_OPENGL_M(gluBuild2DMipmaps)

#define glRectf(a,b,c,d) DUMMY_OPENGL_M(glRectf)
#define glRecti(a,b,c,d) DUMMY_OPENGL_M(glRecti)
#define glRectfv(a,b) DUMMY_OPENGL_M(glRectfv)

#define gluLookAt(a,b,c,d,e,f,g,h,i) DUMMY_OPENGL_M(gluLookAt)

#define glNormal3f(a,b,c) DUMMY_OPENGL_M(glNormal3f)
#define glNormal3fv(a) DUMMY_OPENGL_M(glNormal3fv)

#define glRotatef(a,b,c,d) DUMMY_OPENGL_M(glRotatef)

#define glMultiTexCoord2fARB(a,b,c) DUMMY_OPENGL_M(glMultiTexCoord2fARB)
#define glMultiTexCoord3fARB(a,b,c,d) DUMMY_OPENGL_M(glMultiTexCoord3fARB)

  /*
  inline glMultTransposeMatrixf(float * data)
  {
    float tmp[16];
    tmp[0] = data[0];
    glMultMatrixf(tmp);
  }
*/
/*
inline void gluOrtho2D(float left, float right, float bot, float top)
{
  glOrthof(left, right, -1.5f, 1.5f, -1.0f, 1.0f);
}
*/

#endif // RADIANT_OSX

#endif // DUMMYOPENGL_HPP
