
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
  const int levels = 12;
  const int texturesperlevel = 5;
  const int formatsperlevel = 3;

  Luminous::Image images[levels][formatsperlevel];

  for(i = 0; i < levels; i++) {
    int dim = 1 << i;
    images[i][0].allocate(dim, dim, Luminous::PixelFormat::rgbUByte());
    images[i][1].allocate(dim, dim, Luminous::PixelFormat::rgbaUByte());
    images[i][2].allocate(dim, dim, Luminous::PixelFormat::bgraUByte());
  }

  info("Built the relevant images for testing.");

  Luminous::Texture2D textures[texturesperlevel][formatsperlevel];

  /* Test how long it takes to upload textures into the GPU, with different pixel formats. */

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
    glEnable(GL_TEXTURE_2D);

    info("\nFRAME %d", frame);

    for(i = 5; i < levels; i++) {

      printf("\n");

      for(int k = 0; k < formatsperlevel; k++) {

        GLenum glLayout = (k == 0) ? GL_RGB : GL_RGBA;

        Radiant::TimeStamp t1 = Radiant::TimeStamp::getTime();

        for(j = 0; j < texturesperlevel; j++) {
          // Create textures without loading:
          textures[j][k].loadBytes(glLayout, images[i][k].width(), images[i][k].height(), 0,
                                   images[i][k].pixelFormat(), false);
          // Luminous::Utils::glTexRect(Nimble::Vector2(0,100), Nimble::Vector2(10, 110));

        }


        double createTime = t1.sinceSecondsD() * 1000 / texturesperlevel;

        Luminous::Utils::glCheck("Texture test 1/3");

        t1 = Radiant::TimeStamp::getTime();


        for(j = 0; j < texturesperlevel; j++) {
          // Create textures and load the actual data:
          textures[j][k].loadBytes(glLayout, images[i][k].width(), images[i][k].height(),
                                   images[i][k].data(),
                                   images[i][k].pixelFormat(), false);
          Luminous::Utils::glTexRect(Nimble::Vector2(0,0), Nimble::Vector2(10, 10));
        }


        double loadTime = t1.sinceSecondsD() * 1000 / texturesperlevel;

        Luminous::Utils::glCheck("Texture test 2/3");

        t1 = Radiant::TimeStamp::getTime();

        for(j = 0; j < texturesperlevel; j++) {
          // reload the actual data:
          textures[j][k].bind();
          glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, images[i][k].width(), images[i][k].height(),
                          images[i][k].pixelFormat().layout(), GL_UNSIGNED_BYTE, images[i][k].data());
          Luminous::Utils::glTexRect(Nimble::Vector2(10,10), Nimble::Vector2(20, 20));
        }

        double subloadTime = t1.sinceSecondsD() * 1000 / texturesperlevel;

        Luminous::Utils::glCheck("Texture test 3/3");

        info("Texture dimensions %d %d, fmtindex = %d, create = %.3lf, load = %.3lf reload = %.3lf milliseconds",
             images[i][k].width(), images[i][k].height(), k,
             createTime, loadTime, subloadTime);

      }

    }

    SDL_GL_SwapBuffers();
  }

  return 0;
}
