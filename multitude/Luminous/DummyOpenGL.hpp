#ifndef LUMINOUS_DUMMYOPENGL_HPP
#define LUMINOUS_DUMMYOPENGL_HPP

/* Dummy implementations of various OpenGL functions/macros which are present in
   full OpenGL, but not in OpenGL ES 2.0.


*/

namespace Luminous
{
  void dumymWarn(const char * funcname, const char * file, int line);
}

#define DUMMY_OPENGL_M(macro) \
  { Luminous::dumymWarn("##macro", __FILE__, __LINE__); }

#define DUMMY_OPENGL_0(func) inline void func()\
  { Luminous::dumymWarn("##func", __FILE__, __LINE__); }
#define DUMMY_OPENGL_1(func) inline void func(int) \
  { Luminous::dumymWarn("##func", __FILE__, __LINE__); }

// #define GL_QUADS

//  DUMMY_OPENGL_0(glPushMatrix);
//  DUMMY_OPENGL_0(glPopMatrix);
  DUMMY_OPENGL_0(glEnd);

#define glMultTransposeMatrixf(a) DUMMY_OPENGL_M(glMultTransposeMatrixf)


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

#define glVertex4f(a,b,c,d) DUMMY_OPENGL_M(glVertex4f)
#define glVertex4fv(a) DUMMY_OPENGL_M(glVertex4fv)

#define glMultiTexCoord2fv(a,b) DUMMY_OPENGL_M(glMultiTexCoord2fv)


#define glColor4f(a,b,c,d) DUMMY_OPENGL_M(glColor4f)
#define glColor4fv(a) DUMMY_OPENGL_M(glColor4fv)

#define glColor3f(a,b,c) DUMMY_OPENGL_M(glColor3f)
#define glColor3fv(a) DUMMY_OPENGL_M(glColor3fv)

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
#endif // DUMMYOPENGL_HPP
