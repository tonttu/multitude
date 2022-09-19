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

#include <Luminous/VertexBuffer.hpp>
#include <Luminous/RenderContext.hpp>
#include <Luminous/Image.hpp>
#include <Luminous/Shader.hpp>
#include <Luminous/Utils.hpp>

#include <Nimble/Random.hpp>

#include <Radiant/Sleep.hpp>
#include <Radiant/Trace.hpp>

#include <Valuable/CmdParser.hpp>

using namespace Radiant;

/* The data for a single quad. These are the paramters that are different
   between the different quads.
*/
class Point
{
public:
  Nimble::Vector2 pos;
  float size;
  float alpha;
};

int main(int /*argc*/, char ** /*argv*/)
{
  // The size of our display
  Nimble::Vector2 size(1000, 600);

  // Initialize SDL & OpenGL:
  SDL_Init(SDL_INIT_VIDEO);

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE,   8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,  8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16 );
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1 );

  SDL_SetVideoMode(size.x, size.y, 0, SDL_OPENGL);

  Luminous::initLuminous();

  glViewport(0, 0, size.x, size.y);

  Luminous::RenderContext rsc;
  Luminous::RenderContext::setThreadContext( & rsc);

  // Create the GLSL program, and load the various shaders into it
  Luminous::GLSLProgramObject prog;

  Luminous::GLSLShaderObject * geoshader =
      Luminous::GLSLShaderObject::fromFile(GL_GEOMETRY_SHADER_EXT, "shader-quads.gs");
  Luminous::GLSLShaderObject * vertexhader =
      Luminous::GLSLShaderObject::fromFile(GL_VERTEX_SHADER, "shader-quads.vs");
  Luminous::GLSLShaderObject * pixelshader =
      Luminous::GLSLShaderObject::fromFile(GL_FRAGMENT_SHADER, "shader-quads.ps");

  if(!geoshader || !vertexhader || !pixelshader) {
    error("Shader compilation failed.");
    return -1;
  }

  prog.addObject(pixelshader);
  prog.addObject(geoshader);
  prog.addObject(vertexhader);

  bool stop = false;

  // Set up the processing parameters for the geometry shader
  prog.setProgramParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_POINTS);
  prog.setProgramParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
  prog.setProgramParameter(GL_GEOMETRY_VERTICES_OUT_EXT, 6);

  Luminous::Utils::glCheck("Creating the geometry shader");

  // Create some random points
  const int n = 40000;
  std::vector<Point> items(n);

  Nimble::Rectf rect(Nimble::Vector2(0,0), size);

  for(int i = 0; i < n; i++) {
    items[i].pos   = Nimble::RandomUniform::instance().randVec2InRect(rect);
    items[i].size  = Nimble::RandomUniform::instance().randMinMax(5, 20);
    items[i].alpha = Nimble::RandomUniform::instance().randMinMax(0.01f, 0.1f);
  }


  if(!prog.link()) {
    error("When linking program: %s", prog.linkerLog());
    return -1;
  }

  // Set the GLSL program parameters
  prog.bind();

  prog.setUniformVector2("vsiz", size);

  int ppos = prog.getAttribLoc("pos");
  int psiz = prog.getAttribLoc("size");
  int palp = prog.getAttribLoc("alpha");

  info("Attribute locations: %d %d %d", ppos, psiz, palp);

  // VBO for rendering the points
  Luminous::VertexBuffer vbo;

  // Start the rendering
  Luminous::Utils::glUsualBlend();

  Radiant::TimeStamp begin(Radiant::TimeStamp::getTime());

  int frames = 0;

  for( ; !stop; frames++) {

    // Check if we should exit:
    SDL_Event event;

    if(SDL_PollEvent(&event)) {
      switch(event.type) {
      case SDL_QUIT:
        Radiant::info("Quit called, stopping now");
        stop = true;
      };
    }

    // Setup basic transformations:
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, size.x, 0, size.y, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0.2f, 0.2f, 0.2f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glColor4f(1, 1, 1, 1);

    /* Randomize the locations of the points. This way we get new data
       for every frame. */

    for(int i = 0; i < n; i++) {
      items[i].pos = Nimble::RandomUniform::instance().randVec2InRect(rect);
    }

    // Load the data.
    vbo.fill( & items[0], n * sizeof(Point), Luminous::VertexBuffer::DYNAMIC_DRAW);

    // Bind the VBO and GLSL program
    prog.bind();
    vbo.bind();

    // Set up the VBO for rendering
    glEnableVertexAttribArray(ppos);
    glEnableVertexAttribArray(psiz);
    glEnableVertexAttribArray(palp);

    glVertexAttribPointer(ppos, 2, GL_FLOAT, GL_FALSE, sizeof(Point), BUFFER_OFFSET(0));
    glVertexAttribPointer(psiz, 1, GL_FLOAT, GL_FALSE, sizeof(Point), BUFFER_OFFSET(sizeof(float) * 2));
    glVertexAttribPointer(palp, 1, GL_FLOAT, GL_FALSE, sizeof(Point), BUFFER_OFFSET(sizeof(float) * 3));

    // Draw all the points (aka squares)
    glDrawArrays(GL_POINTS, 0, n);

    glDisableVertexAttribArray(ppos);
    glDisableVertexAttribArray(psiz);
    glDisableVertexAttribArray(palp);

    // Unbind the VBO and the GLSL program
    vbo.unbind();
    prog.unbind();

    // Check that everything went fine
    Luminous::Utils::glCheck("After rendering a frame");

    // The frame is rendered, now swap the buffer
    SDL_GL_SwapBuffers();
  }

  // Report the FPS to the user
  float fps = frames / begin.sinceSecondsD();
  info("Rendered %d quads per frames, %.2f fps ", n, fps);

  return 0;
}
