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

#include <Radiant/Sleep.hpp>
#include <Radiant/Trace.hpp>

#include <Valuable/CmdParser.hpp>

#include <QStringList>

using namespace Radiant;

int main(int argc, char ** argv)
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

  Luminous::RenderContext rsc;
  Luminous::RenderContext::setThreadContext( & rsc );

  int i, j = 0;

  const int levels = 13;
  const int texturesperlevel = 5;
  const int formatsperlevel = 3;

  Valuable::Node opts;
  Valuable::AttributeInt uselevels(&opts, "levels", 12);
  Valuable::AttributeBool drawrects(&opts, "drawrects", false);

  Valuable::CmdParser::parse(argc, argv, opts);

  uselevels = Nimble::Math::Clamp((int) uselevels, 6, levels);

  Luminous::Image images[levels][formatsperlevel];

  const char * formatnames[formatsperlevel] = { "RGB ", "RGBA", "BGRA" };

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

    for(i = 5; i < uselevels; i++) {

      printf("\n");

      for(int k = 0; k < formatsperlevel; k++) {

        GLenum glLayout = (k == 0) ? GL_RGB : GL_RGBA;

        int usetextures = texturesperlevel;

        if(images[i][k].width() > 2048)
          usetextures = 1;

        Radiant::TimeStamp t1 = Radiant::TimeStamp::getTime();

        for(j = 0; j < usetextures; j++) {
          // Create textures without loading:
          textures[j][k].loadBytes(glLayout, images[i][k].width(), images[i][k].height(), 0,
                                   images[i][k].pixelFormat(), false);
          // Luminous::Utils::glTexRect(Nimble::Vector2(0,100), Nimble::Vector2(10, 110));

        }


        double createTime = t1.sinceSecondsD() * 1000 / usetextures;

        // Radiant::Sleep::sleepMs(500);

        Luminous::Utils::glCheck("Texture test 1/3");

        t1 = Radiant::TimeStamp::getTime();


        for(j = 0; j < usetextures; j++) {
          // Create textures and load part of the actual data:
          textures[j][k].loadBytes(glLayout, images[i][k].width(), images[i][k].height(), 0,
                                   images[i][k].pixelFormat(), false);
          glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, images[i][k].width(), images[i][k].height() / 8,
                          images[i][k].pixelFormat().layout(), GL_UNSIGNED_BYTE, images[i][k].data());
          if(drawrects)
            Luminous::Utils::glTexRect(Nimble::Vector2(0,0), Nimble::Vector2(10, 10));
        }


        double someTime = t1.sinceSecondsD() * 1000 / usetextures;

        Luminous::Utils::glCheck("Texture test 2/4");

        t1 = Radiant::TimeStamp::getTime();


        for(j = 0; j < usetextures; j++) {
          // Create textures and load the actual data:
          textures[j][k].loadBytes(glLayout, images[i][k].width(), images[i][k].height(),
                                   images[i][k].data(),
                                   images[i][k].pixelFormat(), false);
          if(drawrects)
            Luminous::Utils::glTexRect(Nimble::Vector2(0,0), Nimble::Vector2(10, 10));
        }


        double loadTime = t1.sinceSecondsD() * 1000 / usetextures;

        Luminous::Utils::glCheck("Texture test 3/4");

        t1 = Radiant::TimeStamp::getTime();

        for(j = 0; j < usetextures; j++) {
          // reload the actual data:
          textures[j][k].bind();
          glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, images[i][k].width(), images[i][k].height(),
                          images[i][k].pixelFormat().layout(), GL_UNSIGNED_BYTE, images[i][k].data());
          if(drawrects)
            Luminous::Utils::glTexRect(Nimble::Vector2(10,10), Nimble::Vector2(20, 20));
        }

        double subloadTime = t1.sinceSecondsD() * 1000 / usetextures;

        Luminous::Utils::glCheck("Texture test 4/5");

        info("%s %d x %d, create = %.3lf, some = %.3lf, load = %.3lf reload = %.3lf ms",
             formatnames[k], images[i][k].width(), images[i][k].height(),
             createTime, someTime, loadTime, subloadTime);

      }

    }

    SDL_GL_SwapBuffers();
  }

  return 0;
}
