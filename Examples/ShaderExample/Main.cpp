/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include <Luminous/Luminous.hpp>
#include <Luminous/Shader.hpp>
#include <Luminous/Utils.hpp>
#include <Luminous/RenderContext.hpp>

#include <Radiant/Sleep.hpp>

#include <SDL/SDL.h>

#define SHADER(str) #str

int main(int, char**)
{
  Radiant::enableVerboseOutput(true);

  Luminous::Shader red, green, blue, rings;

  /* First three ultra-simple shaders: */
  red.setFragmentShader
      ("void main(void) { gl_FragColor = vec4(1, 0, 0, 1); }");
  green.setFragmentShader
      ("void main(void) { gl_FragColor = vec4(0, 1, 0, 1); }");
  blue.setFragmentShader
      ("void main(void) { gl_FragColor = vec4(0, 0, 1, 1); }");

  /* Then something a bit more interesting. This creates rings. When the
     scale goes up, we get really cool moire-effects.
  */
  rings.setFragmentShader(SHADER(
      uniform float scale;
      void main(void) {
        vec2 offset = gl_TexCoord[0].st - vec2(0.5, 0.5);
        float val = 0.5 + 2.5 *  sin(length(offset) * scale);
        gl_FragColor = vec4(val, val, val, 1);
      }
    ));

  Valuable::AttributeFloat scale(0, "scale", 10.0f);
  rings.addShaderUniform( & scale);

  SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16 );
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1 );

  SDL_SetVideoMode(600 , 600, 0, SDL_OPENGL);

  Luminous::initLuminous();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, 600, 0, 600, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  Luminous::RenderContext rsc;
  Luminous::RenderContext::setThreadContext( & rsc);
  Luminous::Utils::glUsualBlend();

  glColor3f(1.0f, 0.5f, 0.5f);

  int index = 0;

  for(bool running = true; running; ) {
    index++;

    SDL_Event event;

    if(SDL_PollEvent( & event)) {
      if(event.type == SDL_QUIT)
        running = false;
      else if((event.type == SDL_KEYDOWN) &&
         (event.key.keysym.sym == SDLK_ESCAPE))
        running = false;
    }

    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    red.bind();
    Luminous::Utils::glTriangle(30, 30, 30, 270, 270, 70);

    green.bind();
    Luminous::Utils::glTriangle(30, 570, 30, 330, 270, 370);

    blue.bind();
    Luminous::Utils::glTriangle(330, 570, 330, 330, 570, 540);

    scale = (index % 10000);
    rings.bind();
    Luminous::Utils::glTexRect(330, 30, 570, 270);


    Luminous::Utils::glCheck("After rendering");

    SDL_GL_SwapBuffers();
#ifndef WIN32
    Radiant::Sleep::sleepMs(20);
#endif
  }

  return 0;
}
