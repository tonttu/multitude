
#include <Luminous/Luminous.hpp>

#include <SDL/SDL.h>

#include <Luminous/GLResources.hpp>
#include <Luminous/Image.hpp>
#include <Luminous/Texture.hpp>
#include <Luminous/Utils.hpp>

#include <Radiant/Trace.hpp>

using namespace Radiant;

int main(int argc, char ** argv)
{
  if(argc != 2) {
    printf("Usage: %s <file>\n", argv[0]);
    return 1;
  }
  argc--; argv++;
  const char * file = argv[0];

  SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16 );
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1 );

  SDL_SetVideoMode(400 , 400, 0, SDL_OPENGL);

  Luminous::initLuminous();

  glViewport(0, 0, 400, 400);

  Luminous::GLResources rsc(Radiant::ResourceLocator::instance());
  Luminous::GLResources::setThreadResources( & rsc, 0, 0);

  Luminous::ImageInfo info;
  if(Luminous::Image::ping(file, info)) {
    printf("%s : %d x %d\n", file, info.width, info.height);
  }

  Luminous::Image image;
  if(!image.read(file)) {
    printf("failed to open %s\n", file);
    return 1;
  }

  for(bool running = true; running; ) {
    SDL_Event event;

    if(SDL_PollEvent(&event)) {
      switch(event.type) {
      case SDL_QUIT:
        running = false;
	Radiant::info("Quit called, stopping now");
        break;
      };
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 400, 0, 400, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(1.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    // glDisable(GL_TEXTURE_2D);
    glColor3f(1.f, 1.f, 1.f);
    image.bind();

    Luminous::Utils::glTexRect(50, 350, 350, 50);

    Luminous::Utils::glCheck("Main.cpp");

    SDL_GL_SwapBuffers();
  }

  // Debug
  const char * output = "debug.jpg";
  if(!image.write(output)) {
    printf("failed to save %s\n", output);
    return false;
  }

  return 0;
}
