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

#include <SDL/SDL.h>

#include <Luminous/RenderContext.hpp>
#include <Luminous/Image.hpp>
#include <Luminous/Texture.hpp>
#include <Luminous/Utils.hpp>

#include <Radiant/Trace.hpp>

#include <Resonant/DSPNetwork.hpp>

#include <VideoDisplay/ShowGL.hpp>

using namespace Radiant;

int main(int argc, char ** argv)
{

  Uint32 flags = SDL_OPENGL;
  int width = 800;
  int height = 400;

  const char * file = 0;

  for(int i = 1; i < argc; i++) {
    const char * arg = argv[i];
    const char * arg2 = (i + 1) < argc ? argv[i+1] : 0;

    if(strcmp(arg, "--fullscreen") == 0)
      flags = flags | SDL_FULLSCREEN;
    else if(strcmp(arg, "--width") == 0&& arg2) {
      width = atoi(arg2);
      i++;
    }
    else if(strcmp(arg, "--height") == 0 && arg2) {
      height = atoi(arg2);
      i++;
    }
    else if(strcmp(arg, "--file") == 0&& arg2) {
      file = arg2;
      i++;
    }
    else {
      printf("Unknown argument \"%s\"\n"
                          "Usage:\n"
                          "%s --file filename [options]\n"
                          " --file  +filename  Set the file to be played\n"
                          " -- width  +int     Set the width of the playback window\n"
                          " --height  +int     Set the height of the playback window\n"
                          " --fullscreen       Turn on fullscreen mode\n"
                          "Example: %s --fullscreen ---width 7680 --height 2160 --file anthem.mov\n",
                          arg, argv[0], argv[0]);
      return 0;
    }
  }

  SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16 );
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1 );

  SDL_SetVideoMode(width, height, 0, flags);

  Luminous::initLuminous();

  glViewport(0, 0, width, height);

  Luminous::RenderContext rsc;
  Luminous::RenderContext::setThreadContext( & rsc);

  std::shared_ptr<Resonant::DSPNetwork> dsp = Resonant::DSPNetwork::instance();
  dsp->start();

  VideoDisplay::ShowGL show;

  show.init(file);

  for(bool running = true; running; ) {
    SDL_Event event;

    if(SDL_PollEvent(&event)) {
      if(event.type == SDL_QUIT) {
        running = false;
        Radiant::info("Quit called, stopping now");
        break;
      }
      else if(event.type == SDL_KEYDOWN) {
        int ascii = (int) event.key.keysym.sym;
        if(ascii == 'q') {
          Radiant::info("Quit called, stopping now");
          break;
        }
        else if(ascii == ' ') {
          show.togglePause();
        }
      }

    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(1.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    show.update();
    show.render( & rsc, Nimble::Vector2(0,0), Nimble::Vector2(width, height), Radiant::Color());

    Luminous::Utils::glCheck("Main.cpp");

    SDL_GL_SwapBuffers();
  }

  info("Stopping video player");
  show.stop();
  info("Stopping DSP network");
  dsp->stop();

  return 0;
}
