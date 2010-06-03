
#include <Luminous/Luminous.hpp>

#include <SDL/SDL.h>

#include <Luminous/GLResources.hpp>
#include <Luminous/Image.hpp>
#include <Luminous/Texture.hpp>
#include <Luminous/Utils.hpp>

#include <Radiant/Trace.hpp>

using namespace Radiant;

int main(int, char **)
{
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

  int i, j = 0;
  const int levels = 13;
  const int texturesperlevel = 5;

  Luminous::Image images[levels];

  for(i = 0; i < levels; i++) {
    int dim = 1 << i;
    images[i].allocate(dim, dim, Luminous::PixelFormat::rgbUByte());
  }

  info("Built the relevant images for testing.");

  Luminous::Texture2D textures[texturesperlevel];



  for(int frame = 0; frame < 3; frame++) {
    SDL_Event event;

    if(SDL_PollEvent(&event)) {
      switch(event.type) {
      case SDL_QUIT:
        frame = 1000000;
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

    info("\nFRAME %d", i);

    for(i = 5; i < levels; i++) {

      Radiant::TimeStamp t1 = Radiant::TimeStamp::getTime();

      for(j = 0; j < texturesperlevel; j++) {
        // Create textures without loading:
        textures[j].loadBytes(GL_RGB, images[i].width(), images[i].height(), 0,
                              Luminous::PixelFormat::rgbUByte(), false);
      }

      double createTime = t1.sinceSecondsD() * 1000 / texturesperlevel;


      t1 = Radiant::TimeStamp::getTime();

      for(j = 0; j < texturesperlevel; j++) {
        // Create textures and load the actual data:
        textures[j].loadBytes(GL_RGB, images[i].width(), images[i].height(),
                              images[i].data(),
                              Luminous::PixelFormat::rgbUByte(), false);
      }

      double loadTime = t1.sinceSecondsD() * 1000 / texturesperlevel;

      t1 = Radiant::TimeStamp::getTime();

      for(j = 0; j < texturesperlevel; j++) {
        // Create textures and load the actual data:
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, images[i].width(), images[i].height(),
                        GL_RGB, GL_UNSIGNED_BYTE, images[i].data());
      }

      double subloadTime = t1.sinceSecondsD() * 1000 / texturesperlevel;

      info("Texture dimensions %d %d, create = %.3lf, load = %.3lf reload = %.3lf milliseconds",
           images[i].width(), images[i].height(),
           createTime, loadTime, subloadTime);


    }

    SDL_GL_SwapBuffers();
  }


  return 0;
}
